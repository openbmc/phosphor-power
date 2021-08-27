#pragma once

#include "power_interface.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server/object.hpp>
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
     * @param[in] bus D-Bus bus object
     * @param[in] event event object
     */
    PowerControl(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /**
     * Overridden power object's getPgood method
     * @return power good
     */
    int getPgood() override;

    /**
     * Overridden power object's getPgoodTimeout method
     * @return power good timeout
     */
    int getPgoodTimeout() override;

    /**
     * Overridden power object's getState method
     * @return power state
     */
    int getState() override;

    /**
     * Overridden power object's setPgoodTimeout method
     * @param[in] timeout power good timeout
     */
    void setPgoodTimeout(int timeout) override;

    /**
     * Overridden power object's setState method
     * @param[in] state power state.
     */
    void setState(int state) override;

  private:
    /**
     * The D-Bus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * Power good
     */
    int pgood{0};

    /**
     * Power good timeout constant
     */
    static constexpr int pgoodTimeout{10}; // Seconds

    /**
     * Poll interval constant
     */
    static constexpr int pollInterval{3000}; // Milliseconds

    /**
     * Power state
     */
    int state{0};

    /**
     * Power good timeout
     */
    int timeout{pgoodTimeout};

    /**
     * Timer to poll the pgood
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> timer;

    /**
     * Polling method for monitoring the system power good
     */
    void pollPgood();
};

} // namespace phosphor::power::sequencer
