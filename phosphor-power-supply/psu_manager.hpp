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
    int powerSupplyCount;
    std::vector<uint64_t> inputVoltage;
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
     * Constructor to read configuration from D-Bus.
     *
     * @param[in] bus - D-Bus bus object
     * @param[in] e - event object
     */
    PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e);

    /**
     * Get PSU properties from D-Bus, use that to build a power supply
     * object.
     *
     * @param[in] properties - A map of property names and values
     *
     */
    void getPSUProperties(util::DbusPropertyMap& properties);

    /**
     * Get PSU configuration from D-Bus
     */
    void getPSUConfiguration();

    /**
     * @brief Initialize the system properties from the Supported Configuration
     *        D-Bus object provided by Entity Manager.
     */
    void getSystemProperties();

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
                validateConfig();
            }
            else
            {
                powerOn = false;
                runValidateConfig = true;
            }
        }
        catch (std::exception& e)
        {
            log<level::INFO>("Failed to get power state. Assuming it is off.");
            powerOn = false;
            runValidateConfig = true;
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

    /** @brief Used to subscribe to Entity Manager interfaces added */
    std::unique_ptr<sdbusplus::bus::match_t> entityManagerIfacesAddedMatch;

    /**
     * @brief Callback for power state property changes
     *
     * Process changes to the powered on state property for the system.
     *
     * @param[in] msg - Data associated with the power state signal
     */
    void powerStateChanged(sdbusplus::message::message& msg);

    /**
     * @brief Callback for entity-manager interface added
     *
     * Process the information from the supported configuration and or IBM CFFPS
     * Connector interface being added.
     *
     * @param[in] msg - Data associated with the interfaces added signal
     */
    void entityManagerIfaceAdded(sdbusplus::message::message& msg);

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
     * @brief Helper function to populate the system properties
     *
     * @param[in] properties - A map of property names and values
     */
    void populateSysProperties(const util::DbusPropertyMap& properties);

    /**
     * @brief Perform power supply configuration validation.
     * @details Validates if the existing power supply properties are a
     * supported configuration, and acts on its findings such as logging errors.
     */
    void validateConfig();

    /**
     * @brief Flag to indicate if the validateConfig() function should be run.
     * Set to false once the configuration has been validated to avoid running
     * multiple times due to interfaces added signal. Set to true during power
     * off to trigger the validation on power on.
     */
    bool runValidateConfig = true;

    /**
     * @brief Map of supported PSU configurations that include the model name
     * and their properties.
     */
    std::map<std::string, sys_properties> supportedConfigs;

    /**
     * @brief The vector for power supplies.
     */
    std::vector<std::unique_ptr<PowerSupply>> psus;
};

} // namespace phosphor::power::manager
