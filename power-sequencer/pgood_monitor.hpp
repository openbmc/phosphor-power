#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include "event.hpp"
#include "timer.hpp"

namespace witherspoon
{
namespace power
{

/**
 * @class PGOODMonitor
 *
 * Monitors PGOOD and creates an error if it doesn't come on in time.
 *
 * The run() function is designed to be called right after the
 * power sequencer device is told to kick off a power on.
 *
 * Future commits will analyze the power sequencer chip for errors
 * on a PGOOD fail.
 */
class PGOODMonitor
{
    public:

        PGOODMonitor() = delete;
        ~PGOODMonitor() = default;
        PGOODMonitor(const PGOODMonitor&) = delete;
        PGOODMonitor& operator=(const PGOODMonitor&) = delete;
        PGOODMonitor(PGOODMonitor&&) = delete;
        PGOODMonitor& operator=(PGOODMonitor&&) = delete;

        /**
         * Constructor
         *
         * @param[in] b - D-Bus bus object
         * @param[in] e - event object
         * @param[in] t - time to allow PGOOD to come up
         */
        PGOODMonitor(sdbusplus::bus::bus& b,
                event::Event& e,
                std::chrono::seconds& t) :
            bus(b),
            event(e),
            interval(t),
            timer(e, [this]() { this->analyze(); })
            {
            }

        /**
         * The timer callback.
         *
         * Creates a PGOOD failure error log.
         */
        void analyze();

        /**
         * Waits a specified amount of time for PGOOD to
         * come on, and if it fails to come on in that time
         * an error log will be created.
         *
         * @return - the return value from sd_event_loop()
         */
        int run();

    private:

        /**
         * Enables the properties changed signal callback
         * on the power object so we can tell when PGOOD
         * comes on.
         */
        void startListening();

        /**
         * The callback function for the properties changed
         * signal.
         */
        void propertyChanged();

        /**
         * Returns true if the system has been turned on
         * but PGOOD isn't up yet.
         */
        bool pgoodPending();

        /**
         * Used to break out of the event loop in run()
         */
        void exitEventLoop();

        /**
         * The D-Bus object
         */
        sdbusplus::bus::bus& bus;

        /**
         * The match object for the properties changed signal
         */
        std::unique_ptr<sdbusplus::bus::match_t> match;

        /**
         * The sd_event structure used by the timer
         */
        event::Event& event;

        /**
         * The amount of time to wait for PGOOD to turn on
         */
        std::chrono::seconds interval;

        /**
         * The timer used to do the callback
         */
        Timer timer;
};

}
}
