#pragma once

#include "config.h"

#include "compatible_system_types_finder.hpp"
#include "power_interface.hpp"
#include "services.hpp"
#include "system.hpp"

#include <gpiod.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
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
     * Computer system being controlled and monitored by the BMC.
     *
     * Contains the information loaded from the JSON configuration file.
     * Contains nullptr if the configuration file has not been loaded.
     */
    std::unique_ptr<System> system;

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
     * Checks whether a pgood fault has occurred on one of the rails being
     * monitored.
     *
     * If a pgood fault was found, this method returns a string containing the
     * error that should be logged.  If no fault was found, an empty string is
     * returned.
     *
     * @param additionalData Additional data to include in the error log if
     *                       a pgood fault was found
     * @return error that should be logged if a pgood fault was found, or an
     *         empty string if no pgood fault was found
     */
    std::string findPgoodFault(
        std::map<std::string, std::string>& additionalData);

    /**
     * Polling method for monitoring the system power good
     */
    void pollPgood();

    /**
     * Set up GPIOs
     */
    void setUpGpio();

    /**
     * Loads the JSON configuration file.
     *
     * Looks for the config file using findConfigFile().
     *
     * If the config file is found, it is parsed and the resulting information
     * is stored in the system data member.
     */
    void loadConfigFile();

    /**
     * Finds the JSON configuration file for the current system based on the
     * compatible system types.
     *
     * Does nothing if the compatible system types have not been found yet.
     *
     * @return absolute path to the config file, or empty path if file not found
     */
    std::filesystem::path findConfigFile();
};

} // namespace phosphor::power::sequencer
