#pragma once

#include <chrono>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace phosphor::power::sequencer
{

/**
 * @class PowerControl
 * This class implements GPIO control of power on and off.
 */
class PowerControl
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

  private:
    /**
     * The D-Bus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * Event to loop on
     */
    sdeventplus::Event eventLoop;

    /**
     * Timer to poll the pgood
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>> timer;

    /**
     * Poll interval constant
     */
    static constexpr int pollInterval{3000}; // Milliseconds

    /**
     * Power good timeout constant
     */
    static constexpr int pgoodTimeout{10}; // Seconds

    /**
     * Polling method for monitoring the system power good
     */
    void pollPgood();
};

} // namespace phosphor::power::sequencer

