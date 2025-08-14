#include "chassis_manager.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <filesystem>

using namespace phosphor::power;

int main()
{
        using namespace phosphor::logging;

        CLI::App app{"OpenBMC Power Supply Unit Monitor"};

        auto bus = sdbusplus::bus::new_default();
        auto event = sdeventplus::Event::get_default();

        // Attach the event object to the bus object so we can
        // handle both sd_events (for the timers) and dbus signals.
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

        chassis_manager::ChassisManager chassis_manager(bus, event);

        return chassis_manager.run();
}
