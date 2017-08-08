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
#include <iostream>
#include <phosphor-logging/log.hpp>
#include <systemd/sd-daemon.h>
#include "argument.hpp"
#include "event.hpp"
#include "power_supply.hpp"
#include "device_monitor.hpp"

using namespace witherspoon::power;
using namespace phosphor::logging;

int main(int argc, char* argv[])
{
    auto options = ArgumentParser(argc, argv);

    auto objpath = (options)["path"];
    auto instnum = (options)["instance"];
    auto invpath = (options)["inventory"];
    if (argc < 4)
    {
        std::cerr << std::endl << "Too few arguments" << std::endl;
        options.usage(argv);
        return -1;
    }

    if (objpath == ArgumentParser::emptyString)
    {
        log<level::ERR>("Device monitoring path argument required");
        return -2;
    }

    if (instnum == ArgumentParser::emptyString)
    {
        log<level::ERR>("Device monitoring instance number argument required");
        return -3;
    }

    if (invpath == ArgumentParser::emptyString)
    {
        log<level::ERR>("Device monitoring inventory path argument required");
        return -4;
    }

    auto bus = sdbusplus::bus::new_default();

    auto objname = "power_supply" + instnum;
    auto instance = std::stoul(instnum);
    auto psuDevice = std::make_unique<psu::PowerSupply>(objname,
                                                        std::move(instance),
                                                        std::move(objpath),
                                                        std::move(invpath),
                                                        bus);

    sd_event* events = nullptr;

    auto r = sd_event_default(&events);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_default()",
                        entry("ERROR=%s", strerror(-r)));
        return -5;
    }

    witherspoon::power::event::Event eventPtr{events};

    //Attach the event object to the bus object so we can
    //handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

    // TODO: Use inventory path to subscribe to signal change for power supply presence.

    //Attach the event object to the bus object so we can
    //handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

    // TODO: Get power state on startup.
    // TODO: Get presence state on startup and subscribe to presence changes.
    // TODO - set to vinUVFault to false on presence change & start of poweron
    // TODO - set readFailLogged to false on presence change and start of poweron
    auto pollInterval = std::chrono::milliseconds(1000);
    DeviceMonitor mainloop(std::move(psuDevice), eventPtr, pollInterval);
    mainloop.run();

    return 0;
}
