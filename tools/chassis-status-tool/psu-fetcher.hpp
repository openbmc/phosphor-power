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

#pragma once

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

/**
 * @struct PsuConfig
 * @brief PSU inventory data and queried status properties.
 */
struct PsuConfig
{
    std::string name;
    std::string inventoryPath;
    std::optional<bool> present;
    std::optional<std::string> model;
    std::string voltageRatingSensorPath;
    std::optional<double> inputVoltageRating;
    std::string presentError;
    std::string modelError;
    std::string voltageRatingError;
};

/** @brief Fetches and displays PSU status properties for a chassis. */
class PsuFetcher
{
  public:
    PsuFetcher(sdbusplus::bus_t& bus, bool isVerbose,
               const std::map<std::string, bool>& psuPropMap = {});

    /**
     * @brief Display PSU configuration properties for the given chassis number.
     * @param[in] chassisNumber  Chassis number (0-based or 1-based matching
     * system).
     */
    void display(int chassisNumber) const;

    /**
     * @brief Returns whether any PSU properties are enabled for monitoring.
     */
    bool hasEnabledProperties() const;

    static constexpr auto smallIndent = "    ";
    static constexpr auto mediumIndent = "       ";
    static constexpr auto largeIndent = "          ";

  private:
    std::vector<PsuConfig> getPsuConfigsForChassis(int chassisNumber) const;

    // D-Bus connection
    sdbusplus::bus_t& bus;

    // Flag for verbose output.
    bool verbose{false};

    // Map of property name to enabled flag.
    std::map<std::string, bool> psuPropMap;

    // All PSU inventory paths, cached at construction.
    std::vector<std::string> psuPaths;

    // True if system has multiple chassis.
    bool isMulti{false};
};
