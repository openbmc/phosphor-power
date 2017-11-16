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

    sd_event* events = nullptr;

    auto r = sd_event_default(&events);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_default()",
                        entry("ERROR=%s", strerror(-r)));
        return -5;
    }

    auto bus = sdbusplus::bus::new_default();
    witherspoon::power::event::Event eventPtr{events};

    //Attach the event object to the bus object so we can
    //handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

    auto objname = "power_supply" + instnum;
    auto instance = std::stoul(instnum);
    // The state changes from 0 to 1 when the BMC_POWER_UP line to the power
    // sequencer is asserted. It can take 50ms for the sequencer to assert the
    // ENABLE# line that goes to the power supplies. The Witherspoon power
    // supply can take a max of 100ms from ENABLE# asserted to 12V in spec.
    // Once 12V in spec., the power supply will nominally take 1 second to
    // assert DC_GOOD (and update POWER_GOOD Negated), +/1 100ms. That would
    // give us a 1250ms delay from state=1 to checking STATUS_WORD, however,
    // the sysfs files will only be updated by the ibm-cffps device driver once
    // a second, so rounding up from 1 to 5 seconds.
    std::chrono::seconds powerOnDelay(5);
    // Timer to delay setting internal presence tracking. Allows for servicing
    // the power supply.
    std::chrono::seconds presentDelay(2);
    auto psuDevice = std::make_unique<psu::PowerSupply>(objname,
                                                        std::move(instance),
                                                        std::move(objpath),
                                                        std::move(invpath),
                                                        bus,
                                                        eventPtr,
                                                        powerOnDelay,
                                                        presentDelay);

    auto pollInterval = std::chrono::milliseconds(1000);
    DeviceMonitor mainloop(std::move(psuDevice), eventPtr, pollInterval);
    mainloop.run();

    return 0;
}
