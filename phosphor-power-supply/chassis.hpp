#pragma once
#include "new_power_supply.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/Decorator/PowerSystemInputs/server.hpp>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

struct SupportedPsuConfiguration
{
    int powerSupplyCount;
    std::vector<uint64_t> inputVoltage;
    bool powerConfigFullLoad;
};

using namespace phosphor::power::psu;
using namespace phosphor::power::util;
using namespace sdeventplus;

namespace phosphor::power::chassis
{

constexpr uint64_t invalidObjectPathUniqueId = 9999;
using PowerSystemInputsInterface = sdbusplus::xyz::openbmc_project::State::
    Decorator::server::PowerSystemInputs;
using PowerSystemInputsObject =
    sdbusplus::server::object_t<PowerSystemInputsInterface>;

// Validation timeout. Allow 30s to detect if new EM interfaces show up in D-Bus
// before performing the validation.
// Previously the timer was set to 10 seconds was too short, it results in
// incorrect errors being logged, but no real consequence of longer timeout.
constexpr auto validationTimeout = std::chrono::seconds(30);

/**
 * @class PowerSystemInputs
 * @brief A concrete implementation for the PowerSystemInputs interface.
 */
class PowerSystemInputs : public PowerSystemInputsObject
{
  public:
    PowerSystemInputs(sdbusplus::bus_t& bus, const std::string& path) :
        PowerSystemInputsObject(bus, path.c_str())
    {}
};

/**
 * @class Chassis
 *
 * @brief This class will create an object used to manage and monitor a list of
 * power supply devices attached to the chassis.
 */
class Chassis
{
  public:
    Chassis() = delete;
    ~Chassis() = default;
    Chassis(const Chassis&) = delete;
    Chassis& operator=(const Chassis&) = delete;
    Chassis(Chassis&&) = delete;
    Chassis& operator=(Chassis&&) = delete;

    /**
     * @brief Constructor to read configuration from D-Bus.
     *
     * @param[in] bus - D-Bus bus object
     * @param[in] chassisPath - Chassis path
     * @param[in] event - Event loop object
     */
    Chassis(sdbusplus::bus_t& bus, const std::string& chassisPath,
            const sdeventplus::Event& e);

    /**
     * @brief Retrieves the unique identifier of the chassis.
     *
     * @return uint64_t The unique 64 bits identifier of the chassis.
     */
    uint64_t getChassisId()
    {
        return chassisPathUniqueId;
    }

    /**
     * @brief Analyze the status of each of the power supplies. Log errors for
     * faults, when and where appropriate.
     */
    void analyze();

    /**
     * @brief Get the status of Power on.
     */
    bool isPowerOn()
    {
        return powerOn;
    }

    /**
     * @brief Initialize power monitoring infrastructure for Chassis.
     * Sets up configuration validation timer, attempts to create GPIO,
     * subscribe to D-Bus power state change events.
     */
    void initPowerMonitoring();

    /**
     * @brief Handles addition of the SupportedConfiguration interface.
     * This function triggered when the SupportedConfiguration interface added
     * to a D-Bus object. The function  calls populateSupportedConfiguration()
     * and updateMissingPSUs() to processes the provided properties.
     *
     * @param properties A map of D-Bus properties associated with the
     * SupportedConfiguration interface.
     */
    void supportedConfigurationInterfaceAdded(
        const util::DbusPropertyMap& properties);

    /**
     * @brief Handle the addition of PSU interface.
     * This function is called when a Power Supply interface added to a D-Bus.
     * This function calls getPSUProperties() and updateMissingPSUs().
     *
     * @param properties A map of D-Bus properties for the PSU interface.
     */
    void psuInterfaceAdded(util::DbusPropertyMap& properties);

    /**
     * @brief Call to validate the psu configuration if the power is on and both
     * the IBMCFFPSConnector and SupportedConfiguration interfaces have been
     * processed
     */
    void validatePsuConfigAndInterfacesProcessed()
    {
        if (powerOn && !psus.empty() && !supportedConfigs.empty())
        {
            validationTimer->restartOnce(validationTimeout);
        }
    };

