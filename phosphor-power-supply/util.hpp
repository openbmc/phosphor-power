#pragma once

#include "util_base.hpp"
#include "utility.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/format.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>

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

    void setPresence(sdbusplus::bus::bus& bus, const std::string& invpath,
                     bool present, const std::string& name) const override
    {
        using InternalFailure =
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
        using Property = std::string;
        using Value = std::variant<bool, std::string>;
        // Association between property and its value
        using PropertyMap = std::map<Property, Value>;
        PropertyMap invProp;

        invProp.emplace("Present", present);
        invProp.emplace("PrettyName", name);

        using Interface = std::string;
        // Association between interface and the D-Bus property
        using InterfaceMap = std::map<Interface, PropertyMap>;
        InterfaceMap invIntf;
        invIntf.emplace("xyz.openbmc_project.Inventory.Item",
                        std::move(invProp));

        Interface extraIface = "xyz.openbmc_project.Inventory.Item.PowerSupply";

        invIntf.emplace(extraIface, PropertyMap());

        using Object = sdbusplus::message::object_path;
        // Association between object and the interface
        using ObjectMap = std::map<Object, InterfaceMap>;
        ObjectMap invObj;
        invObj.emplace(std::move(invpath), std::move(invIntf));

        using namespace phosphor::logging;
        log<level::INFO>(fmt::format("Updating inventory present property. "
                                     "present:{} invpath:{} name:{}",
                                     present, invpath, name)
                             .c_str());

        auto invService = phosphor::power::util::getService(
            INVENTORY_OBJ_PATH, INVENTORY_MGR_IFACE, bus);

        // Update inventory
        auto invMsg =
            bus.new_method_call(invService.c_str(), INVENTORY_OBJ_PATH,
                                INVENTORY_MGR_IFACE, "Notify");
        invMsg.append(std::move(invObj));
        auto invMgrResponseMsg = bus.call(invMsg);
        if (invMgrResponseMsg.is_method_error())
        {
            log<level::ERR>(
                "Error in inventory manager call to update inventory");
            elog<InternalFailure>();
        }
    }
};

} // namespace phosphor::power::psu
