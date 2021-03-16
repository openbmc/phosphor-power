#include "psu_manager.hpp"

#include "utility.hpp"

#include <fmt/format.h>
#include <sys/types.h>
#include <unistd.h>

using namespace phosphor::logging;

namespace phosphor::power::manager
{

constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";
constexpr auto maxCountProp = "MaxCount";

PSUManager::PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e,
                       const std::string& configfile) :
    bus(bus)
{
    // Parse out the JSON properties
    sysProperties = {0};
    getJSONProperties(configfile);
    // Subscribe to InterfacesAdded before doing a property read, otherwise
    // the interface could be created after the read attempt but before the
    // match is created.
    entityManagerIfacesAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::sender(
                "xyz.openbmc_project.EntityManager"),
        std::bind(&PSUManager::entityManagerIfaceAdded, this,
                  std::placeholders::_1));
    getSystemProperties();

    using namespace sdeventplus;
    auto interval = std::chrono::milliseconds(1000);
    timer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
        e, std::bind(&PSUManager::analyze, this), interval);

    // Subscribe to power state changes
    powerService = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);
    powerOnMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(POWER_OBJ_PATH,
                                                        POWER_IFACE),
        [this](auto& msg) { this->powerStateChanged(msg); });

    initialize();
}

void PSUManager::getJSONProperties(const std::string& path)
{
    nlohmann::json configFileJSON = util::loadJSONFromFile(path.c_str());

    if (configFileJSON == nullptr)
    {
        throw std::runtime_error("Failed to load JSON configuration file");
    }

    if (!configFileJSON.contains("PowerSupplies"))
    {
        throw std::runtime_error("Missing required PowerSupplies");
    }

    for (auto psuJSON : configFileJSON["PowerSupplies"])
    {
        if (psuJSON.contains("Inventory") && psuJSON.contains("Bus") &&
            psuJSON.contains("Address"))
        {
            std::string invpath = psuJSON["Inventory"];
            std::uint8_t i2cbus = psuJSON["Bus"];
            std::string i2caddr = psuJSON["Address"];
            auto psu =
                std::make_unique<PowerSupply>(bus, invpath, i2cbus, i2caddr);
            psus.emplace_back(std::move(psu));
        }
        else
        {
            log<level::ERR>("Insufficient PowerSupply properties");
        }
    }

    if (psus.empty())
    {
        throw std::runtime_error("No power supplies to monitor");
    }
}

void PSUManager::populateSysProperties(util::DbusPropertyMap& properties)
{
    try
    {
        auto propIt = properties.find(maxCountProp);
        if (propIt != properties.end())
        {
            const uint64_t* count = std::get_if<uint64_t>(&(propIt->second));
            if (count != nullptr)
            {
                sysProperties.maxPowerSupplies = *count;
            }
        }
    }
    catch (std::exception& e)
    {
    }
}

void PSUManager::getSystemProperties()
{

    try
    {
        util::DbusSubtree subtree =
            util::getSubTree(bus, INVENTORY_OBJ_PATH, supportedConfIntf, 0);
        if (subtree.empty())
        {
            throw std::runtime_error("Supported Configuration Not Found");
        }

        for (const auto& [objPath, services] : subtree)
        {
            std::string service = services.begin()->first;
            if (objPath.empty() || service.empty())
            {
                continue;
            }
            auto properties = util::getAllProperties(
                bus, objPath, supportedConfIntf, service);
            populateSysProperties(properties);
        }
    }
    catch (std::exception& e)
    {
        // Interface or property not found. Let the Interfaces Added callback
        // process the information once the interfaces are added to D-Bus.
    }
}

void PSUManager::entityManagerIfaceAdded(sdbusplus::message::message& msg)
{
    try
    {
        sdbusplus::message::object_path objPath;
        std::map<std::string, std::map<std::string, util::DbusVariant>>
            interfaces;
        msg.read(objPath, interfaces);

        auto itIntf = interfaces.find(supportedConfIntf);
        if (itIntf == interfaces.cend())
        {
            return;
        }

        populateSysProperties(itIntf->second);
    }
    catch (std::exception& e)
    {
        // Ignore, the property may be of a different type than expected.
    }
}

