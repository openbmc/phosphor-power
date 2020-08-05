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

#include "chassis.hpp"
#include "config_file_parser.hpp"
#include "exception_utils.hpp"
#include "rule.hpp"
#include "utility.hpp"

#include <sdbusplus/bus.hpp>

#include <chrono>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <variant>

namespace phosphor::power::regulators
{

namespace fs = std::filesystem;

/**
 * Standard configuration file directory.  This directory is part of the
 * firmware install image.  It contains the standard version of the config file.
 */
const fs::path standardConfigFileDir{"/usr/share/phosphor-regulators"};

/**
 * Test configuration file directory.  This directory can contain a test version
 * of the config file.  The test version will override the standard version.
 */
const fs::path testConfigFileDir{"/etc/phosphor-regulators"};

Manager::Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event) :
    ManagerObject{bus, objPath, true}, bus{bus}, eventLoop{event}, services{bus}
{
    /* Temporarily comment out until D-Bus interface is defined and available.
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
    */

    // Temporarily hard-code JSON config file name to first system that will use
    // this application.  Remove this when D-Bus interface is available.
    fileName = "ibm_rainier.json";

    if (!fileName.empty())
    {
        loadConfigFile();
    }

    // Obtain dbus service name
    bus.request_name(busName);
}

void Manager::configure()
{
    // Verify System object exists; this means config file has been loaded
    if (system)
    {
        // Configure the regulator devices in the system
        system->configure(services);
    }
    else
    {
        services.getJournal().logError("Unable to configure regulator devices: "
                                       "Configuration file not loaded");
        // TODO: Log error
    }

    // TODO Configuration errors that should halt poweron,
    // throw InternalFailure exception (or similar) to
    // fail the call(busctl) to this method
}

void Manager::monitor(bool enable)
{
    if (enable)
    {
        /* Temporarily comment out until monitoring is supported.
            Timer timer(eventLoop, std::bind(&Manager::timerExpired, this));
            // Set timer as a repeating 1sec timer
            timer.restart(std::chrono::milliseconds(1000));
            timers.emplace_back(std::move(timer));
        */
    }
    else
    {
        /* Temporarily comment out until monitoring is supported.
            // Delete all timers to disable monitoring
            timers.clear();
        */

        // Verify System object exists; this means config file has been loaded
        if (system)
        {
            // Close the regulator devices in the system.  Monitoring is
            // normally disabled because the system is being powered off.  The
            // devices should be closed in case hardware is removed or replaced
            // while the system is at standby.
            system->closeDevices(services);
        }
    }
}

void Manager::timerExpired()
{
    // TODO Analyze, refresh sensor status, and
    // collect/update telemetry for each regulator
}

void Manager::sighupHandler(sdeventplus::source::Signal& /*sigSrc*/,
                            const struct signalfd_siginfo* /*sigInfo*/)
{
    if (!fileName.empty())
    {
        loadConfigFile();
    }
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

        // An interfacesAdded signal returns a dictionary of interface
        // names to a dictionary of properties and their values
        // https://dbus.freedesktop.org/doc/dbus-specification.html
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
        if (!fileName.empty())
        {
            loadConfigFile();
        }
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

fs::path Manager::findConfigFile()
{
    // First look in the test directory
    fs::path pathName{testConfigFileDir / fileName};
    if (!fs::exists(pathName))
    {
        // Look in the standard directory
        pathName = standardConfigFileDir / fileName;
        if (!fs::exists(pathName))
        {
            throw std::runtime_error{"Configuration file does not exist: " +
                                     pathName.string()};
        }
    }

    return pathName;
}

void Manager::loadConfigFile()
{
    try
    {
        // Find the absolute path to the config file
        fs::path pathName = findConfigFile();

        // Log info message in journal; config file path is important
        services.getJournal().logInfo("Loading configuration file " +
                                      pathName.string());

        // Parse the config file
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassis{};
        std::tie(rules, chassis) = config_file_parser::parse(pathName);

        // Store config file information in a new System object.  The old System
        // object, if any, is automatically deleted.
        system = std::make_unique<System>(std::move(rules), std::move(chassis));
    }
    catch (const std::exception& e)
    {
        // Log error messages in journal
        services.getJournal().logError(exception_utils::getMessages(e));
        services.getJournal().logError("Unable to load configuration file");

        // TODO: Create error log entry
    }
}

} // namespace phosphor::power::regulators
