#pragma once

#include <interfaces/manager.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <algorithm>

namespace phosphor
{
namespace power
{
namespace regulators
{

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto objPath = "/xyz/openbmc_project/power/regulators/manager";
constexpr auto sysDbusObj = "/xyz/openbmc_project/inventory";
constexpr auto sysDbusPath = "/xyz/openbmc_project/inventory/system";
constexpr auto sysDbusIntf = "xyz.openbmc_project.Inventory.Item.System";
constexpr auto sysDbusProp = "Identifier";

using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

using ManagerObject = sdbusplus::server::object::object<
    phosphor::power::regulators::interface::Manager>;

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

    /**
     * @brief Callback function to handle receiving a HUP signal
     * to reload the configuration data.
     *
     * @param[in] sigSrc - sd_event_source signal wrapper
     * @param[in] sigInfo - signal info on signal fd
     */
    void sighupHandler(sdeventplus::source::Signal& sigSrc,
                       const struct signalfd_siginfo* sigInfo);

    /**
     * @brief Callback function to handle interfacesAdded dbus signals
     *
     * @param[in] msg - Expanded sdbusplus message data
     */
    void signalHandler(sdbusplus::message::message& msg);

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

    /**
     * List of dbus signal matches
     */
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> signals;

    /**
     * JSON configuration data filename
     */
    std::string fileName;

    /**
     * @brief Set the JSON configuration data filename
     *
     * @param[in] fName = filename without `.json` extension
     */
    inline void setFileName(const std::string& fName)
    {
        fileName = fName;
        if (!fileName.empty())
        {
            // Replace all spaces with underscores
            std::replace(fileName.begin(), fileName.end(), ' ', '_');
            fileName.append(".json");
        }
    };

    /**
     * @brief Get the JSON configuration data filename from dbus
     *
     * @return - JSON configuration data filename
     */
    const std::string getFileNameDbus();
};

} // namespace regulators
} // namespace power
} // namespace phosphor