void PSUManager::powerStateChanged(sdbusplus::message::message& msg)
{
    int32_t state = 0;
    std::string msgSensor;
    std::map<std::string, std::variant<int32_t>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find("state");
    if (valPropMap != msgData.end())
    {
        state = std::get<int32_t>(valPropMap->second);

        // Power is on when state=1. Clear faults.
        if (state)
        {
            powerOn = true;
            clearFaults();
        }
        else
        {
            powerOn = false;
        }
    }
}

void PSUManager::createError(
    const std::string& faultName,
    const std::map<std::string, std::string>& additionalData)
{
    using namespace sdbusplus::xyz::openbmc_project;
    constexpr auto loggingObjectPath = "/xyz/openbmc_project/logging";
    constexpr auto loggingCreateInterface =
        "xyz.openbmc_project.Logging.Create";

    try
    {
        auto service =
            util::getService(loggingObjectPath, loggingCreateInterface, bus);

        if (service.empty())
        {
            log<level::ERR>("Unable to get logging manager service");
            return;
        }

        auto method = bus.new_method_call(service.c_str(), loggingObjectPath,
                                          loggingCreateInterface, "Create");

        auto level = Logging::server::Entry::Level::Error;
        method.append(faultName, level, additionalData);

        auto reply = bus.call(method);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::format(
                "Failed creating event log for fault {} due to error {}",
                faultName, e.what())
                .c_str());
    }
}

void PSUManager::analyze()
{
    for (auto& psu : psus)
    {
        psu->analyze();
    }

    if (powerOn)
    {
        for (auto& psu : psus)
        {
            std::map<std::string, std::string> additionalData;
            additionalData["_PID"] = std::to_string(getpid());
            // TODO: Fault priorities #918
            if (!psu->isFaultLogged() && !psu->isPresent())
            {
                // Create error for power supply missing.
                additionalData["CALLOUT_INVENTORY_PATH"] =
                    psu->getInventoryPath();
                additionalData["CALLOUT_PRIORITY"] = "H";
                createError(
                    "xyz.openbmc_project.Power.PowerSupply.Error.Missing",
                    additionalData);
                psu->setFaultLogged();
            }
            else if (!psu->isFaultLogged() && psu->isFaulted())
            {
                additionalData["STATUS_WORD"] =
                    std::to_string(psu->getStatusWord());
                additionalData["STATUS_MFR"] =
                    std::to_string(psu->getMFRFault());
                // If there are faults being reported, they possibly could be
                // related to a bug in the firmware version running on the power
                // supply. Capture that data into the error as well.
                additionalData["FW_VERSION"] = psu->getFWVersion();

                if ((psu->hasInputFault() || psu->hasVINUVFault()))
                {
                    /* The power supply location might be needed if the input
                     * fault is due to a problem with the power supply itself.
                     * Include the inventory path with a call out priority of
                     * low.
                     */
                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();
                    additionalData["CALLOUT_PRIORITY"] = "L";
                    createError("xyz.openbmc_project.Power.PowerSupply.Error."
                                "InputFault",
                                additionalData);
                    psu->setFaultLogged();
                }
                else if (psu->hasMFRFault())
                {
                    /* This can represent a variety of faults that result in
                     * calling out the power supply for replacement: Output
                     * OverCurrent, Output Under Voltage, and potentially other
                     * faults.
                     *
                     * Also plan on putting specific fault in AdditionalData,
                     * along with register names and register values
                     * (STATUS_WORD, STATUS_MFR, etc.).*/

                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.Fault",
                        additionalData);

                    psu->setFaultLogged();
                }
                else if (psu->hasCommFault())
                {
                    /* Attempts to communicate with the power supply have
                     * reached there limit. Create an error. */
                    additionalData["CALLOUT_DEVICE_PATH"] =
                        psu->getDevicePath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.CommFault",
                        additionalData);

                    psu->setFaultLogged();
                }
            }
        }
    }
}

} // namespace phosphor::power::manager
