/**
 * Copyright © 2019 IBM Corporation
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
#include "psu_manager.hpp"
#include "utility.hpp"

#include <stdio.h>

#include <CLI/CLI.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

using namespace phosphor::power;

void get_json_properties(const std::string& path, int& i, sys_properties& p)
{
    using namespace phosphor::logging;

    nlohmann::json psuJSON = util::loadJSONFromFile(path.c_str());

    if (psuJSON == nullptr)
    {
        log<level::ERR>("Failed to parse JSON configuration file");
        i = 1000;
        return;
    }

    i = psuJSON.at("pollInterval");
    printf("pollInterval = %d\n", i);

    p.minPowerSupplies = psuJSON["SystemProperties"]["MinPowerSupplies"];
    p.maxPowerSupplies = psuJSON["SystemProperties"]["MaxPowerSupplies"];
}

int main(int argc, char* argv[])
{
    CLI::App app{"OpenBMC Power Supply Unit Monitor"};

    std::string configfile;
    app.add_option("-c,--config", configfile, "JSON configuration file path");

    // Read the arguments.
    CLI11_PARSE(app, argc, argv);

    // Parse out the JSON properties needed to pass down to the PSU manager.
    auto interval = 1000;
    sys_properties properties;
    get_json_properties(configfile, interval, properties);

    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();

    // Attach the event object to the bus object so we can
    // handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    auto pollInterval = std::chrono::milliseconds(interval);
    manager::psuManager manager(bus, event, pollInterval, properties);

    return manager.run();
}
