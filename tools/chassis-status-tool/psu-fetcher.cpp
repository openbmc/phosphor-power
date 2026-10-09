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
    return pos != propMap.end() && pos->second;
}
} // namespace

PsuFetcher::PsuFetcher(sdbusplus::bus_t& bus, bool isVerbose,
                       const std::map<std::string, bool>& psuPropMap) :
    bus(bus), verbose(isVerbose), psuPropMap(psuPropMap),
    psuPaths(phosphor::power::util::getPSUInventoryPaths(bus)),
    isMulti(phosphor::power::util::isMultiChassis(bus))
{}

bool PsuFetcher::hasEnabledProperties() const
{
    return std::ranges::any_of(psuPropMap, [](const auto& entry) {
        return entry.second;
    });
}

std::vector<PsuConfig> PsuFetcher::getPsuConfigsForChassis(
    int chassisNumber) const
{
    std::vector<PsuConfig> configs;

    if (psuPaths.empty())
    {
        return configs;
    }

    // Filter paths matching chassisNumber
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

    for (const auto& psuPath : chassisPsuPaths)
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
            try
            {
                bool presentVal = false;
                phosphor::power::util::getProperty<bool>(
                    INVENTORY_IFACE, "Present", psuPath, inventoryService, bus,
                    presentVal);
                config.present = presentVal;
            }
            catch (const std::exception& e)
            {
                config.presentError = e.what();
            }
        }

        // Query Model
        if (isPropEnabled(psuPropMap, "Model"))
        {
            try
            {
                std::string modelVal;
                phosphor::power::util::getProperty<std::string>(
                    ASSET_IFACE, "Model", psuPath, inventoryService, bus,
                    modelVal);
                config.model = modelVal;
            }
            catch (const std::exception& e)
            {
                config.modelError = e.what();
            }
        }

        // Query Input Voltage Rating
        if (isPropEnabled(psuPropMap, "Voltage"))
        {
            int slot = config.name.back() - '0';
            std::string sensorPath;
            if (isMulti)
            {
                sensorPath = std::format(
                    "/xyz/openbmc_project/sensors/voltage/chassis{}_ps{}_input_voltage_rating",
                    chassisNumber, slot);
            }
            else
            {
                sensorPath = std::format(
                    "/xyz/openbmc_project/sensors/voltage/ps{}_input_voltage_rating",
                    slot);
            }
            config.voltageRatingSensorPath = sensorPath;

            try
            {
                double voltageVal = 0.0;
                phosphor::power::util::getProperty<double>(
                    sensorValueIface, "Value", sensorPath, psuMonitorService,
                    bus, voltageVal);
                config.inputVoltageRating = voltageVal;
            }
            catch (const std::exception& e)
            {
                config.voltageRatingError = e.what();
            }
        }

        configs.push_back(config);
    }

    return configs;
}

void PsuFetcher::display(int chassisNumber) const
{
    if (!hasEnabledProperties())
    {
        return;
    }

    if (psuPaths.empty())
    {
        if (verbose)
        {
            std::println(stderr, "{}PSU: no inventory paths found",
                         smallIndent);
        }
        return;
    }

    auto psuConfigs = getPsuConfigsForChassis(chassisNumber);
    if (psuConfigs.empty())
    {
        if (verbose)
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
            std::print("{}Present: ", mediumIndent);
            if (psu.present.has_value())
            {
                std::println("{}", *psu.present ? "True" : "False");
            }
            else
            {
                std::println("Unknown");
                if (verbose && !psu.presentError.empty())
                {
                    std::println(stderr, "{}{}", largeIndent, psu.presentError);
                }
            }

            if (verbose)
            {
                std::println("{}Object Path: {}", largeIndent,
                             psu.inventoryPath);
                std::println("{}Interface: {}", largeIndent, INVENTORY_IFACE);
            }
        }

        if (isPropEnabled(psuPropMap, "Model"))
        {
            std::print("{}Model: ", mediumIndent);
            if (psu.model.has_value())
            {
                std::println("{}", *psu.model);
            }
            else
            {
                std::println("Unknown");
                if (verbose && !psu.modelError.empty())
                {
                    std::println(stderr, "{}{}", largeIndent, psu.modelError);
                }
            }

            if (verbose)
            {
                std::println("{}Object Path: {}", largeIndent,
                             psu.inventoryPath);
                std::println("{}Interface: {}", largeIndent, ASSET_IFACE);
            }
        }

        if (isPropEnabled(psuPropMap, "Voltage"))
        {
            std::print("{}Input Voltage Rating: ", mediumIndent);
            if (psu.inputVoltageRating.has_value())
            {
                std::println("{:.0f} V", *psu.inputVoltageRating);
            }
            else
            {
                std::println("Unknown");
                if (verbose && !psu.voltageRatingError.empty())
                {
                    std::println(stderr, "{}{}", largeIndent,
                                 psu.voltageRatingError);
                }
            }

            if (verbose)
            {
                std::println("{}Object Path: {}", largeIndent,
                             psu.voltageRatingSensorPath);
                std::println("{}Interface: {}", largeIndent, sensorValueIface);
            }
        }
    }
}
