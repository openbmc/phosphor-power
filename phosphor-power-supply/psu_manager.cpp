#include "psu_manager.hpp"

#include "utility.hpp"

#include <fmt/format.h>
#include <sys/types.h>
#include <unistd.h>

using namespace phosphor::logging;

namespace phosphor::power::manager
{

PSUManager::PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e,
                       const std::string& configfile) :
    bus(bus)
{
    // Parse out the JSON properties
    sysProperties = {0};
    getJSONProperties(configfile);

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

PSUManager::PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e) :
    bus(bus)
{
    sysProperties = {0};
    std::string IBMCFFPSInterface =
        "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
    getPSUProperties(bus, IBMCFFPSInterface, psus);

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

    if (!configFileJSON.contains("SystemProperties"))
    {
        throw std::runtime_error("Missing required SystemProperties");
    }

    if (!configFileJSON.contains("PowerSupplies"))
    {
        throw std::runtime_error("Missing required PowerSupplies");
    }

    auto sysProps = configFileJSON["SystemProperties"];

    if (sysProps.contains("MaxPowerSupplies"))
    {
        sysProperties.maxPowerSupplies = sysProps["MaxPowerSupplies"];
    }

    for (auto psuJSON : configFileJSON["PowerSupplies"])
    {
        if (psuJSON.contains("Inventory") && psuJSON.contains("Bus") &&
            psuJSON.contains("Address"))
        {
            std::string invpath = psuJSON["Inventory"];
            std::uint8_t i2cbus = psuJSON["Bus"];
            std::uint16_t i2caddr = static_cast<uint16_t>(psuJSON["Address"]);
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

void PSUManager::getPSUProperties(
    sdbusplus::bus::bus& bus, const std::string& interface,
    std::vector<std::unique_ptr<PowerSupply>>& psus)
{
    using namespace phosphor::power::util;
    const auto basePSUInvPath =
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply";
    auto depth = 0;
    auto objects = getSubTree(bus, "/", interface, depth);
    // I should get an array of objects back.
    // Each object will have a path, a service, and an interface.
    // The interface should match the one passed into this function.
    for (const auto& elem : objects)
    {
        // For each element in the array of objects, I want to get properties
        // from the service, path, interface, and property: I2CBus, I2CAddress,
        // and the Name, which will be used to build the inventory path.
        auto service = elem.second.begin()->first;
        auto path = elem.first;

        std::uint64_t i2cbus = 0;
        std::uint64_t i2caddr = 0;
        std::string psuname;

        getProperty<std::uint64_t>(interface, "I2CBus", path, service, bus,
                                   i2cbus);
        getProperty<std::uint64_t>(interface, "I2CAddress", path, service, bus,
                                   i2caddr);
        getProperty<std::string>(interface, "Name", path, service, bus,
                                 psuname);

        std::string invpath = basePSUInvPath;
        invpath.push_back(psuname.back());

        log<level::DEBUG>(fmt::format("Inventory Path: {}", invpath).c_str());

        auto psu = std::make_unique<PowerSupply>(bus, invpath, i2cbus, i2caddr);
        psus.emplace_back(std::move(psu));
    }

    if (psus.empty())
    {
        throw std::runtime_error("No power supplies to monitor");
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
