#include "psu_manager.hpp"

#include "utility.hpp"

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
    // Parse out the JSON properties needed to pass down to the PSU manager.
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
    using namespace phosphor::logging;

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

    auto SysProps = configFileJSON["SystemProperties"];

    if (!SysProps.contains("pollInterval"))
    {
        throw std::runtime_error("Missing required pollInterval property");
    }

    p.pollInterval = SysProps["pollInterval"];

    if (SysProps.contains("MinPowerSupplies"))
    {
        p.minPowerSupplies = SysProps["MinPowerSupplies"];
    }
    else
    {
        p.minPowerSupplies = 0;
    }

    if (SysProps.contains("MaxPowerSupplies"))
    {
        p.maxPowerSupplies = SysProps["MaxPowerSupplies"];
    }
    else
    {
        p.maxPowerSupplies = 0;
    }

    for (auto psuJSON : configFileJSON["PowerSupplies"])
    {
        if (psuJSON.contains("Inventory"))
        {
            std::string invpath = psuJSON["Inventory"];
            auto psu = std::make_unique<PowerSupply>(bus, invpath);
            psus.emplace_back(std::move(psu));
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
