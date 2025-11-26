#include "config.h"

#include "chassis_manager.hpp"

#include <phosphor-logging/lg2.hpp>

using namespace phosphor::logging;

namespace phosphor::power::chassis_manager
{
using namespace phosphor::power::util;
constexpr auto managerBusName =
    "xyz.openbmc_project.Power.MultiChassisPSUMonitor";
constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";

ChassisManager::ChassisManager(sdbusplus::bus_t& bus,
                               const sdeventplus::Event& e) :
    bus(bus), eventLoop(e)
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
    initChassisPowerMonitoring();
}

void ChassisManager::entityManagerIfaceAdded(sdbusplus::message_t& msg)
{
    try
    {
        phosphor::power::chassis::Chassis* chassisMatchPtr = nullptr;
        sdbusplus::message::object_path objPath;
        std::map<std::string, std::map<std::string, util::DbusVariant>>
            interfaces;
        msg.read(objPath, interfaces);

        std::string objPathStr = objPath;

        auto itInterface = interfaces.find(supportedConfIntf);
        if (itInterface != interfaces.cend())
        {
            lg2::info("InterfacesAdded supportedConfIntf- objPathStr= {OBJ}",
                      "OBJ", objPathStr);
            auto myChassisId = getParentEMUniqueId(bus, objPathStr);
            chassisMatchPtr = getMatchingChassisPtr(myChassisId);
            if (chassisMatchPtr)
            {
                lg2::debug("InterfacesAdded for: {SUPPORTED_CONFIGURATION}",
                           "SUPPORTED_CONFIGURATION", supportedConfIntf);
                chassisMatchPtr->supportedConfigurationInterfaceAdded(
                    itInterface->second);
            }
        }
        itInterface = interfaces.find(IBMCFFPSInterface);
        if (itInterface != interfaces.cend())
        {
            lg2::debug("InterfacesAdded IBMCFFPSInterface- objPathStr= {OBJ}",
                       "OBJ", objPathStr);
            auto myChassisId = getParentEMUniqueId(bus, objPathStr);
            chassisMatchPtr = getMatchingChassisPtr(myChassisId);
            if (chassisMatchPtr)
            {
                lg2::info("InterfacesAdded for: {IBMCFFPSINTERFACE}",
                          "IBMCFFPSINTERFACE", IBMCFFPSInterface);
                chassisMatchPtr->psuInterfaceAdded(itInterface->second);
            }
        }
        if (chassisMatchPtr != nullptr)
        {
            lg2::debug(
                "InterfacesAdded validatePsuConfigAndInterfacesProcessed()");
            chassisMatchPtr->validatePsuConfigAndInterfacesProcessed();
        }
    }
    catch (const std::exception& e)
    {
        // Ignore, the property may be of a different type than expected.
    }
}

phosphor::power::chassis::Chassis* ChassisManager::getMatchingChassisPtr(
    uint64_t chassisId)
{
    for (const auto& chassisPtr : listOfChassis)
    {
        if (chassisPtr->getChassisId() == chassisId)
        {
            return chassisPtr.get();
        }
    }
    lg2::debug("Chassis ID {ID} not found", "ID", chassisId);
    return nullptr;
}

void ChassisManager::analyze()
{
    for (const auto& chassis : listOfChassis)
    {
        chassis->analyze();
    }
}

void ChassisManager::initializeChassisList()
{
    try
    {
        auto chassisPathList = getChassisInventoryPaths(bus);
        for (const auto& chassisPath : chassisPathList)
        {
            std::filesystem::path path(chassisPath);
            const std::string chassisName = path.filename();

            lg2::info(
                "ChassisManager::initializeChassisList chassisPath= {CHASSIS_PATH}",
                "CHASSIS_PATH", chassisPath);
            auto chassis = std::make_unique<phosphor::power::chassis::Chassis>(
                bus, chassisPath, chassisName, eventLoop);
            listOfChassis.push_back(std::move(chassis));
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to initialize chassis list, error: {ERROR}", "ERROR",
                   e);
    }
}

void ChassisManager::initChassisPowerMonitoring()
{
    for (const auto& chassis : listOfChassis)
    {
        chassis->initPowerMonitoring();
    }
}

} // namespace phosphor::power::chassis_manager
