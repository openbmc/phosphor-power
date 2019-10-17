#include "psu_manager.hpp"

#include "utility.hpp"

namespace phosphor
{
namespace power
{
namespace manager
{

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
