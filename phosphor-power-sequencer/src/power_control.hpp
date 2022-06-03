#pragma once

#include "power_interface.hpp"
#include "power_sequencer_monitor.hpp"
#include "utility.hpp"

#include <gpiod.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>

namespace phosphor::power::sequencer
{

using PowerObject = sdbusplus::server::object::object<PowerInterface>;

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
    PowerControl(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /** @copydoc PowerInterface::getPgood() */
    int getPgood() const override;

    /** @copydoc PowerInterface::getPgoodTimeout() */
    int getPgoodTimeout() const override;

    /** @copydoc PowerInterface::getState() */
    int getState() const override;

    /**
     * Callback function to handle interfacesAdded D-Bus signals
     * @param msg Expanded sdbusplus message data
     */
    void interfacesAddedHandler(sdbusplus::message::message& msg);

    /** @copydoc PowerInterface::setPgoodTimeout() */
    void setPgoodTimeout(int timeout) override;

    /** @copydoc PowerInterface::setState() */
    void setState(int state) override;

    /** @copydoc PowerInterface::setPowerSupplyError() */
    void setPowerSupplyError(const std::string& error) override;

  private:
    /**
     * The D-Bus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * The power sequencer device to monitor.
     */
    std::unique_ptr<PowerSequencerMonitor> device;

    /**
     * Indicates if a specific power sequencer device has already been found.
     */
    bool deviceFound{false};

    /**
     * Indicates if a failure has already been found. Cleared at power on.
     */
    bool failureFound{false};

    /**
     * Indicates if a state transition is taking place
     */
    bool inStateTransition{false};

    /**
     * The match to Entity Manager interfaces added.
     */
    sdbusplus::bus::match_t match;

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
    static constexpr std::chrono::seconds pgoodTimeout{10};

    /**
     * Point in time at which power good timeout will take place
     */
    std::chrono::time_point<std::chrono::steady_clock> pgoodTimeoutTime;

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
     * Get the device properties
     * @param properties A map of property names and values
     */
    void getDeviceProperties(util::DbusPropertyMap& properties);

    /**
     * Polling method for monitoring the system power good
     */
    void pollPgood();

    /**
     * Set up power sequencer device
     */
    void setUpDevice();

    /**
     * Set up GPIOs
     */
    void setUpGpio();
};

} // namespace phosphor::power::sequencer
