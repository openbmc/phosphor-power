#pragma once

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/Power/Regulators/Manager/server.hpp>

namespace phosphor
{
namespace power
{
namespace regulators
{

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto objPath = "/xyz/openbmc_project/power/regulators/manager";

using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

using ManagerObject = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Power::Regulators::server::Manager>;

class Manager : public ManagerObject
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /**
     * Constructor
     * Creates a manager of the handling regulators.
     *
     * @param[in] bus - the dbus bus
     * @param[in] event - the sdevent event
     */
    Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /**
     * @brief Overridden manager object's configure method
     */
    virtual void configure();

    /**
     * @brief Overridden manager object's monitor method
     *
     * @param[in] enable - Enable or disable regulator monitoring
     */
    virtual void monitor(bool enable);

    /**
     * @brief Timer expired callback function
     */
    void timerExpired();

  private:
    /**
     * The dbus bus
     */
    sdbusplus::bus::bus& bus;

    /**
     * Event to loop on
     */
    sdeventplus::Event eventLoop;

    /**
     * List of event timers
     */
    std::vector<Timer> timers;
};

} // namespace regulators
} // namespace power
} // namespace phosphor
