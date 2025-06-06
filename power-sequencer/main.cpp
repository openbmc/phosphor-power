/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "config.h"

#include "mihawk-cpld.hpp"
#include "pgood_monitor.hpp"
#include "runtime_monitor.hpp"
#include "ucd90160.hpp"

#include <CLI/CLI.hpp>
#include <sdeventplus/event.hpp>

#include <chrono>
#include <iostream>

using namespace phosphor::power;

int main(int argc, char** argv)
{
    CLI::App app{"Phosphor sequencer monitor"};
    std::string action{};
    std::string interVal{};

    std::vector<std::string> actionTypes = {"pgood-monitor", "runtime-monitor"};
    app.add_option("-a,--action", action,
                   "Action: pgood-monitor or runtime-monitor\n")
        ->required()
        ->transform(CLI::IsMember(actionTypes));
    app.add_option("-i,--interval", interVal,
                   "Interval in milliseconds:\n"
                   "PGOOD monitor:   time allowed for PGOOD to come up\n"
                   "Runtime monitor: polling interval.\n")
        ->required();

    try
    {
        app.parse(argc, argv);
    }
    catch (CLI::Error& e)
    {
        return app.exit(e);
    }

    auto i = strtoul(interVal.c_str(), nullptr, 10);
    if (i == 0)
    {
        std::cerr << "Invalid interval value\n";
        exit(EXIT_FAILURE);
    }

    std::chrono::milliseconds interval{i};

    auto event = sdeventplus::Event::get_default();
    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    auto device = std::make_unique<SEQUENCER>(0, bus);

    std::unique_ptr<DeviceMonitor> monitor;

    if (action == "pgood-monitor")
    {
        // If PGOOD doesn't turn on within a certain
        // time, analyze the device for errors
        monitor = std::make_unique<PGOODMonitor>(std::move(device), bus, event,
                                                 interval);
    }
    else // runtime-monitor
    {
        // Continuously monitor this device both by polling
        // and on 'power lost' signals.
        monitor = std::make_unique<RuntimeMonitor>(std::move(device), bus,
                                                   event, interval);
    }

    return monitor->run();
}
