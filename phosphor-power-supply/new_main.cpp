#include "chassis_manager.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <filesystem>

using namespace phosphor::power;

int main()
{
    try
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
    catch (const std::exception& e)
    {
        lg2::error("main exception {ERR}", "ERR", e);
        return -2;
    }
    catch (...)
    {
        lg2::error("Caught unexpected exception type");
        return -3;
    }
    return 1;
}
