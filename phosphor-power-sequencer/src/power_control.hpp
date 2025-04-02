#pragma once

#include "config.h"

#include "compatible_system_types_finder.hpp"
#include "device_finder.hpp"
#include "power_interface.hpp"
#include "power_sequencer_device.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <gpiod.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace phosphor::power::sequencer
{

using PowerObject = sdbusplus::server::object_t<PowerInterface>;

/**
 * @class PowerControl
 * This class implements GPIO control of power on / off, and monitoring of the
 * chassis power good.
 */
class PowerControl : public PowerObject
{
  public:
    PowerControl() = delete;
    PowerControl(const PowerControl&) = delete;
    PowerControl& operator=(const PowerControl&) = delete;
    PowerControl(PowerControl&&) = delete;
    PowerControl& operator=(PowerControl&&) = delete;
    ~PowerControl() = default;

    /**
     * Creates a controller object for power on and off.
     * @param bus D-Bus bus object
     * @param event event object
     */
    PowerControl(sdbusplus::bus_t& bus, const sdeventplus::Event& event);

    /** @copydoc PowerInterface::getPgood() */
    int getPgood() const override;

    /** @copydoc PowerInterface::getPgoodTimeout() */
    int getPgoodTimeout() const override;

    /** @copydoc PowerInterface::getState() */
    int getState() const override;

    /** @copydoc PowerInterface::setPgoodTimeout() */
    void setPgoodTimeout(int timeout) override;

    /** @copydoc PowerInterface::setState() */
    void setState(int state) override;

    /** @copydoc PowerInterface::setPowerSupplyError() */
    void setPowerSupplyError(const std::string& error) override;

    /**
     * Callback that is called when a list of compatible system types is found.
     *
     * @param types Compatible system types for the current system ordered from
     *              most to least specific
     */
    void compatibleSystemTypesFound(const std::vector<std::string>& types);

    /**
     * Callback that is called when a power sequencer device is found.
     *
     * @param properties Properties of device that was found
     */
    void deviceFound(const DeviceProperties& properties);

  private:
    /**
     * The D-Bus bus object
     */
    sdbusplus::bus_t& bus;

    /**
     * System services like hardware presence and the journal.
     */
    BMCServices services;

    /**
     * Object that finds the compatible system types for the current system.
     */
    std::unique_ptr<util::CompatibleSystemTypesFinder> compatSysTypesFinder;

    /**
     * Compatible system types for the current system ordered from most to least
     * specific.
     */
    std::vector<std::string> compatibleSystemTypes;

    /**
     * Object that finds the power sequencer device in the system.
     */
    std::unique_ptr<DeviceFinder> deviceFinder;

    /**
     * Power sequencer device properties.
     */
    std::optional<DeviceProperties> deviceProperties;

    /**
     * Power sequencer device that enables and monitors the voltage rails.
     */
    std::unique_ptr<PowerSequencerDevice> device;

    /**
     * Indicates if a failure has already been found. Cleared at power on.
     */
    bool failureFound{false};

    /**
     * Indicates if a state transition is taking place
     */
    bool inStateTransition{false};

    /**
     * Minimum time from cold start to power on constant
     */
    static constexpr std::chrono::seconds minimumColdStartTime{15};

    /**
     * Minimum time from power off to power on constant
     */
    static constexpr std::chrono::seconds minimumPowerOffTime{25};

    /**
     * Power good
     */
    int pgood{0};

    /**
     * GPIO line object for chassis power good
     */
    gpiod::line pgoodLine;

    /**
     * Power good timeout constant
     */
    static constexpr std::chrono::seconds pgoodTimeout{PGOOD_TIMEOUT};

    /**
     * Point in time at which power good timeout will take place
     */
    std::chrono::time_point<std::chrono::steady_clock> pgoodTimeoutTime;

    /**
     * Timer to wait after pgood failure. This is to allow the power supplies
     * and other hardware time to complete failure processing.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> pgoodWaitTimer;

    /**
     * Poll interval constant
     */
    static constexpr std::chrono::milliseconds pollInterval{3000};

    /**
     * GPIO line object for power on / power off control
     */
    gpiod::line powerControlLine;

    /**
     * Point in time at which minumum power off time will have passed
     */
    std::chrono::time_point<std::chrono::steady_clock> powerOnAllowedTime;

    /**
     * Power supply error.  Cleared at power on.
     */
    std::string powerSupplyError;

    /**
     * Power state
     */
    int state{0};

    /**
     * Power good timeout
     */
    std::chrono::seconds timeout{pgoodTimeout};

    /**
     * Timer to poll the pgood
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> timer;

    /**
     * Callback to begin failure processing after observing pgood failure wait
     */
    void onFailureCallback();

    /**
     * Begin pgood failure processing
     *
     * @param wasTimeOut Indicates whether failure state was determined by
     *                   timing out
     */
    void onFailure(bool wasTimeOut);

    /**
     * Polling method for monitoring the system power good
     */
    void pollPgood();

    /**
     * Set up GPIOs
     */
    void setUpGpio();

    /**
     * Loads the JSON configuration file and creates the power sequencer device
     * object.
     *
     * Does nothing if the compatible system types or device properties have not
     * been found yet.  These are obtained from D-Bus.  The order in which they
     * are found and the time to find them varies.
     */
    void loadConfigFileAndCreateDevice();

    /**
     * Finds the JSON configuration file for the current system based on the
     * compatible system types.
     *
     * Does nothing if the compatible system types have not been found yet.
     *
     * @return absolute path to the config file, or empty path if file not found
     */
    std::filesystem::path findConfigFile();

    /**
     * Parses the specified JSON configuration file.
     *
     * Returns the resulting vector of Rail objects in the output parameter.
     *
     * @param configFile Absolute path to the config file
     * @param rails Rail objects within the config file
     * @return true if file was parsed successfully, false otherwise
     */
    bool parseConfigFile(const std::filesystem::path& configFile,
                         std::vector<std::unique_ptr<Rail>>& rails);

    /**
     * Creates the power sequencer device object based on the device properties.
     *
     * Does nothing if the device properties have not been found yet.
     *
     * @param rails Voltage rails that are enabled and monitored by the device
     */
    void createDevice(std::vector<std::unique_ptr<Rail>> rails);
};

} // namespace phosphor::power::sequencer
