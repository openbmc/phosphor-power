#include "config.h"

#include "chassis_manager.hpp"

#include <phosphor-logging/lg2.hpp>

using namespace phosphor::logging;

namespace phosphor::power::chassis_manager
{
using namespace phosphor::power::util;
constexpr auto managerBusName = "xyz.openbmc_project.Power.PSUMonitor";
constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";

ChassisManager::ChassisManager(sdbusplus::bus_t& bus,
                               const sdeventplus::Event& e) :
    bus(bus), eventLoop(e)
{
    // Determine if this is a multi-chassis system
    isMultiChassisSystem = isMultiChassis(bus);

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

    // Initialize all chassis (present or not)
    // Each chassis will set up its own present property listener
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
        sdbusplus::object_path objPath;
        std::map<std::string, std::map<std::string, util::DbusVariant>>
            interfaces;
        msg.read(objPath, interfaces);
        std::string objPathStr = objPath;

        std::filesystem::path fsPath(objPathStr);
        std::string backplanePath = fsPath.parent_path().string();
        auto itInterface = interfaces.find(supportedConfIntf);
        if (itInterface != interfaces.cend())
        {
            lg2::info("InterfacesAdded supportedConfIntf- objPath= {BP}", "BP",
                      objPath);
            auto myPositionId = getEMPositionId(bus, backplanePath);
            chassisMatchPtr = getMatchingChassisPtr(myPositionId);
            if (chassisMatchPtr)
            {
                chassisMatchPtr->supportedConfigurationInterfaceAdded(
                    itInterface->second);
            }
        }

        itInterface = interfaces.find(IBMCFFPSInterface);
        if (itInterface != interfaces.cend())
        {
            auto associations = getAssociations(bus, backplanePath);
            auto targetChassisPath = getChassisAssociation(associations);

            if (!targetChassisPath.empty())
            {
                auto myChassisId = getParentEMPositionId(bus, objPathStr);
                chassisMatchPtr = getMatchingChassisPtr(myChassisId);
                if (chassisMatchPtr)
                {
                    lg2::info("InterfacesAdded for: {IBMCFFPSINTERFACE}",
                              "IBMCFFPSINTERFACE", IBMCFFPSInterface);
                    chassisMatchPtr->psuInterfaceAdded(itInterface->second);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Exception in entityManagerIfaceAdded: {ERROR}", "ERROR", e);
    }
}

phosphor::power::chassis::Chassis* ChassisManager::getMatchingChassisPtr(
    uint64_t chassisPositionId)
{
    for (const auto& chassisPtr : listOfChassis)
    {
        if (chassisPtr->getChassisPositionId() == chassisPositionId)
        {
            return chassisPtr.get();
        }
    }
    lg2::debug("Chassis ID {ID} not found", "ID", chassisPositionId);
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
            // Add all chassis to the list regardless of present state
            // Each chassis will monitor its own present property
            addChassisToList(chassisPath);
            lg2::info("Added chassis to list: {PATH}", "PATH", chassisPath);
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to initialize chassis list, error: {ERROR}", "ERROR",
                   e);
    }
}

void ChassisManager::addChassisToList(const std::string& chassisPath)
{
    std::filesystem::path path(chassisPath);
    const std::string chassisName = path.filename();

    auto chassis = std::make_unique<phosphor::power::chassis::Chassis>(
        bus, chassisPath, chassisName, eventLoop, isMultiChassisSystem);
    listOfChassis.push_back(std::move(chassis));
}

void ChassisManager::initChassisPowerMonitoring()
{
    for (const auto& chassis : listOfChassis)
    {
        chassis->initPowerMonitoring();
    }
}

Associations ChassisManager::getAssociations(sdbusplus::bus_t& bus,
                                             const std::string& inventoryPath)
{
    Associations associations;

    auto service =
        phosphor::power::util::getService(inventoryPath, ASSOC_DEF_IFACE, bus);

    if (service.empty())
    {
        lg2::debug("No service found for: {INV}", "INV", inventoryPath);
        return associations;
    }

    try
    {
        phosphor::power::util::getProperty<Associations>(
            ASSOC_DEF_IFACE, ASSOC_PROP, inventoryPath, service, bus,
            associations);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get associations for {PATH}: {ERROR}", "PATH",
                   inventoryPath, "ERROR", e.what());
    }

    return associations;
}

std::string ChassisManager::getChassisAssociation(
    const Associations& associations)
{
    for (const auto& [forward, reverse, endpoint] : associations)
    {
        if (endpoint.find("powersupply") != std::string::npos)
        {
            std::filesystem::path fsPath(endpoint);
            return fsPath;
        }
    }

    return {};
}

} // namespace phosphor::power::chassis_manager
