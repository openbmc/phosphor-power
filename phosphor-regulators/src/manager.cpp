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
#include "format_utils.hpp"
#include "rule.hpp"
#include "utility.hpp"

#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/State/Chassis/server.hpp>

#include <chrono>
#include <exception>
#include <functional>
#include <span>
#include <thread>
#include <tuple>
#include <utility>

namespace phosphor::power::regulators
{

namespace fs = std::filesystem;

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto managerObjPath = "/xyz/openbmc_project/power/regulators/manager";
constexpr auto chassisStatePath = "/xyz/openbmc_project/state/chassis0";
constexpr auto chassisStateIntf = "xyz.openbmc_project.State.Chassis";
constexpr auto chassisStateProp = "CurrentPowerState";
constexpr std::chrono::minutes maxTimeToWaitForCompatTypes{5};

using PowerState =
    sdbusplus::xyz::openbmc_project::State::server::Chassis::PowerState;

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

Manager::Manager(sdbusplus::bus_t& bus, const sdeventplus::Event& event) :
    ManagerObject{bus, managerObjPath}, bus{bus}, eventLoop{event},
    services{bus},
    phaseFaultTimer{event, std::bind(&Manager::phaseFaultTimerExpired, this)},
    sensorTimer{event, std::bind(&Manager::sensorTimerExpired, this)}
{
    // Create object to find compatible system types for current system.
    // Note that some systems do not provide this information.
    compatSysTypesFinder = std::make_unique<util::CompatibleSystemTypesFinder>(
        bus, std::bind_front(&Manager::compatibleSystemTypesFound, this));

    // If no system types found so far, try to load default config file
    if (compatibleSystemTypes.empty())
    {
        loadConfigFile();
    }

    // Obtain D-Bus service name
    bus.request_name(busName);

    // If system is already powered on, enable monitoring
    if (isSystemPoweredOn())
    {
        monitor(true);
    }
}

void Manager::configure()
{
    // Clear any cached data or error history related to hardware devices
    clearHardwareData();

    // Wait until the config file has been loaded or hit max wait time
    waitUntilConfigFileLoaded();

    // Verify config file has been loaded and System object is valid
    if (isConfigFileLoaded())
    {
        // Configure the regulator devices in the system
        system->configure(services);
    }
    else
    {
        // Write error message to journal
        services.getJournal().logError("Unable to configure regulator devices: "
                                       "Configuration file not loaded");

        // Log critical error since regulators could not be configured.  Could
        // cause hardware damage if default regulator settings are very wrong.
        services.getErrorLogging().logConfigFileError(Entry::Level::Critical,
                                                      services.getJournal());

        // Throw InternalFailure to propogate error status to D-Bus client
        throw sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure{};
    }
}

void Manager::monitor(bool enable)
{
    // Check whether already in the requested monitoring state
    if (enable == isMonitoringEnabled)
    {
        return;
    }

    isMonitoringEnabled = enable;
    if (isMonitoringEnabled)
    {
        services.getJournal().logDebug("Monitoring enabled");

        // Restart phase fault detection timer with repeating 15 second interval
        phaseFaultTimer.restart(std::chrono::seconds(15));

        // Restart sensor monitoring timer with repeating 1 second interval
        sensorTimer.restart(std::chrono::seconds(1));

        // Enable sensors service; put all sensors in an active state
        services.getSensors().enable();
    }
    else
    {
        services.getJournal().logDebug("Monitoring disabled");

        // Disable timers
        phaseFaultTimer.setEnabled(false);
        sensorTimer.setEnabled(false);

        // Disable sensors service; put all sensors in an inactive state
        services.getSensors().disable();

        // Verify config file has been loaded and System object is valid
        if (isConfigFileLoaded())
        {
            // Close the regulator devices in the system.  Monitoring is
            // normally disabled because the system is being powered off.  The
            // devices should be closed in case hardware is removed or replaced
            // while the system is powered off.
            system->closeDevices(services);
        }
    }
}

void Manager::compatibleSystemTypesFound(const std::vector<std::string>& types)
{
    // If we don't already have compatible system types
    if (compatibleSystemTypes.empty())
    {
        std::string typesStr = format_utils::toString(std::span{types});
        services.getJournal().logInfo(
            std::format("Compatible system types found: {}", typesStr));

        // Store compatible system types
        compatibleSystemTypes = types;

        // Find and load JSON config file based on system types
        loadConfigFile();
    }
}

void Manager::phaseFaultTimerExpired()
{
    // Verify config file has been loaded and System object is valid
    if (isConfigFileLoaded())
    {
        // Detect redundant phase faults in regulator devices in the system
        system->detectPhaseFaults(services);
    }
}

void Manager::sensorTimerExpired()
{
    // Notify sensors service that a sensor monitoring cycle is starting
    services.getSensors().startCycle();

    // Verify config file has been loaded and System object is valid
    if (isConfigFileLoaded())
    {
        // Monitor sensors for the voltage rails in the system
        system->monitorSensors(services);
    }

    // Notify sensors service that current sensor monitoring cycle has ended
    services.getSensors().endCycle();
}

void Manager::sighupHandler(sdeventplus::source::Signal& /*sigSrc*/,
                            const struct signalfd_siginfo* /*sigInfo*/)
{
    // Reload the JSON configuration file
    loadConfigFile();
}

void Manager::clearHardwareData()
{
    // Clear any cached hardware presence data and VPD values
    services.getPresenceService().clearCache();
    services.getVPD().clearCache();

    // Verify config file has been loaded and System object is valid
    if (isConfigFileLoaded())
    {
        // Clear any cached hardware data in the System object
        system->clearCache();

        // Clear error history related to hardware devices in the System object
        system->clearErrorHistory();
    }
}

fs::path Manager::findConfigFile()
{
    // Build list of possible base file names
    std::vector<std::string> fileNames{};

    // Add possible file names based on compatible system types (if any)
    for (const std::string& systemType : compatibleSystemTypes)
    {
        // Look for file name that is entire system type + ".json"
        // Example: com.acme.Hardware.Chassis.Model.MegaServer.json
        fileNames.emplace_back(systemType + ".json");

        // Look for file name that is last node of system type + ".json"
        // Example: MegaServer.json
        std::string::size_type pos = systemType.rfind('.');
        if ((pos != std::string::npos) && ((systemType.size() - pos) > 1))
        {
            fileNames.emplace_back(systemType.substr(pos + 1) + ".json");
        }
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

bool Manager::isSystemPoweredOn()
{
    bool isOn{false};

    try
    {
        // Get D-Bus property that contains the current power state for
        // chassis0, which represents the entire system (all chassis)
        using namespace phosphor::power::util;
        auto service = getService(chassisStatePath, chassisStateIntf, bus);
        if (!service.empty())
        {
            PowerState currentPowerState;
            getProperty(chassisStateIntf, chassisStateProp, chassisStatePath,
                        service, bus, currentPowerState);
            if (currentPowerState == PowerState::On)
            {
                isOn = true;
            }
        }
    }
    catch (const std::exception& e)
    {
        // Current power state might not be available yet.  The regulators
        // application can start before the power state is published on D-Bus.
    }

    return isOn;
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
            system = std::make_unique<System>(std::move(rules),
                                              std::move(chassis));
        }
    }
    catch (const std::exception& e)
    {
        // Log error messages in journal
        services.getJournal().logError(exception_utils::getMessages(e));
        services.getJournal().logError("Unable to load configuration file");

        // Log error
        services.getErrorLogging().logConfigFileError(Entry::Level::Error,
                                                      services.getJournal());
    }
}

void Manager::waitUntilConfigFileLoaded()
{
    // If config file not loaded and list of compatible system types is empty
    if (!isConfigFileLoaded() && compatibleSystemTypes.empty())
    {
        // Loop until compatible system types found or waited max amount of time
        auto start = std::chrono::system_clock::now();
        std::chrono::system_clock::duration timeWaited{0};
        while (compatibleSystemTypes.empty() &&
               (timeWaited <= maxTimeToWaitForCompatTypes))
        {
            // Try to find list of compatible system types.  Force finder object
            // to re-find system types on D-Bus because we are not receiving
            // InterfacesAdded signals within this while loop.
            compatSysTypesFinder->refind();
            if (compatibleSystemTypes.empty())
            {
                // Not found; sleep 5 seconds
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(5s);
            }
            timeWaited = std::chrono::system_clock::now() - start;
        }
    }
}

} // namespace phosphor::power::regulators
