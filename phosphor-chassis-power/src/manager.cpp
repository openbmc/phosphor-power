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

#include "format_utils.hpp"

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

constexpr std::chrono::minutes maxTimeToWaitForCompatTypes{1};

Manager::Manager(sdbusplus::bus_t& bus, const sdeventplus::Event& event) :
    bus(bus), eventLoop(event),
    compatibleSystemsTimer{
        event, std::bind(&Manager::compatibleSystemTypesNotFoundCallback, this)}
{
    // Start a timer to wait for compatible types.
    compatibleSystemsTimer.restartOnce(maxTimeToWaitForCompatTypes);

    // Create object to find compatible system types for current system.
    // Note that some systems do not provide this information.
    compatSysTypesFinder = std::make_unique<util::CompatibleSystemTypesFinder>(
        bus, std::bind_front(&Manager::compatibleSystemTypesFound, this));

    // Obtain D-Bus service name
    bus.request_name(busName);
}

void Manager::compatibleSystemTypesNotFoundCallback()
{
    lg2::error("Compatible system types not found");
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
            system = true;

            // Parse the config file

            // Store config file information
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

} // namespace phosphor::power::chassis
