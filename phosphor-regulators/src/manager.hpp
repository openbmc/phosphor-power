#pragma once

#include <interfaces/manager_interface.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace phosphor::power::regulators
{

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto objPath = "/xyz/openbmc_project/power/regulators/manager";

using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

using ManagerObject = sdbusplus::server::object::object<
    phosphor::power::regulators::interface::ManagerInterface>;

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
     * Creates a manager over the regulators.
     *
     * @param[in] bus - the dbus bus
     * @param[in] event - the sdevent event
     */
    Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /**
     * @brief Overridden manager object's configure method
     */
    void configure() override;

    /**
     * @brief Overridden manager object's monitor method
     *
     * @param[in] enable - Enable or disable regulator monitoring
     */
    void monitor(bool enable) override;

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

} // namespace phosphor::power::regulators
