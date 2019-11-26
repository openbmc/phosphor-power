#include "power_supply.hpp"

#include "types.hpp"
#include "utility.hpp"

namespace phosphor
{
namespace power
{
namespace psu
{

void PowerSupply::updatePresence()
{
    // Use getProperty utility function to get presence status.
    util::getProperty(INVENTORY_IFACE, PRESENT_PROP, inventoryPath,
                      INVENTORY_MGR_IFACE, bus, this->present);
}

void PowerSupply::inventoryChanged(sdbusplus::message::message& msg)
{
    std::string msgSensor;
    std::map<std::string, sdbusplus::message::variant<uint32_t, bool>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find(PRESENT_PROP);
    if (valPropMap != msgData.end())
    {
        if (std::get<bool>(valPropMap->second))
        {
            clearFaults();
        }
        else
        {
            present = false;

            // Clear out the now outdated inventory properties
            updateInventory();
        }
    }
}

} // namespace psu
} // namespace power
} // namespace phosphor
