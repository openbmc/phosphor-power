#include "psu_manager.hpp"

#include "utility.hpp"

using namespace phosphor::logging;

namespace phosphor
{
namespace power
{
namespace manager
{

PSUManager::PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e,
                       const std::string& configfile) :
    bus(bus)
{
    // Parse out the JSON properties
    sys_properties properties;
    getJSONProperties(configfile, bus, properties, psus);

    using namespace sdeventplus;
    auto interval = std::chrono::milliseconds(properties.pollInterval);
    timer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
        e, std::bind(&PSUManager::analyze, this), interval);

    minPSUs = {properties.minPowerSupplies};
    maxPSUs = {properties.maxPowerSupplies};

    // Subscribe to power state changes
    powerService = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);
    powerOnMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(POWER_OBJ_PATH,
                                                        POWER_IFACE),
        [this](auto& msg) { this->powerStateChanged(msg); });

    initialize();
}

void PSUManager::getJSONProperties(
    const std::string& path, sdbusplus::bus::bus& bus, sys_properties& p,
    std::vector<std::unique_ptr<PowerSupply>>& psus)
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

    if (!sysProps.contains("pollInterval"))
    {
        throw std::runtime_error("Missing required pollInterval property");
    }

    p.pollInterval = sysProps["pollInterval"];

    if (sysProps.contains("MinPowerSupplies"))
    {
        p.minPowerSupplies = sysProps["MinPowerSupplies"];
    }
    else
    {
        p.minPowerSupplies = 0;
    }

    if (sysProps.contains("MaxPowerSupplies"))
    {
        p.maxPowerSupplies = sysProps["MaxPowerSupplies"];
    }
    else
    {
        p.maxPowerSupplies = 0;
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

void PSUManager::powerStateChanged(sdbusplus::message::message& msg)
{
    int32_t state = 0;
    std::string msgSensor;
    std::map<std::string, sdbusplus::message::variant<int32_t>> msgData;
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

} // namespace manager
} // namespace power
} // namespace phosphor
