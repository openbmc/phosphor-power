#include "config.h"

#include "chassis_manager.hpp"

#include <iostream>

using namespace phosphor::logging;

namespace phosphor::power::chassis_manager
{
constexpr auto managerBusName =
    "xyz.openbmc_project.Power.MultiChassisPSUMonitor";
constexpr auto objectManagerObjPath =
    "/xyz/openbmc_project/power/power_supplies";
constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";

constexpr auto INPUT_HISTORY_SYNC_DELAY = 5;

ChassisManager::ChassisManager(sdbusplus::bus_t& bus,
                               const sdeventplus::Event& e) :
    bus(bus), objectManager(bus, objectManagerObjPath), eventLoop(e)
{
    // Subscribe to InterfacesAdded before doing a property read, otherwise
    // the interface could be created after the read attempt but before the
    // match is created.
    entityManagerIfacesAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::sender(
                "xyz.openbmc_project.EntityManager"),
        std::bind(&ChassisManager::entityManagerIfaceAdded, this,
                  std::placeholders::_1));

    initializeChassisList();

    // Request the bus name before the analyze() function, which is the one that
    // determines the brownout condition and sets the status d-bus property.
    bus.request_name(managerBusName);

    using namespace sdeventplus;
    auto interval = std::chrono::milliseconds(1000);
    timer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
        e, std::bind(&ChassisManager::analyze, this), interval);
}

void ChassisManager::entityManagerIfaceAdded(sdbusplus::message_t& msg)
{
    try
    {
        sdbusplus::message::object_path objPath;
        std::map<std::string, std::map<std::string, util::DbusVariant>>
            interfaces;
        msg.read(objPath, interfaces);

        std::string tmp = objPath;

        if (interfaces.count(PSU_INVENTORY_IFACE))
        {
            for (auto& chassis : chassisList)
            {
                if (chassis->getChassisPath() == objPath)
                {
                    auto itIntf = interfaces.find(supportedConfIntf);
                    if (itIntf != interfaces.cend())
                    {
                        chassis->populateSupportedConfiguration(itIntf->second);
                        chassis->updateMissingPSUs();
                    }

                    itIntf = interfaces.find(IBMCFFPSInterface);
                    if (itIntf != interfaces.cend())
                    {
                        lg2::info("InterfacesAdded for: {IBMCFFPSINTERFACE}",
                                  "IBMCFFPSINTERFACE", IBMCFFPSInterface);
                        chassis->getPSUProperties(itIntf->second);
                        chassis->updateMissingPSUs();
                    }
                    // Call to validate the psu configuration if the power is on
                    // and both the IBMCFFPSConnector and SupportedConfiguration
                    // interfaces have been processed
                    if (chassis->isPowerOn() && !chassis->isPsusEmpty() &&
                        !chassis->isSupportedConfigsEmpty())
                    {
                        chassis->restartValidationTimerOnce();
                    }
                    break;
                }
            }
        }
        else if (interfaces.count(CHASSIS_IFACE))
        {
            addChassisIfNew(objPath);
        }
    }
    catch (const std::exception& e)
    {
        lg2::info("entityManagerIfaceAdded exception: {ERROR}", "ERROR", e);
        // Ignore, the property may be of a different type than expected.
    }
}

void ChassisManager::analyze()
{
    for (const auto& chassis : chassisList)
    {
        chassis->analyze();
    }
}

void ChassisManager::initializeChassisList()
{
    try
    {
        auto chassisList =
            phosphor::power::util::getSubTree(bus, "/", CHASSIS_IFACE, 0);

        for (const auto& [chassisPath, serviceMap] : chassisList)
        {
            lg2::info(
                "ChassisManager::initializeChassisList chassisPath= {CHASSIS_PATH}",
                "CHASSIS_PATH", chassisPath);
            addChassisIfNew(chassisPath);
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to initialize chassis list, error: {ERROR}", "ERROR",
                   e);
    }
}

void ChassisManager::addChassisIfNew(const std::string& path)
{
    bool present{false};
    phosphor::power::util::getProperty(INVENTORY_IFACE, PRESENT_PROP, path,
                                       INVENTORY_MGR_IFACE, bus, present);
    if (knownChassisPaths.contains(path) || !present)
    {
        return;
    }
    auto chassis = std::make_unique<phosphor::power::chassis::Chassis>(
        bus, path, eventLoop);
    knownChassisPaths.insert(path);
    chassisList.push_back(std::move(chassis));
}
} // namespace phosphor::power::chassis_manager
