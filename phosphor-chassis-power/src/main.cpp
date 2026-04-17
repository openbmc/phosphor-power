// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2025 IBM Corporation

#include "manager.hpp"
#include "services.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <exception>

int main()
{
    using namespace phosphor::power;

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto event = sdeventplus::Event::get_default();
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

        chassis::BMCServices services;
        chassis::Manager manager(bus, event, services);

        return event.loop();
    }
    catch (const std::exception& e)
    {
        lg2::error("Application failed: {ERROR}", "ERROR", e);
        return 1;
    }
}
