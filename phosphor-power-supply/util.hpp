#pragma once
#include "util_base.hpp"
#include "utility.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/format.h>

#include <gpiod.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>

#include <bitset>
#include <chrono>

namespace phosphor::power::psu
{

using Property = std::string;
using Value = std::variant<bool, std::string>;
using PropertyMap = std::map<Property, Value>;
using Interface = std::string;
using InterfaceMap = std::map<Interface, PropertyMap>;
using Object = sdbusplus::message::object_path;
using ObjectMap = std::map<Object, InterfaceMap>;

class Util : public UtilBase
{
  public:
    bool getPresence(sdbusplus::bus_t& bus,
                     const std::string& invpath) const override
    {
        bool present = false;

        // Use getProperty utility function to get presence status.
        util::getProperty(INVENTORY_IFACE, PRESENT_PROP, invpath,
                          INVENTORY_MGR_IFACE, bus, present);

        return present;
    }

    void setPresence(sdbusplus::bus_t& bus, const std::string& invpath,
                     bool present, const std::string& name) const override
    {
        using namespace phosphor::logging;
        log<level::INFO>(fmt::format("Updating inventory present property. "
                                     "present:{} invpath:{} name:{}",
                                     present, invpath, name)
                             .c_str());

        using InternalFailure =
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
        PropertyMap invProp;

        invProp.emplace("Present", present);
        invProp.emplace("PrettyName", name);

        InterfaceMap invIntf;
        invIntf.emplace("xyz.openbmc_project.Inventory.Item",
                        std::move(invProp));

        Interface extraIface = "xyz.openbmc_project.Inventory.Item.PowerSupply";

        invIntf.emplace(extraIface, PropertyMap());

        ObjectMap invObj;
        invObj.emplace(std::move(invpath), std::move(invIntf));

        try
        {
            auto invService = phosphor::power::util::getService(
                INVENTORY_OBJ_PATH, INVENTORY_MGR_IFACE, bus);

            // Update inventory
            auto invMsg = bus.new_method_call(invService.c_str(),
                                              INVENTORY_OBJ_PATH,
                                              INVENTORY_MGR_IFACE, "Notify");
            invMsg.append(std::move(invObj));
            auto invMgrResponseMsg = bus.call(invMsg);
        }
        catch (const std::exception& e)
        {
            log<level::ERR>(
                fmt::format(
                    "Error in inventory manager call to update inventory: {}",
                    e.what())
                    .c_str());
            elog<InternalFailure>();
        }
    }

    void setAvailable(sdbusplus::bus_t& bus, const std::string& invpath,
                      bool available) const override
    {
        PropertyMap invProp;
        InterfaceMap invIntf;
        ObjectMap invObj;

        invProp.emplace(AVAILABLE_PROP, available);
        invIntf.emplace(AVAILABILITY_IFACE, std::move(invProp));

        invObj.emplace(std::move(invpath), std::move(invIntf));

        try
        {
            auto invService = phosphor::power::util::getService(
                INVENTORY_OBJ_PATH, INVENTORY_MGR_IFACE, bus);

            auto invMsg = bus.new_method_call(invService.c_str(),
                                              INVENTORY_OBJ_PATH,
                                              INVENTORY_MGR_IFACE, "Notify");
            invMsg.append(std::move(invObj));
            auto invMgrResponseMsg = bus.call(invMsg);
        }
        catch (const sdbusplus::exception_t& e)
        {
            using namespace phosphor::logging;
            log<level::ERR>(
                fmt::format("Error in inventory manager call to update "
                            "availability interface: {}",
                            e.what())
                    .c_str());
            throw;
        }
    }

