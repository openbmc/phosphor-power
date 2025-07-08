#pragma once

#include "getChassis.hpp"
#include "power_supply.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/Decorator/PowerSystemInputs/server.hpp>

#include <unordered_set>

using namespace phosphor::power::psu;
using namespace phosphor::logging;

namespace phosphor::power::chassis_manager
{

using PowerSystemInputsInterface = sdbusplus::xyz::openbmc_project::State::
    Decorator::server::PowerSystemInputs;
using PowerSystemInputsObject =
    sdbusplus::server::object_t<PowerSystemInputsInterface>;

// Validation timeout. Allow 30s to detect if new EM interfaces show up in D-Bus
// before performing the validation.
constexpr auto validationTimeout = std::chrono::seconds(30);

/**
 * @class ChassisManager
 *
 * @brief Manages and monitors power supply devices for the chassis.
 *
 * @detail This class interacts with D-Bus to detect chassis power supply,
 * subscribe to Entity Manager interface changes.
 */
class ChassisManager
{
  public:
    ChassisManager() = delete;
    ~ChassisManager() = default;
    ChassisManager(const ChassisManager&) = delete;
    ChassisManager& operator=(const ChassisManager&) = delete;
    ChassisManager(ChassisManager&&) = delete;
    ChassisManager& operator=(ChassisManager&&) = delete;

    /**
     * @brief Constructs a ChassisManager instance.
     *
     * @details Sets up D-Bus interfaces, creates timer for power supply
     * validation and monitoring, and subscribes to entity-manager interfaces.
     *
     * @param[in] bus - Reference to the system D-Bus object.
     * @param[in] e - Reference to event loop.
     */
    ChassisManager(sdbusplus::bus_t& bus, const sdeventplus::Event& e);

    /**
     * @brief Starts the main event loop for monitoring.
     *
     * @return int Returns the result the result of the event loop execution.
     */
    int run()
    {
        return timer->get_event().loop();
    }

  private:
    /**
     * @brief The D-Bus object
     */
    sdbusplus::bus_t& bus;

    /**
     * @brief The timer that runs to periodically check the power supplies.
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        timer;

    /**
     * @brief The timer that performs power supply validation as the entity
     * manager interfaces show up in d-bus.
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        validationTimer;

    /**
     * @brief Used to subscribe to Entity Manager interfaces added
     */
    std::unique_ptr<sdbusplus::bus::match_t> entityManagerIfacesAddedMatch;

    /**
     * @brief Implement the ObjectManager for PowerSystemInputs object.
     *
     * @details Implements the org.freedesktop.DBus.ObjectManager
     * interface used to communicate updates to the PowerSystemInputs
     * object on the /xyz/openbmc_project/power/power_supplies root
     * D-Bus path.
     */
    sdbusplus::server::manager_t objectManager;

    /**
     * @brief List of chassis objects populated dynamically.
     */
    std::vector<std::unique_ptr<phosphor::power::chassis::Chassis>> chassisList;

    /**
     * @brief Declares a constant reference to an sdeventplus::Envent to manage
     * async processing.
     */
    const sdeventplus::Event& eventLoop;

    /**
     * @brief Tracks known chassis paths to prevent duplication
     */
    std::unordered_set<std::string> knownChassisPaths;

    /**
     * @brief Callback for entity-manager interface added
     *
     * @details Process the information from the supported configuration and
     * or IBM CFFPS Connector interface being added.
     *
     * @param[in] msg - D-Bus message containing the interface details.
     */
    void entityManagerIfaceAdded(sdbusplus::message_t& msg);

    /**
     * @brief Invoke the PSU analysis method in each chassis on the system.
     *
     * @details Scans each chassis and analyze chassis power supplies and log
     * any detected errors.
     */
    void analyze();

    /**
     * @brief Initialize the list of chassis object from the inventory, scans
     * the D-Bus subtree for chassis and creates Chassis instances.
     */
    void initializeChassisList();

    /**
     * @brief Checks whether the given object path exist in the chassis list. If
     * not, creates a new chassis instance.
     */
    void addChassisIfNew(const std::string& path);
};

} // namespace phosphor::power::chassis_manager
