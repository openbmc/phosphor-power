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

constexpr auto assocProperty = "Associations";
constexpr auto assocInterface = "xyz.openbmc_project.Association.Definitions";
constexpr auto INVENTORY_ITEM_IFACE = "xyz.openbmc_project.Inventory.Item";

ChassisManager::ChassisManager(sdbusplus::bus_t& bus,
                               const sdeventplus::Event& e) :
    bus(bus), eventLoop(e)
{
    // Determine if this is a multi-chassis system
    multiChassisSystem = isMultiChassis(bus);

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

    // Subscribe to Chassis Present property changes
    chassisPresentMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/inventory", INVENTORY_ITEM_IFACE),
        std::bind(&ChassisManager::chassisPresentChanged, this,
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
            lg2::info("InterfacesAdded supportedConfIntf- backplanePath= {BP}",
                      "BP", backplanePath);
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
            auto chassisPath = getChassisAssociation(associations);

            if (!chassisPath.empty())
            {
                std::filesystem::path chassisFsPath(chassisPath);
                std::string targetChassisPath =
                    chassisFsPath.parent_path().string();
                lg2::debug(
                    "InterfacesAdded IBMCFFPSInterface- targetChassisPath= {TCH}",
                    "TCH", targetChassisPath);

                // Check if chassis exists and is present
                bool chassisExists = false;
                bool chassisPresent = false;

                for (const auto& chassis : listOfChassis)
                {
                    if (chassis->getInventoryPath() == targetChassisPath)
                    {
                        chassisExists = true;
                        // Check if this chassis is still present
                        try
                        {
                            chassisPresent =
                                isChassisPresent(targetChassisPath);
                        }
                        catch (const std::exception& e)
                        {
                            lg2::warning(
                                "Failed to check chassis presence: {ERROR}",
                                "ERROR", e.what());
                            chassisPresent = false;
                        }
                        break;
                    }
                }

                // Add chassis if it doesn't exist OR if it exists but is not
                // present
                if (!chassisExists || !chassisPresent)
                {
                    if (chassisExists && !chassisPresent)
                    {
                        lg2::info("Removing stale chassis: {PATH}", "PATH",
                                  targetChassisPath);
                        chassisRemoved(targetChassisPath);
                    }

                    lg2::info("Adding chassis to list: {PATH}", "PATH",
                              targetChassisPath);
                    addChassisToList(targetChassisPath);
                }
                else
                {
                    lg2::debug("Chassis {PATH} already exists and is present",
                               "PATH", targetChassisPath);
                }

                auto myChassisId = getParentEMPositionId(bus, objPathStr);
                chassisMatchPtr = getMatchingChassisPtr(myChassisId);
                if (chassisMatchPtr)
                {
                    lg2::info("InterfacesAdded for: {IBMCFFPSINTERFACE}",
                              "IBMCFFPSINTERFACE", IBMCFFPSInterface);
                    chassisMatchPtr->psuInterfaceAdded(itInterface->second);
                }
            }
            else
            {
                lg2::warning("No chassis path found for backplane {BP}", "BP",
                             backplanePath);
            }

            if (chassisMatchPtr != nullptr)
            {
                lg2::debug(
                    "InterfacesAdded validatePsuConfigAndInterfacesProcessed()");
                chassisMatchPtr->validatePsuConfigAndInterfacesProcessed();
            }
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Exception in entityManagerIfaceAdded: {ERROR}", "ERROR",
                   e.what());
    }
}

void ChassisManager::chassisPresentChanged(sdbusplus::message_t& msg)
{
    try
    {
        std::string objectPath;
        std::string interface;
        std::map<std::string, util::DbusVariant> properties;
        msg.read(objectPath, interface, properties);

        // Only process if this is a chassis path
        if (objectPath.find("/chassis") == std::string::npos)
        {
            return;
        }

        auto it = properties.find("Present");
        if (it != properties.end())
        {
            bool present = std::get<bool>(it->second);

            lg2::info("Chassis {PATH} Present changed to {PRESENT}", "PATH",
                      objectPath, "PRESENT", present);

            if (!present)
            {
                // Chassis went away
                chassisRemoved(objectPath);
            }
            else
            {
                // Chassis came back
                lg2::info("Chassis returned: {PATH}", "PATH", objectPath);

                // Remove old instance if exists (to ensure clean state)
                chassisRemoved(objectPath);

                // Add fresh instance
                addChassisToList(objectPath);
            }
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Exception in chassisPresentChanged: {ERROR}", "ERROR",
                   e.what());
    }
}

void ChassisManager::chassisRemoved(const std::string& chassisPath)
{
    lg2::info("Removing chassis: {PATH}", "PATH", chassisPath);

    auto initialSize = listOfChassis.size();

    // Remove chassis from list
    listOfChassis.erase(
        std::remove_if(listOfChassis.begin(), listOfChassis.end(),
                       [&chassisPath](const auto& chassis) {
                           return chassis->getInventoryPath() == chassisPath;
                       }),
        listOfChassis.end());

    if (listOfChassis.size() < initialSize)
    {
        lg2::info("Chassis {PATH} removed from list", "PATH", chassisPath);
    }
    else
    {
        lg2::debug("Chassis {PATH} not found in list", "PATH", chassisPath);
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
            if (!isChassisPresent(chassisPath))
            {
                continue;
            }
            addChassisToList(chassisPath);
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
        bus, chassisPath, chassisName, eventLoop, multiChassisSystem);
    listOfChassis.push_back(std::move(chassis));
}

void ChassisManager::initChassisPowerMonitoring()
{
    for (const auto& chassis : listOfChassis)
    {
        chassis->initPowerMonitoring();
    }
}

bool ChassisManager::isChassisPresent(const std::string& chassisPath)
{
    constexpr auto INVENTORY_MGR_SERVICE =
        "xyz.openbmc_project.Inventory.Manager";

    try
    {
        bool isPresent = false;
        util::getProperty(INVENTORY_IFACE, "Present", chassisPath,
                          INVENTORY_MGR_SERVICE, bus, isPresent);
        return isPresent;
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get Present property for {PATH}: {ERROR}", "PATH",
                   chassisPath, "ERROR", e.what());
        return false;
    }
}

Associations ChassisManager::getAssociations(sdbusplus::bus_t& bus,
                                             const std::string& inventoryPath)
{
    Associations associations;

    auto service =
        phosphor::power::util::getService(inventoryPath, assocInterface, bus);

    if (service.empty())
    {
        lg2::debug("No service found for: {INV}", "INV", inventoryPath);
        return associations;
    }

    try
    {
        phosphor::power::util::getProperty<Associations>(
            assocInterface, assocProperty, inventoryPath, service, bus,
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
        if (endpoint.find("chassis") != std::string::npos)
        {
            std::filesystem::path fsPath(endpoint);
            return fsPath;
        }
    }

    return {};
}

} // namespace phosphor::power::chassis_manager
