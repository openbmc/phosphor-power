#pragma once

#include "power_supply.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

struct sys_properties
{
    int maxPowerSupplies;
};

using namespace phosphor::power::psu;
using namespace phosphor::logging;

namespace phosphor::power::manager
{

/**
 * @class PSUManager
 *
 * This class will create an object used to manage and monitor a list of power
 * supply devices.
 */
class PSUManager
{
  public:
    PSUManager() = delete;
    ~PSUManager() = default;
    PSUManager(const PSUManager&) = delete;
    PSUManager& operator=(const PSUManager&) = delete;
    PSUManager(PSUManager&&) = delete;
    PSUManager& operator=(PSUManager&&) = delete;

    /**
     * Constructor to read configuration from JSON file.
     *
     * @param[in] bus - D-Bus bus object
     * @param[in] e - event object
     * @param[in] configfile - string path to the configuration file
     */
    PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e,
               const std::string& configfile);

    /**
     * Constructor to read configuration from D-Bus.
     *
     * @param[in] bus - D-Bus bus object
     * @param[in] e - event object
     */
    PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e);

    /**
     * @brief Initialize the system properties and PowerSupply objects from
     *        the JSON config file.
     * @param[in] path - Path to the JSON config file
     */
    void getJSONProperties(const std::string& path);

    /**
     * Get PSU properties from D-Bus, use that to build array of power supply
     * objects.
     *
     * @param[in] bus - D-Bus object
     * @param[in] interface - Interface string to get PSU information from.
     * @param[in] psus - Array (vectory) of power supply objects to populate.
     */
    void getPSUProperties(sdbusplus::bus::bus& bus,
                          const std::string& interface,
                          std::vector<std::unique_ptr<PowerSupply>>& psus);

    /**
     * Initializes the manager.
     *
     * Get current BMC state, ...
     */
    void initialize()
    {
        // When state = 1, system is powered on
        int32_t state = 0;

        try
        {
            // Use getProperty utility function to get power state.
            util::getProperty<int32_t>(POWER_IFACE, "state", POWER_OBJ_PATH,
                                       powerService, bus, state);

            if (state)
            {
                powerOn = true;
            }
            else
            {
                powerOn = false;
            }
        }
        catch (std::exception& e)
        {
            log<level::INFO>("Failed to get power state. Assuming it is off.");
            powerOn = false;
        }

        onOffConfig(phosphor::pmbus::ON_OFF_CONFIG_CONTROL_PIN_ONLY);
        clearFaults();
        updateInventory();
    }

    /**
     * Starts the timer to start monitoring the list of devices.
     */
    int run()
    {
        return timer->get_event().loop();
    }

    /**
     * Write PMBus ON_OFF_CONFIG
     *
     * This function will be called to cause the PMBus device driver to send the
     * ON_OFF_CONFIG command. Takes one byte of data.
     */
    void onOffConfig(const uint8_t data)
    {
        for (auto& psu : psus)
        {
            psu->onOffConfig(data);
        }
    }

    /**
     * This function will be called in various situations in order to clear
     * any fault status bits that may have been set, in order to start over
     * with a clean state. Presence changes and power state changes will want
     * to clear any faults logged.
     */
    void clearFaults()
    {
        for (auto& psu : psus)
        {
            psu->clearFaults();
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
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        timer;

    /**
     * Create an error
     *
     * @param[in] faultName - 'name' message for the BMC error log entry
     * @param[in] additionalData - The AdditionalData property for the error
     */
    void createError(const std::string& faultName,
                     const std::map<std::string, std::string>& additionalData);

    /**
     * Analyze the status of each of the power supplies.
     *
     * Log errors for faults, when and where appropriate.
     */
    void analyze();

    /** @brief True if the power is on. */
    bool powerOn = false;

    /** @brief Used as part of subscribing to power on state changes*/
    std::string powerService;

    /** @brief Used to subscribe to D-Bus power on state changes */
    std::unique_ptr<sdbusplus::bus::match_t> powerOnMatch;

    /**
     * @brief Callback for power state property changes
     *
     * Process changes to the powered on state property for the system.
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
        for (auto& psu : psus)
        {
            psu->updateInventory();
        }
    }

    /**
     * @brief The system properties.
     */
    sys_properties sysProperties;

    /**
     * @brief The vector for power supplies.
     */
    std::vector<std::unique_ptr<PowerSupply>> psus;
};

} // namespace phosphor::power::manager
