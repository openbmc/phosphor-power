#pragma once

#include "chassis.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/Decorator/PowerSystemInputs/server.hpp>

using namespace phosphor::power::psu;

namespace phosphor::power::chassis_manager
{
using Association = std::tuple<std::string, std::string, std::string>;
using Associations = std::vector<Association>;

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
     * @brief Used to subscribe to Entity Manager interfaces added
     */
    std::unique_ptr<sdbusplus::match> entityManagerIfacesAddedMatch;

    /**
     * @brief List of chassis objects populated dynamically.
     */
    std::vector<std::unique_ptr<phosphor::power::chassis::Chassis>>
        listOfChassis;

    /**
     * @brief Declares a constant reference to an sdeventplus::Envent to manage
     * async processing.
     */
    const sdeventplus::Event& eventLoop;

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
     * @details Scan the system for chassis and analyze each chassis power
     * supplies and log any detected errors.
     */
    void analyze();

    /**
     * @brief Initialize the list of chassis object from the inventory, scans
     * the D-Bus subtree for chassis and creates Chassis instances.
     */
    void initializeChassisList();

    /**
     * @brief Retrieves a pointer to a Chassis object matching the given ID.
     *
     * @param[in] chassisPositionId - Unique identifier of the chassis to search
     * for.
     * @return Raw pointer to the matching Chassis object if found, otherwise a
     * nullptr.
     */
    phosphor::power::chassis::Chassis* getMatchingChassisPtr(
        uint64_t chassisPositonId);

    /**
     * @brief Initialize chassis power monitoring.
     */
    void initChassisPowerMonitoring();

    /**
     * @brief True if multi chassis system
     */
    bool isMultiChassisSystem;

    /**
     * @brief Extract the chassis endpoint path from a collection of D-Bus
     * associations.
     *
     * @details Searchs through provided associations to find the first
     * endpoint path that contains "chassis".
     *
     * @param[in] associations - A collection of association tuples, where each
     * tuple contains: forward, reverse, and endpoint
     *
     * @return -  The chassis path, or an empty if no chassis association
     * exists
     */
    std::string getChassisAssociation(const Associations& associations);

    /**
     * @brief Retrieves D-Bus associations for a given inventory object path
     *
     * @details Queries the D-Bus association interface to obtain all
     * associations related to the specified inventory path.
     *
     * @param[in] bus - Reference to the system D-Bus object.
     * @param[in] inventoryPath - The D-Bus object path of the inventory item to
     * query
     *
     * @return Associations - A collection of association tuples contains:
     * forward, reverse, and endpoint. An empty collection if no service is
     * found.
     */
    Associations getAssociations(sdbusplus::bus_t& bus,
                                 const std::string& inventoryPath);

    /**
     * @brief Creates and adds a new Chassis object to the managed chassis list
     *
     * @details Constructs a Chassis object from the provided D-Bus path and
     * adds it to the internal list of managed chassis.
     *
     * @param[in] chassisPath - The full D-Bus object path of the chassis to add
     */
    void addChassisToList(const std::string& chassisPath);
};

} // namespace phosphor::power::chassis_manager
