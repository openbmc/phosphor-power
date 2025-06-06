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

#include "device_monitor.hpp"
#include "power_supply.hpp"

#include <CLI/CLI.hpp>
#include <sdeventplus/event.hpp>

#include <iostream>

using namespace phosphor::power;

int main(int argc, char* argv[])
{
    CLI::App app{"PSU Monitor"};

    std::string objpath{};
    std::string instnum{};
    std::string invpath{};
    std::string records{};
    std::string syncGPIOPath{};
    std::string syncGPIONum{};

    app.add_option("-p,--path", objpath, "Path to location to monitor\n")
        ->required();
    app.add_option("-n,--instance", instnum,
                   "Instance number for this power supply\n")
        ->required();
    app.add_option("-i,--inventory", invpath,
                   "Inventory path for this power supply\n")
        ->required();
    app.add_option(
           "-r,--num-history-records", records,
           "Number of input power history records to provide on D-Bus\n")
        ->expected(0, 1);
    app.add_option(
           "-a,--sync-gpio-path", syncGPIOPath,
           "GPIO chip device for the GPIO that performs the sync function\n")
        ->expected(0, 1);
    app.add_option("-u,--sync-gpio-num", syncGPIONum,
                   "GPIO number for the GPIO that performs the sync function\n")
        ->expected(0, 1);

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::Error& e)
    {
        return app.exit(e);
    }

    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();

    // Attach the event object to the bus object so we can
    // handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

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
    auto psuDevice = std::make_unique<psu::PowerSupply>(
        objname, std::move(instance), std::move(objpath), std::move(invpath),
        bus, event, powerOnDelay, presentDelay);

    // Get the number of input power history records to keep in D-Bus.
    long int numRecords = 0;
    if (!records.empty())
    {
        numRecords = std::stol(records);
        if (numRecords < 0)
        {
            std::cerr << "Invalid number of history records specified.\n";
            return -6;
        }
    }

    if (numRecords != 0)
    {
        // Get the GPIO information for controlling the SYNC signal.
        // If one is there, they both must be.
        if ((syncGPIOPath.empty() && !syncGPIONum.empty()) ||
            (!syncGPIOPath.empty() && syncGPIONum.empty()))
        {
            std::cerr << "Invalid sync GPIO number or path\n";
            return -7;
        }

        size_t gpioNum = 0;
        if (!syncGPIONum.empty())
        {
            gpioNum = stoul(syncGPIONum);
        }

        std::string name{"ps" + instnum + "_input_power"};
        std::string basePath =
            std::string{INPUT_HISTORY_SENSOR_ROOT} + '/' + name;

        psuDevice->enableHistory(basePath, numRecords, syncGPIOPath, gpioNum);

        // Systemd object manager
        sdbusplus::server::manager_t objManager{bus, basePath.c_str()};

        std::string busName =
            std::string{INPUT_HISTORY_BUSNAME_ROOT} + '.' + name;
        bus.request_name(busName.c_str());
    }

    auto pollInterval = std::chrono::milliseconds(1000);
    return DeviceMonitor(std::move(psuDevice), event, pollInterval).run();
}
