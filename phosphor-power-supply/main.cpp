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

#include <CLI/CLI.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <filesystem>

using namespace phosphor::power;

int main(int argc, char* argv[])
{
    try
    {
        using namespace phosphor::logging;

        CLI::App app{"OpenBMC Power Supply Unit Monitor"};

        std::string configfile;
        app.add_option("-c,--config", configfile,
                       "JSON configuration file path");

        // Read the arguments.
        CLI11_PARSE(app, argc, argv);
        if (configfile.empty())
        {
            if (std::filesystem::exists(
                    "/etc/phosphor-psu-monitor/psu_config.json"))
            {
                configfile = "/etc/phosphor-psu-monitor/psu_config.json";
            }
            else
            {
                configfile = "/usr/share/phosphor-psu-monitor/psu_config.json";
            }
        }

        if (!std::filesystem::exists(configfile))
        {
            log<level::ERR>((std::string("Configuration file does not exist: "
                                         "FILENAME=") +
                             configfile)
                                .c_str());
            return -1;
        }

        auto bus = sdbusplus::bus::new_default();
        auto event = sdeventplus::Event::get_default();

        // Attach the event object to the bus object so we can
        // handle both sd_events (for the timers) and dbus signals.
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

        manager::PSUManager manager(bus, event);

        return manager.run();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(e.what());
        return -2;
    }
    catch (...)
    {
        log<level::ERR>("Caught unexpected exception type");
        return -3;
    }
}
