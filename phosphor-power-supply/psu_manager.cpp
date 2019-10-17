#include "psu_manager.hpp"

#include "utility.hpp"

namespace phosphor
{
namespace power
{
namespace manager
{

void PSUManager::updatePowerState()
{
    // When state = 1, system is powered on
    int32_t state = 0;

    try
    {
        auto service = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);

        // Use getProperty utility function to get power state.
        util::getProperty<int32_t>(POWER_IFACE, "state", POWER_OBJ_PATH,
                                   service, bus, state);

        if (state)
        {
            powerOn = true;
            log<level::INFO>("power is on");
        }
        else
        {
            powerOn = false;
            log<level::INFO>("power is off");
        }
    }
    catch (std::exception& e)
    {
        log<level::INFO>("Failed to get power state. Assuming it is off.");
        powerOn = false;
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
        state =
            sdbusplus::message::variant_ns::get<int32_t>(valPropMap->second);

        // Power is on when state=1. Clear faults.
        if (state)
        {
            powerOn = true;
            log<level::INFO>("PSUManager::powerStateChanged power is on");
            clearFaults();
        }
        else
        {
            powerOn = false;
            log<level::INFO>("PSUManager::powerStateChanged power is off");
        }
    }
}

} // namespace manager
} // namespace power
} // namespace phosphor
