#pragma once

#include "util_base.hpp"
#include "utility.hpp"

namespace phosphor::power::psu
{

class Util : public UtilBase
{
  public:
    //~Util(){};
    bool getPresence(sdbusplus::bus::bus& bus,
                     const std::string& invpath) const override
    {
        bool present = false;

        // Use getProperty utility function to get presence status.
        util::getProperty(INVENTORY_IFACE, PRESENT_PROP, invpath,
                          INVENTORY_MGR_IFACE, bus, present);

        return present;
    }

};

} // namespace phosphor::power::psu
