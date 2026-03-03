/**
 * Copyright © 2021 IBM Corporation
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
#include "types.hpp"

#include <exception>
#include <format>
#include <functional>
#include <span>
#include <utility>

namespace phosphor::power::sequencer
{

constexpr auto rootObjectPath = "/org/openbmc/control";

Manager::Manager(sdbusplus::bus_t& bus, const sdeventplus::Event& event) :
    bus{bus}, objectManager{bus, rootObjectPath}, services{bus},
    monitorTimer{event, std::bind(&Manager::monitor, this), monitorInterval}
{
    // Obtain D-Bus service name
    bus.request_name(POWER_IFACE);

    findConfigFile();

    // Call monitor() once right away rather than waiting for the timer
    monitor();
}

void Manager::compatibleSystemTypesFound(const std::vector<std::string>& types)
{
    // Exit if we already found a config file or list of system types is empty
    if (!configFilePath.empty() || types.empty())
    {
        return;
    }

    std::string typesStr = format_utils::toString(std::span{types});
    services.logInfoMsg(
        std::format("Compatible system types found: {}", typesStr));

    // Try to find a config file that matches one of the compatible system types
    try
    {
        configFilePath = config_file_parser::find(types);
        if (!configFilePath.empty())
        {
            loadConfigFile();
        }
    }
    catch (const std::exception& e)
    {
        services.logErrorMsg(std::format(
            "Unable to find JSON configuration file: {}", e.what()));
    }
}

void Manager::monitor()
{
    try
    {
        // If config file has been found/loaded and System object created
        if (system)
        {
            system->monitor(services);
        }
    }
    catch (const std::exception& e)
    {
        services.logErrorMsg(
            std::format("Unable to monitor system: {}", e.what()));
    }
}

void Manager::findConfigFile()
{
    if (SEQUENCER_USE_DEFAULT_CONFIG_FILE)
    {
        // Load default config file
        configFilePath = config_file_parser::getDefaultConfigFilePath();
        loadConfigFile();
    }
    else
    {
        // Find D-Bus Compatible interface. Use that data to find config file.
        compatSysTypesFinder =
            std::make_unique<util::CompatibleSystemTypesFinder>(
                bus,
                std::bind_front(&Manager::compatibleSystemTypesFound, this));
    }
}

void Manager::loadConfigFile()
{
    try
    {
        services.logInfoMsg(std::format("Loading JSON configuration file: {}",
                                        configFilePath.string()));
        std::vector<std::unique_ptr<Chassis>> chassis =
            config_file_parser::parse(configFilePath);
        system = std::make_unique<System>(std::move(chassis));
        system->initializeMonitoring(services);
    }
    catch (const std::exception& e)
    {
        services.logErrorMsg(std::format(
            "Unable to parse JSON configuration file: {}", e.what()));
    }
}

} // namespace phosphor::power::sequencer
