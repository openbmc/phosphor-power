/**
 * Copyright © 2026 IBM Corporation
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

#include "config_file_parser.hpp"
#include "format_utils.hpp"

#include <phosphor-logging/lg2.hpp>

#include <chrono>
#include <exception>
#include <functional>

namespace phosphor::power::chassis
{

namespace fs = std::filesystem;

/**
 * Standard configuration file directory.  This directory is part of the
 * firmware install image.  It contains the standard version of the config file.
 */
const fs::path standardConfigFileDir{"/usr/share/phosphor-chassis-power"};

/**
 * Test configuration file directory.  This directory is not part of the
 * firmware install image.  It contains the test version of the config file.
 */
const fs::path testConfigFileDir{"/tmp/phosphor-chassis-power"};

constexpr auto busName = "xyz.openbmc_project.Power.Chassis";
constexpr auto chassisStatePath = "/xyz/openbmc_project/state/chassis0";
constexpr auto chassisStateIntf = "xyz.openbmc_project.State.Chassis";
constexpr auto chassisStateProp = "CurrentPowerState";

constexpr std::chrono::minutes maxTimeToWaitForCompatTypes{1};

constexpr std::chrono::seconds monitorInterval{1};

using PowerState =
    sdbusplus::xyz::openbmc_project::State::server::Chassis::PowerState;

Manager::Manager(const sdeventplus::Event& event, Services& services) :
    eventLoop(event),
    compatibleSystemsTimer{
        event,
        std::bind(&Manager::compatibleSystemTypesNotFoundCallback, this)},
    monitorTimer{event, std::bind(&Manager::monitor, this), monitorInterval},
    services(services)
{
    // Start a timer to wait for compatible types.
    compatibleSystemsTimer.restartOnce(maxTimeToWaitForCompatTypes);

    // Create object to find compatible system types for current system.
    compatSysTypesFinder = std::make_unique<util::CompatibleSystemTypesFinder>(
        services.getBus(),
        std::bind_front(&Manager::compatibleSystemTypesFound, this));

    // Subscribe to system power state changes
    try
    {
        chassisPowerStateMatch = std::make_unique<sdbusplus::match>(
            services.getBus(),
            sdbusplus::match_rules::propertiesChanged(chassisStatePath,
                                                      chassisStateIntf),
            std::bind_front(&Manager::chassisPowerStateChanged, this));
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to create D-Bus match for chassis power state changes: {EXCEPTION}",
            "EXCEPTION", e.what());
        throw;
    }

    // Obtain D-Bus service name
    services.getBus().request_name(busName);
}

void Manager::compatibleSystemTypesNotFoundCallback()
{
    lg2::error("Compatible system types not found");
}

void Manager::monitor()
{
    if (system)
    {
        system->monitor();
    }
}

void Manager::compatibleSystemTypesFound(const std::vector<std::string>& types)
{
    // If we don't already have compatible system types
    if (compatibleSystemTypes.empty())
    {
        // Compatible systems found disable fail timer.
        compatibleSystemsTimer.setEnabled(false);

        std::string typesStr = format_utils::toString(std::span{types});
        lg2::info("Compatible system types found: {TYPES}", "TYPES", typesStr);
        // Store compatible system types
        compatibleSystemTypes = types;

        // Find and load JSON config file based on system types
        loadConfigFile();
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

    // Look for a config file with one of the possible base names
    for (const std::string& fileName : fileNames)
    {
        // Check if file exists in test directory
        fs::path pathName = testConfigFileDir / fileName;
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
            lg2::info("Loading configuration file {PATH}", "PATH",
                      pathName.string());

            // Parse the config file
            auto chassis = config_file_parser::parse(pathName, services);

            // Create System object with parsed chassis, passing services as a
            // shared_ptr (non-owning; Manager owns the Services lifetime)
            system = std::make_unique<System>(
                std::move(chassis),
                std::shared_ptr<Services>(&services, [](Services*) {}));

            lg2::info("Parsed {NUMBER} chassis", "NUMBER",
                      system->getChassis().size());

            // Initialize power system inputs status for all chassis
            system->initializePowerSystemInputs(services.getBus());

            // Initialize the status monitors for all chassis
            system->initializeStatusMonitors();
        }
        else
        {
            lg2::error("Error config file not found");
        }
    }
    catch (const std::exception& e)
    {
        // Log error messages in journal
        lg2::error("Unable to load configuration file: {EXCEPTION}",
                   "EXCEPTION", e.what());
    }
}

void Manager::clearErrorHistory()
{
    if (system)
    {
        system->clearErrorHistory();
    }
}

void Manager::chassisPowerStateChanged(sdbusplus::message_t& msg)
{
    std::string interface;
    std::map<std::string, std::variant<std::string>> properties;
    msg.read(interface, properties);

    auto it = properties.find(chassisStateProp);

    if (it != properties.end())
    {
        const auto* powerStateStr = std::get_if<std::string>(&it->second);
        if (powerStateStr != nullptr)
        {
            // Convert string to PowerState enum
            PowerState powerState = sdbusplus::xyz::openbmc_project::State::
                server::Chassis::convertPowerStateFromString(*powerStateStr);

            if (powerState == PowerState::TransitioningToOn)
            {
                clearErrorHistory();
            }
        }
    }
}

} // namespace phosphor::power::chassis
