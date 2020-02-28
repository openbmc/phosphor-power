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
#include <variant>

namespace phosphor
{
namespace power
{
namespace regulators
{

Manager::Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event) :
    ManagerObject(bus, objPath, true), bus(bus), eventLoop(event), fileName("")
{
    // Subscribe to interfacesAdded signal for filename property
    std::unique_ptr<sdbusplus::server::match::match> matchPtr =
        std::make_unique<sdbusplus::server::match::match>(
            bus,
            sdbusplus::bus::match::rules::interfacesAdded(sysDbusObj).c_str(),
            std::bind(std::mem_fn(&Manager::signalHandler), this,
                      std::placeholders::_1));
    signals.emplace_back(std::move(matchPtr));

    // Attempt to get the filename property from dbus
    setFileName(getFileNameDbus());

    if (!fileName.empty())
    {
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

void Manager::signalHandler(sdbusplus::message::message& msg)
{
    if (msg)
    {
        sdbusplus::message::object_path op;
        msg.read(op);
        if (static_cast<const std::string&>(op) != sysDbusPath)
        {
            // Object path does not match the path
            return;
        }

        std::map<std::string, std::map<std::string, std::variant<std::string>>>
            intfProp;
        msg.read(intfProp);
        auto itIntf = intfProp.find(sysDbusIntf);
        if (itIntf == intfProp.cend())
        {
            // Interface not found on the path
            return;
        }
        auto itProp = itIntf->second.find(sysDbusProp);
        if (itProp == itIntf->second.cend())
        {
            // Property not found on the interface
            return;
        }
        // Set fileName and call parse json function
        setFileName(std::get<std::string>(itProp->second));
        // TODO Load & parse JSON configuration data file
    }
}

const std::string Manager::getFileNameDbus()
{
    std::string fileName = "";
    using namespace phosphor::power::util;

    try
    {
        // Do not log an error when service or property are not found
        auto service = getService(sysDbusPath, sysDbusIntf, bus, false);
        if (!service.empty())
        {
            getProperty(sysDbusIntf, sysDbusProp, sysDbusPath, service, bus,
                        fileName);
        }
    }
    catch (const sdbusplus::exception::SdBusError&)
    {
        // File name property not available on dbus
        fileName = "";
    }

    return fileName;
}

} // namespace regulators
} // namespace power
} // namespace phosphor
