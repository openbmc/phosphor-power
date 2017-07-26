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

using namespace witherspoon::power;
using namespace phosphor::logging;

int main(int argc, char* argv[])
{
    auto rc = -1;
    auto options = ArgumentParser(argc, argv);

    auto objpath = (options)["path"];
    if (argc < 2)
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

    auto bus = sdbusplus::bus::new_default();
    sd_event* events = nullptr;

    auto r = sd_event_default(&events);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_default()",
                        entry("ERROR=%s", strerror(-r)));
        return -3;
    }

    witherspoon::power::event::Event eventPtr{events};

    //Attach the event object to the bus object so we can
    //handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

    r = sd_event_loop(eventPtr.get());
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_loop",
                        entry("ERROR=%s", strerror(-r)));
    }

    rc = 0;

    return rc;
}