  private:
    /**
     * @brief The D-Bus object
     */
    sdbusplus::bus_t& bus;

    /**
     * @brief The timer that performs power supply validation as the entity
     * manager interfaces show up in d-bus.
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        validationTimer;

    /** @brief True if the power is on. */
    bool powerOn = false;

    /** @brief True if power control is in the window between chassis pgood loss
     * and power off.
     */
    bool powerFaultOccurring = false;

    /** @brief True if an error for a brownout has already been logged. */
    bool brownoutLogged = false;

    /** @brief Used as part of subscribing to power on state changes*/
    std::string powerService;

    /** @brief Used to subscribe to D-Bus power on state changes */
    std::unique_ptr<sdbusplus::bus::match_t> powerOnMatch;

    /** @brief Used to subscribe to D-Bus power supply presence changes */
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> presenceMatches;

    /**
     * @brief Flag to indicate if the validateConfig() function should be run.
     * Set to false once the configuration has been validated to avoid
     * running multiple times due to interfaces added signal. Set to
     * true during power off to trigger the validation on power on.
     */
    bool runValidateConfig = true;

    /**
     * @brief Map of supported PSU configurations that include the model name
     * and their properties.
     */
    std::map<std::string, SupportedPsuConfiguration> supportedConfigs;

    /**
     * @brief The vector for power supplies.
     */
    std::vector<std::unique_ptr<PowerSupply>> psus;

    /**
     * @brief The device driver name for all power supplies.
     */
    std::string driverName;

    /**
     * @brief The libgpiod object for setting the power supply config
     */
    std::unique_ptr<GPIOInterfaceBase> powerConfigGPIO = nullptr;

    /**
     * @brief Chassis D-Bus object path
     */
    std::string chassisPath;

    /**
     * @brief Chassis name;
     */
    std::string chassisShortName;

    /**
     * @brief The Chassis path unique ID
     *
     * Note: chassisPathUniqueId must be declared before powerSystemInputs.
     */
    uint64_t chassisPathUniqueId = invalidObjectPathUniqueId;

    /**
     * @brief PowerSystemInputs object
     */
    PowerSystemInputs powerSystemInputs;

    /**
     * @brief Declares a constant reference to an sdeventplus::Event to manage
     * async processing.
     */
    const sdeventplus::Event& eventLoop;

    /**
     * @brief GPIO to toggle to 'sync' power supply input history.
     */
    std::unique_ptr<GPIOInterfaceBase> syncHistoryGPIO = nullptr;

    /**
     * @brief Get PSU properties from D-Bus, use that to build a power supply
     * object.
     *
     * @param[in] properties - A map of property names and values
     */
    void getPSUProperties(util::DbusPropertyMap& properties);

    /**
     * @brief Get PSU configuration from D-Bus
     */
    void getPSUConfiguration();

    /**
     * @brief Queries D-Bus for chassis configuration provided by the Entity
     * Manager. Matches the object against the current chassis unique ID. Upon
     * finding a match calls populateSupportedConfiguration().
     */
    void getSupportedConfiguration();

    /**
     * @brief Callback for inventory property changes
     *
     * Process change of the Power Supply presence.
     *
     * @param[in]  msg - Data associated with the Present change signal
     **/
    void psuPresenceChanged(sdbusplus::message_t& msg);

    /**
     * @brief Helper function to populate the PSU supported configuration
     *
     * @param[in] properties - A map of property names and values
     */
    void populateSupportedConfiguration(
        const util::DbusPropertyMap& properties);

    /**
     * @brief Build the device driver name for the power supply.
     *
     * @param[in] i2cbus - i2c bus
     * @param[in] i2caddr - i2c bus address
     */
    void buildDriverName(uint64_t i2cbus, uint64_t i2caddr);

    /**
     * @brief Find PSU with device driver name, then populate the device
     * driver name to all PSUs (including missing PSUs).
     */
    void populateDriverName();

