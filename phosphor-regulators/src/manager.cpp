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

#include <algorithm>
#include <chrono>
#include <exception>
#include <functional>
#include <map>
#include <tuple>
#include <utility>
#include <variant>

namespace phosphor::power::regulators
{

namespace fs = std::filesystem;

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto managerObjPath = "/xyz/openbmc_project/power/regulators/manager";
constexpr auto compatibleIntf =
    "xyz.openbmc_project.Configuration.IBMCompatibleSystem";
constexpr auto compatibleNamesProp = "Names";

/**
 * Default configuration file name.  This is used when the system does not
 * implement the D-Bus compatible interface.
 */
constexpr auto defaultConfigFileName = "config.json";

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
    ManagerObject{bus, managerObjPath, true}, bus{bus}, eventLoop{event},
    services{bus}
{
    // Subscribe to D-Bus interfacesAdded signal from Entity Manager.  This
    // notifies us if the compatible interface becomes available later.
    std::string matchStr = sdbusplus::bus::match::rules::interfacesAdded() +
                           sdbusplus::bus::match::rules::sender(
                               "xyz.openbmc_project.EntityManager");
    std::unique_ptr<sdbusplus::server::match::match> matchPtr =
        std::make_unique<sdbusplus::server::match::match>(
            bus, matchStr,
            std::bind(&Manager::interfacesAddedHandler, this,
                      std::placeholders::_1));
    signals.emplace_back(std::move(matchPtr));

    // Try to find compatible system types using D-Bus compatible interface.
    // Note that it might not be supported on this system, or the service that
    // provides the interface might not be running yet.
    findCompatibleSystemTypes();

    // Try to find and load the JSON configuration file
    loadConfigFile();

    // Obtain dbus service name
    bus.request_name(busName);
}

void Manager::configure()
{
    // Clear any cached data or error history related to hardware devices
    clearHardwareData();

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

void Manager::interfacesAddedHandler(sdbusplus::message::message& msg)
{
    // Verify message is valid
    if (!msg)
    {
        return;
    }

    try
    {
        // Read object path for object that was created or had interface added
        sdbusplus::message::object_path objPath;
        msg.read(objPath);

        // Read the dictionary whose keys are interface names and whose values
        // are dictionaries containing the interface property names and values
        std::map<std::string,
                 std::map<std::string, std::variant<std::vector<std::string>>>>
            intfProp;
        msg.read(intfProp);

        // Find the compatible interface, if present
        auto itIntf = intfProp.find(compatibleIntf);
        if (itIntf != intfProp.cend())
        {
            // Find the Names property of the compatible interface, if present
            auto itProp = itIntf->second.find(compatibleNamesProp);
            if (itProp != itIntf->second.cend())
            {
                // Get value of Names property
                auto propValue = std::get<0>(itProp->second);
                if (!propValue.empty())
                {
                    // Store list of compatible system types
                    compatibleSystemTypes = propValue;

                    // Find and load JSON config file based on system types
                    loadConfigFile();
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Error trying to read interfacesAdded message.  One possible cause
        // could be a property whose value is not a std::vector<std::string>.
    }
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
            // while the system is powered off.
            system->closeDevices(services);
        }
    }
}

void Manager::sighupHandler(sdeventplus::source::Signal& /*sigSrc*/,
                            const struct signalfd_siginfo* /*sigInfo*/)
{
    // Reload the JSON configuration file
    loadConfigFile();
}

void Manager::timerExpired()
{
    // TODO Analyze, refresh sensor status, and
    // collect/update telemetry for each regulator
}

void Manager::clearHardwareData()
{
    // Clear any cached hardware presence data
    services.getPresenceService().clearCache();

    // Verify System object exists; this means config file has been loaded
    if (system)
    {
        // Clear any cached hardware data in the System object
        system->clearCache();
    }

    // TODO: Clear error history related to hardware devices
}

void Manager::findCompatibleSystemTypes()
{
    using namespace phosphor::power::util;

    try
    {
        // Query object mapper for object paths that implement the compatible
        // interface.  Returns a map of object paths to a map of services names
        // to their interfaces.
        DbusSubtree subTree = getSubTree(bus, "/xyz/openbmc_project/inventory",
                                         compatibleIntf, 0);

        // Get the first object path
        auto objectIt = subTree.cbegin();
        if (objectIt != subTree.cend())
        {
            std::string objPath = objectIt->first;

            // Get the first service name
            auto serviceIt = objectIt->second.cbegin();
            if (serviceIt != objectIt->second.cend())
            {
                std::string service = serviceIt->first;
                if (!service.empty())
                {
                    // Get compatible system types property value
                    getProperty(compatibleIntf, compatibleNamesProp, objPath,
                                service, bus, compatibleSystemTypes);
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Compatible system types information is not available.  The current
        // system might not support the interface, or the service that
        // implements the interface might not be running yet.
    }
}

fs::path Manager::findConfigFile()
{
    // Build list of possible base file names
    std::vector<std::string> fileNames{};

    // Add possible file names based on compatible system types (if any)
    for (const std::string& systemType : compatibleSystemTypes)
    {
        // Replace all spaces and commas in system type name with underscores
        std::string fileName{systemType};
        std::replace(fileName.begin(), fileName.end(), ' ', '_');
        std::replace(fileName.begin(), fileName.end(), ',', '_');

        // Append .json suffix and add to list
        fileName.append(".json");
        fileNames.emplace_back(fileName);
    }

    // Add default file name for systems that don't use compatible interface
    fileNames.emplace_back(defaultConfigFileName);

    // Look for a config file with one of the possible base names
    for (const std::string& fileName : fileNames)
    {
        // Check if file exists in test directory
        fs::path pathName{testConfigFileDir / fileName};
        if (fs::exists(pathName))
        {
            return pathName;
        }

        // Check if file exists in standard directory
        pathName = standardConfigFileDir / fileName;
        if (fs::exists(pathName))
        {
            return pathName;
        }
    }

    // No config file found; return empty path
    return fs::path{};
}

void Manager::loadConfigFile()
{
    try
    {
        // Find the absolute path to the config file
        fs::path pathName = findConfigFile();
        if (!pathName.empty())
        {
            // Log info message in journal; config file path is important
            services.getJournal().logInfo("Loading configuration file " +
                                          pathName.string());

            // Parse the config file
            std::vector<std::unique_ptr<Rule>> rules{};
            std::vector<std::unique_ptr<Chassis>> chassis{};
            std::tie(rules, chassis) = config_file_parser::parse(pathName);

            // Store config file information in a new System object.  The old
            // System object, if any, is automatically deleted.
            system =
                std::make_unique<System>(std::move(rules), std::move(chassis));
        }
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
