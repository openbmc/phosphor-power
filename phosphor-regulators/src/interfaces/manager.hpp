#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>

#include <string>

namespace phosphor
{
namespace power
{
namespace regulators
{
namespace server
{

class Manager
{
  public:
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *         - Move operations due to 'this' being registered as the
     *           'context' with sdbus.
     *     Allowed:
     *         - Destructor.
     */
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /**
     * @brief Constructor to put object onto bus at a dbus path.
     * @param[in] bus - Bus to attach to.
     * @param[in] path - Path to attach at.
     */
    Manager(sdbusplus::bus::bus& bus, const char* path);

    /**
     * @brief Implementation for Configure
     * Request to configure the regulators according to the
     * machine's regulators configuration json.
     */
    virtual void configure() = 0;

    /**
     * @brief Implementation for Monitor
     * Begin to monitor the regulators according to the
     * machine's regulators configuration json.
     *
     * @param[in] enable - Enable or disable monitoring of the regulators.
     */
    virtual void monitor(bool enable) = 0;

    /**
     * @brief Emit interface added
     */
    void emit_added()
    {
        _serverInterface.emit_added();
    }

    /**
     * @brief Emit interface removed
     */
    void emit_removed()
    {
        _serverInterface.emit_removed();
    }

    static constexpr auto interface =
        "xyz.openbmc_project.Power.Regulators.Manager";

  private:
    /**
     * @brief sd-bus callback for Configure method
     */
    static int _callbackConfigure(sd_bus_message* msg, void* context,
                                  sd_bus_error* error);

    /**
     * @brief sd-bus callback for Monitor method
     */
    static int _callbackMonitor(sd_bus_message* msg, void* context,
                                sd_bus_error* error);

    static const sdbusplus::vtable::vtable_t _vtable[];

    sdbusplus::server::interface::interface _serverInterface;
};

} // namespace server
} // namespace regulators
} // namespace power
} // namespace phosphor
