/**
 * Copyright © 2020 IBM Corporation
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

#include "manager.hpp"

#include "utility.hpp"

#include <sdbusplus/bus.hpp>

#include <chrono>

namespace phosphor
{
namespace power
{
namespace regulators
{

Manager::Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event) :
    ManagerObject(bus, objPath, true), bus(bus), eventLoop(event)
{
    // Attempt to get the filename property from dbus
    fileName = getFileNameDbus();

    // TODO Subscribe to interfacesAdded signal for filename property
    // Callback should set fileName and call parse json function

    if (!fileName.empty())
    {
        fileName.append(".json");
        // TODO Load & parse JSON configuration data file
    }

    // Obtain dbus service name
    bus.request_name(busName);
}

void Manager::configure()
{
    // TODO Configuration errors that should halt poweron,
    // throw InternalFailure exception (or similar) to
    // fail the call(busctl) to this method
}

void Manager::monitor(bool enable)
{
    if (enable)
    {
        Timer timer(eventLoop, std::bind(&Manager::timerExpired, this));
        // Set timer as a repeating 1sec timer
        timer.restart(std::chrono::milliseconds(1000));
        timers.emplace_back(std::move(timer));
    }
    else
    {
        // Delete all timers to disable monitoring
        timers.clear();
    }
}

void Manager::timerExpired()
{
    // TODO Analyze, refresh sensor status, and
    // collect/update telemetry for each regulator
}

void Manager::sighupHandler(sdeventplus::source::Signal& sigSrc,
                            const struct signalfd_siginfo* sigInfo)
{
    // Suppress "unused parameter" errors
    // *Note: These may or may not be needed when
    // reloading the configuration data
    (void)sigSrc;
    (void)sigInfo;

    // TODO Reload and process the configuration data
}

const std::string Manager::getFileNameDbus()
{
    std::string fileName = "";
    using namespace phosphor::power::util;

    // Do not log an error when service is not found
    auto service = getService(sysDbusPath, sysDbusIntf, bus, false);
    if (!service.empty())
    {
        getProperty(sysDbusIntf, sysDbusProp, sysDbusPath, service, bus,
                    fileName);
    }

    return fileName;
}

} // namespace regulators
} // namespace power
} // namespace phosphor