    /**
     * @brief Get chassis path unique ID.
     *
     * @param [in] path - Chassis path.
     * @return uint64_t - Chassis path unique ID.
     */
    uint64_t getChassisPathUniqueId(const std::string& path);

    /**
     * @brief Initializes the chassis.
     *
     */
    void initialize();

    /**
     * @brief Perform power supply configuration validation.
     * @details Validates if the existing power supply properties are a
     * supported configuration, and acts on its findings such as logging
     * errors.
     */
    void validateConfig();

    /**
     * @brief Analyze the set of the power supplies for a brownout failure. Log
     * error when necessary, clear brownout condition when window has passed.
     */
    void analyzeBrownout();

    /**
     * @brief Toggles the GPIO to sync power supply input history readings
     * @details This GPIO is connected to all supplies.  This will clear the
     * previous readings out of the supplies and restart them both at the
     * same time zero and at record ID 0.  The supplies will return 0
     * bytes of data for the input history command right after this until
     * a new entry shows up.
     *
     * This will cause the code to delete all previous history data and
     * start fresh.
     */
    void syncHistory();

    /**
     * @brief Tells each PSU to set its power supply input
     * voltage rating D-Bus property.
     */
    inline void setInputVoltageRating()
    {
        for (auto& psu : psus)
        {
            psu->setInputVoltageRating();
        }
    }

    /**
     * Create an error
     *
     * @param[in] faultName - 'name' message for the BMC error log entry
     * @param[in,out] additionalData - The AdditionalData property for the error
     */
    void createError(const std::string& faultName,
                     std::map<std::string, std::string>& additionalData);

    /**
     * @brief Check that all PSUs have the same model name and that the system
     * has the required number of PSUs present as specified in the Supported
     * Configuration interface.
     *
     * @param[out] additionalData - Contains debug information on why the check
     *             might have failed. Can be used to fill in error logs.
     * @return true if all the required PSUs are present, false otherwise.
     */
    bool hasRequiredPSUs(std::map<std::string, std::string>& additionalData);

    /**
     * @brief Update inventory for missing required power supplies
     */
    void updateMissingPSUs();

    /**
     * @brief Assign chassis short name.
     */
    void saveChassisName()
    {
        std::filesystem::path path(chassisPath);
        chassisShortName = path.filename();
    }

    /**
     * @brief Callback for power state property changes
     *
     * Process changes to the powered on state property for the chassis.
     *
     * @param[in] msg - Data associated with the power state signal
     */
    void powerStateChanged(sdbusplus::message_t& msg);

    /**
     * @breif Attempt to create GPIO
     */
    void attemptToCreatePowerConfigGPIO();

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
        setPowerSupplyError("");
        for (auto& psu : psus)
        {
            psu->clearFaults();
        }
    }

    /**
     * Let power control/sequencer application know of PSU error(s).
     *
     * @param[in] psuErrorString - string for power supply error
     */
    void setPowerSupplyError(const std::string& psuErrorString);

    /**
     * @brief Set the power-config-full-load GPIO depending on the EM full load
     *        property value.
     */
    void setPowerConfigGPIO();

    /**
     * @brief Helper function to validate that all PSUs have the same model name
     *
     * @param[out] model - The model name. Empty if there is a mismatch.
     * @param[out] additionalData - If there is a mismatch, it contains debug
     *             information such as the mismatched model name.
     * @return true if all the PSUs have the same model name, false otherwise.
     */
    bool validateModelName(std::string& model,
                           std::map<std::string, std::string>& additionalData);

    /**
     * @brief Returns the number of PSUs that are required to be present.
     */
    unsigned int getRequiredPSUCount()
    {
        // TODO
        return 1;
    }

    /**
     * @brief Returns whether the specified PSU is required to be present.
     *
     * @param[in] psu - Power supply to check
     * @return true if PSU is required, false otherwise.
     */
    bool isRequiredPSU(const PowerSupply& psu);
};

} // namespace phosphor::power::chassis