    void handleChassisHealthRollup(sdbusplus::bus_t& bus,
                                   const std::string& invpath,
                                   bool addRollup) const override
    {
        using AssociationTuple =
            std::tuple<std::string, std::string, std::string>;
        using AssociationsProperty = std::vector<AssociationTuple>;
        try
        {
            auto chassisPath = getChassis(bus, invpath);

            auto service = phosphor::power::util::getService(
                invpath, ASSOC_DEF_IFACE, bus);

            AssociationsProperty associations;
            phosphor::power::util::getProperty<AssociationsProperty>(
                ASSOC_DEF_IFACE, ASSOC_PROP, invpath, service, bus,
                associations);

            AssociationTuple critAssociation{"health_rollup", "critical",
                                             chassisPath};

            auto assocIt = std::find(associations.begin(), associations.end(),
                                     critAssociation);
            if (addRollup)
            {
                if (assocIt != associations.end())
                {
                    // It's already there
                    return;
                }

                associations.push_back(critAssociation);
            }
            else
            {
                if (assocIt == associations.end())
                {
                    // It's already been removed.
                    return;
                }

                // If the object still isn't functional, then don't clear
                // the association.
                bool functional = false;
                phosphor::power::util::getProperty<bool>(
                    OPERATIONAL_STATE_IFACE, FUNCTIONAL_PROP, invpath, service,
                    bus, functional);

                if (!functional)
                {
                    return;
                }

                associations.erase(assocIt);
            }

            phosphor::power::util::setProperty(ASSOC_DEF_IFACE, ASSOC_PROP,
                                               invpath, service, bus,
                                               associations);
        }
        catch (const sdbusplus::exception_t& e)
        {
            using namespace phosphor::logging;
            log<level::INFO>(fmt::format("Error trying to handle health rollup "
                                         "associations for {}: {}",
                                         invpath, e.what())
                                 .c_str());
        }
    }

    std::string getChassis(sdbusplus::bus_t& bus,
                           const std::string& invpath) const override
    {
        sdbusplus::message::object_path assocPath = invpath + "/powering";
        sdbusplus::message::object_path basePath{"/"};
        std::vector<std::string> interfaces{CHASSIS_IFACE};

        // Find the object path that implements the chassis interface
        // and also shows up in the endpoints list of the powering assoc.
        auto chassisPaths = phosphor::power::util::getAssociatedSubTreePaths(
            bus, assocPath, basePath, interfaces, 0);

        if (chassisPaths.empty())
        {
            throw std::runtime_error(fmt::format(
                "No association to a chassis found for {}", invpath));
        }

        return chassisPaths[0];
    }
};

std::unique_ptr<GPIOInterfaceBase> createGPIO(const std::string& namedGpio);

class GPIOInterface : public GPIOInterfaceBase
{
  public:
    GPIOInterface() = delete;
    virtual ~GPIOInterface() = default;
    GPIOInterface(const GPIOInterface&) = default;
    GPIOInterface& operator=(const GPIOInterface&) = default;
    GPIOInterface(GPIOInterface&&) = default;
    GPIOInterface& operator=(GPIOInterface&&) = default;

    /**
     * Constructor
     *
     * @param[in] namedGpio - The string for the gpio-line-name
     */
    GPIOInterface(const std::string& namedGpio);

    static std::unique_ptr<GPIOInterfaceBase>
        createGPIO(const std::string& namedGpio);

    /**
     * @brief Attempts to read the state of the GPIO line.
     *
     * Throws an exception if line not found, request line fails, or get_value
     * from line fails.
     *
     * @return 1 for active (low/present), 0 for not active (high/not present).
     */
    int read() override;

    /**
     * @brief Attempts to set the state of the GPIO line to the specified value.
     *
     * Throws an exception if line not found, request line fails, or set_value
     * to line fails.
     *
     * @param[in] value - The value to set the state of the GPIO line, 1 or 0.
     * @param[in] flags - Additional line request flags as defined in gpiod.hpp.
     */
    void write(int value, std::bitset<32> flags) override;

    /**
     * @brief Attempts to toggle (write) a GPIO low then high.
     *
     * Relies on write, so throws exception if line not found, etc.
     *
     * @param[in] delay - Milliseconds to delay betwen low/high toggle.
     */
    void toggleLowHigh(const std::chrono::milliseconds& delay) override;

    /**
     * @brief Returns the name of the GPIO, if not empty.
     */
    std::string getName() const override;

  private:
    gpiod::line line;
};

} // namespace phosphor::power::psu
