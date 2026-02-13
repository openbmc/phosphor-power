#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/vtable.hpp>

#include <string>

namespace phosphor
{
namespace power
{
namespace chassis
{
namespace interface
{

class ManagerInterface
{
  public:
    ManagerInterface() = delete;
    ManagerInterface(const ManagerInterface&) = delete;
    ManagerInterface& operator=(const ManagerInterface&) = delete;
    ManagerInterface(ManagerInterface&&) = delete;
    ManagerInterface& operator=(ManagerInterface&&) = delete;
    virtual ~ManagerInterface() = default;
    /**
     * @brief Constructor to put object onto bus at a dbus path.
     * @param[in] bus - Bus to attach to.
     * @param[in] path - Path to attach at.
     */
    ManagerInterface(sdbusplus::bus_t& bus, const char* path);

    /**
     * @brief Implementation for the configure method
     * Request to configure the chassis according to the
     * machine's chassis configuration file.
     */
    virtual void configure() = 0;
    /**
     * @brief This dbus interface's name
     */
    static constexpr auto interface =
        "xyz.openbmc_project.Power.Chassis.Manager";

  private:
    /**
     * @brief Systemd bus callback for the configure method
     */
    static int callbackConfigure(sd_bus_message* msg, void* context,
                                 sd_bus_error* error);
    /**
     * @brief Systemd vtable structure that contains all the
     * methods, signals, and properties of this interface with their
     * respective systemd attributes
     */
    static const sdbusplus::vtable::vtable_t _vtable[];

    /**
     * @brief Holder for the instance of this interface to be
     * on dbus
     */
    sdbusplus::server::interface::interface _serverInterface;
};
} // namespace interface
} // namespace chassis
} // namespace power
} // namespace phosphor
