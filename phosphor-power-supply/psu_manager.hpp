#pragma once

#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <phosphor-logging/log.hpp>

#include "power_supply.hpp"

struct sys_properties
{
    int minPowerSupplies;
    int maxPowerSupplies;
};

using namespace phosphor::power::psu;
using namespace phosphor::logging;

namespace phosphor
{
namespace power
{
namespace manager
{

constexpr auto POWER_OBJ_PATH = "/org/openbmc/control/power0";
constexpr auto POWER_IFACE = "org.openbmc.control.Power";

/**
 * @class psuManager
 *
 * This class will create an object used to manage and monitor a list of power
 * supply devices.
 */
class psuManager
{
  public:
    psuManager() = delete;
    ~psuManager() = default;
    psuManager(const psuManager&) = delete;
    psuManager& operator=(const psuManager&) = delete;
    psuManager(psuManager&&) = default;
    psuManager& operator=(psuManager&&) = default;

    /**
     * Constructor
     *
     * @param[in] e - event object
     * @param[in] i - polling interval in milliseconds
     */
    psuManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e,
               std::chrono::milliseconds i, struct sys_properties& p) :
        bus(bus),
        timer(e, std::bind(&psuManager::analyze, this), i)
    {

        minPSUs = p.minPowerSupplies;
        minPSUs = p.maxPowerSupplies;

        initialize();

        // Subscribe to power state changes
        powerOnMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus, sdbusplus::bus::match::rules::propertiesChanged(
                POWER_OBJ_PATH, POWER_IFACE),
                [this](auto& msg) { this->powerStateChanged(msg); });
        // Get initial power state.
        updatePowerState();
    }

    /**
     * Initializes the manager.
     *
     * Get current BMC state, ...
     */
    void initialize()
    {
        clearFaults();
        updateInventory();
    }

    /**
     * Starts the timer to start monitoring the list of devices.
     */
    int run()
    {
        return timer.get_event().loop();
    }

    /**
     * This function will be called in various situations in order to clear
     * any fault status bits that may have been set, in order to start over
     * with a clean state. Presence changes and power state changes will want
     * to clear any faults logged.
     */
    void clearFaults()
    {
        log<level::INFO>("clearFaults");
        for (auto psu = psus.begin(); psu != psus.end(); psu++)
        {
            psu->clearFaults();
        }
    }

  protected :
    /**
    * Analyze the status of each of the power supplies.
    */
    void analyze()
    {
        for (auto psu = psus.begin(); psu != psus.end(); psu++)
        {
            psu->analyze();
        }
    }

  private:
    /**
     * The D-Bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * The timer that runs to periodically check the power supplies.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> timer;

    /** @brief True if the power is on. */
    bool powerOn = false;

    /** @brief Used to subscribe to D-Bus power on state changes */
    std::unique_ptr<sdbusplus::bus::match_t> powerOnMatch;

    /**
     * @brief Updates the poweredOn status by querying D-Bus
     *
     * The D-Bus property for the system power state will be read to determine
     * if the system is powered on or not.
     */
    void updatePowerState();

    /**
     * @brief Callback for power state property changes
     *
     * Process changes to the powered on stat property for the system.
     *
     * @param[in] msg - Data associated with the power state signal
     */
    void powerStateChanged(sdbusplus::message::message& msg);

    /**
     * @brief Adds properties to the inventory.
     *
     * Reads the values from the devices and writes them to the associated
     * power supply D-Bus inventory objects.
     *
     * This needs to be done on startup, and each time the presence state
     * changes.
     */
    void updateInventory()
    {
        //log<level::INFO>("updateInventory");
        for (auto psu = psus.begin(); psu != psus.end(); psu++)
        {
            psu->updateInventory();
        }
    }

    /**
     * @brief Minimum number of power supplies to operate.
     */
    int minPSUs = 1;

    /**
     * @brief Maximum number of power supplies possible.
     */
    int maxPSUs = 1;

    /**
     * @brief The list/array/vector for power supplies.
     */
     std::vector<PowerSupply> psus;
};

} // namespace manager
} // namespace power
} // namespace phosphor
