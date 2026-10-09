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

#include "psu-fetcher.hpp"

#include "types.hpp"
#include "utility.hpp"

#include <format>
#include <print>
#include <ranges>

namespace
{
constexpr auto inventoryService = INVENTORY_MGR_IFACE;
constexpr auto psuMonitorService = "xyz.openbmc_project.Power.PSUMonitor";

constexpr auto sensorValueIface = "xyz.openbmc_project.Sensor.Value";

bool isPropEnabled(const std::map<std::string, bool>& propMap,
                   const std::string& key)
{
    auto pos = propMap.find(key);
    return (pos != propMap.end() && pos->second);
}

} // namespace

PsuFetcher::PsuFetcher(sdbusplus::bus_t& bus, bool isVerbose,
                       const std::map<std::string, bool>& psuPropMap) :
    bus(bus), isVerbose(isVerbose), psuPropMap(psuPropMap)
{
    try
    {
        psuPaths = phosphor::power::util::getPSUInventoryPaths(bus);
        isMulti = phosphor::power::util::isMultiChassis(bus);
    }
    catch (const std::exception& e)
    {
        // Leave psuPaths empty so display() reports no PSUs instead of
        // aborting the whole tool.
        if (isVerbose)
        {
            std::println(stderr, "{}PSU: failed to query inventory: {}",
                         smallIndent, e.what());
        }
    }
}

bool PsuFetcher::hasEnabledProperties() const
{
    return std::ranges::any_of(psuPropMap, [](const auto& entry) {
        return entry.second;
    });
}

template <typename T>
void PsuFetcher::queryProperty(
    const std::string& interface, const std::string& property,
    const std::string& path, const std::string& service,
    std::optional<T>& value, std::string& error) const
{
    try
    {
        T result{};
        phosphor::power::util::getProperty<T>(interface, property, path,
                                              service, bus, result);
        value = result;
    }
    catch (const std::exception& e)
    {
        error = e.what();
    }
}

std::vector<std::string> PsuFetcher::filterPsuPaths(int chassisNumber) const
{
    std::vector<std::string> chassisPsuPaths;
    const std::string specificChassisToken =
        std::format("chassis{}/", chassisNumber);

    for (const auto& path : psuPaths)
    {
        if (isMulti)
        {
            if (path.find(specificChassisToken) != std::string::npos)
            {
                chassisPsuPaths.push_back(path);
            }
        }
        else
        {
            if (path.find("system/chassis/") != std::string::npos ||
                path.find(specificChassisToken) != std::string::npos)
            {
                chassisPsuPaths.push_back(path);
            }
        }
    }

    std::ranges::sort(chassisPsuPaths);
    return chassisPsuPaths;
}

std::vector<PsuConfig> PsuFetcher::getPsuConfigsForChassis(
    int chassisNumber) const
{
    std::vector<PsuConfig> configs;

    if (psuPaths.empty())
    {
        return configs;
    }

    for (const auto& psuPath : filterPsuPaths(chassisNumber))
    {
        PsuConfig config{};
        config.inventoryPath = psuPath;

        // Parse short name and slot number (e.g., powersupply0 -> slot 0)
        auto lastSlash = psuPath.find_last_of('/');
        std::string shortName = (lastSlash != std::string::npos)
                                    ? psuPath.substr(lastSlash + 1)
                                    : psuPath;
        config.name = shortName;

        // Query Present
        if (isPropEnabled(psuPropMap, "Present"))
        {
            queryProperty(INVENTORY_IFACE, "Present", psuPath, inventoryService,
                          config.present, config.presentError);
        }

        // Query Model
        if (isPropEnabled(psuPropMap, "Model"))
        {
            queryProperty(ASSET_IFACE, "Model", psuPath, inventoryService,
                          config.model, config.modelError);
        }

        // Query Input Voltage Rating
        if (isPropEnabled(psuPropMap, "InputVoltageRating"))
        {
            auto digitsStart = config.name.find_last_not_of("0123456789") + 1;
            if (digitsStart == config.name.size())
            {
                config.voltageRatingError = "PSU name has no numeric suffix";
            }
            else
            {
                int slot = std::stoi(config.name.substr(digitsStart));

                if (isMulti)
                {
                    config.voltageRatingSensorPath = std::format(
                        "/xyz/openbmc_project/sensors/voltage/chassis{}_ps{}_input_voltage_rating",
                        chassisNumber, slot);
                }
                else
                {
                    config.voltageRatingSensorPath = std::format(
                        "/xyz/openbmc_project/sensors/voltage/ps{}_input_voltage_rating",
                        slot);
                }

                queryProperty(sensorValueIface, "Value",
                              config.voltageRatingSensorPath, psuMonitorService,
                              config.inputVoltageRating,
                              config.voltageRatingError);
            }
        }

        configs.push_back(config);
    }

    return configs;
}

void PsuFetcher::printProperty(
    const std::string& label, const std::optional<std::string>& value,
    const std::string& error, const std::string& path,
    const std::string& interface) const
{
    std::print("{}{}: ", mediumIndent, label);
    if (value.has_value())
    {
        std::println("{}", *value);
    }
    else
    {
        std::println("Unknown");
        if (isVerbose && !error.empty())
        {
            std::println(stderr, "{}{}", largeIndent, error);
        }
    }

    if (isVerbose)
    {
        std::println("{}Object Path: {}", largeIndent, path);
        std::println("{}Interface: {}", largeIndent, interface);
    }
}

void PsuFetcher::display(int chassisNumber) const
{
    if (!hasEnabledProperties())
    {
        return;
    }

    if (psuPaths.empty())
    {
        if (isVerbose)
        {
            std::println(stderr, "{}PSU: no inventory paths found",
                         smallIndent);
        }
        return;
    }

    auto psuConfigs = getPsuConfigsForChassis(chassisNumber);
    if (psuConfigs.empty())
    {
        if (isVerbose)
        {
            std::println(stderr, "{}PSU: no PSUs found for chassis {}",
                         smallIndent, chassisNumber);
        }
        return;
    }

    for (const auto& psu : psuConfigs)
    {
        std::println("{}{}:", smallIndent, psu.name);

        if (isPropEnabled(psuPropMap, "Present"))
        {
            std::optional<std::string> present;
            if (psu.present.has_value())
            {
                present = *psu.present ? "True" : "False";
            }
            printProperty("Present", present, psu.presentError,
                          psu.inventoryPath, INVENTORY_IFACE);
        }

        if (isPropEnabled(psuPropMap, "Model"))
        {
            printProperty("Model", psu.model, psu.modelError, psu.inventoryPath,
                          ASSET_IFACE);
        }

        if (isPropEnabled(psuPropMap, "InputVoltageRating"))
        {
            std::optional<std::string> voltage;
            if (psu.inputVoltageRating.has_value())
            {
                voltage = std::format("{:.0f} V", *psu.inputVoltageRating);
            }
            printProperty("Input Voltage Rating", voltage,
                          psu.voltageRatingError, psu.voltageRatingSensorPath,
                          sensorValueIface);
        }
    }
}
