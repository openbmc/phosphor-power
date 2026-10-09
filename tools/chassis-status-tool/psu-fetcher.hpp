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
     * @param[in] chassisNumber  Chassis number
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
    /**
     * Reads a D-Bus property and stores the value and potential error value.
     *
     * @param[in] interface  D-Bus interface of the property.
     * @param[in] property  Property name.
     * @param[in] path  D-Bus object path.
     * @param[in] service  D-Bus service that owns the object.
     * @param[out] value  The property value, if the read succeeded.
     * @param[out] error  The failure message, if the read failed.
     */
    template <typename T>
    void queryProperty(const std::string& interface,
                       const std::string& property, const std::string& path,
                       const std::string& service, std::optional<T>& value,
                       std::string& error) const;

    /**
     * Prints one PSU property line. Prints "Unknown" if value is empty
     * with the error if verbose. If verbose, also prints any error message,
     * the object path and interface.
     *
     * @param[in] label  Property label to display.
     * @param[in] value  Formatted property value, or empty if it was not read.
     * @param[in] error  Error message from the failed read, if any.
     * @param[in] path  D-Bus object path the property was read from.
     * @param[in] interface  D-Bus interface the property was read from.
     */
    void printProperty(const std::string& label,
                       const std::optional<std::string>& value,
                       const std::string& error, const std::string& path,
                       const std::string& interface) const;

    /**
     * Returns the sorted cached PSU inventory paths that belong to the given
     * chassis.
     *
     * @param[in] chassisNumber  Chassis number used to filter PSU paths.
     */
    std::vector<std::string> filterPsuPaths(int chassisNumber) const;

    /**
     * Returns a populated PsuConfig for each PSU belonging to the
     * given chassis.
     *
     * Filters the cached inventory paths to those matching the given
     * chassisNumber, sorts them, then queries D-Bus for each enabled property.
     * Query errors are stored in the corresponding error fields.
     *
     * @param[in] chassisNumber  Chassis number used to filter PSU paths.
     * @return Vector of PsuConfigs, one entry per matching PSU. Returns an
     * empty vector if no PSU paths are found.
     */
    std::vector<PsuConfig> getPsuConfigsForChassis(int chassisNumber) const;

    // D-Bus connection
    sdbusplus::bus_t& bus;

    // Flag for verbose output.
    bool isVerbose{false};

    // Map of property name to enabled flag.
    std::map<std::string, bool> psuPropMap;

    // All PSU inventory paths, cached at construction.
    std::vector<std::string> psuPaths;

    // True if system has multiple chassis.
    bool isMulti{false};
};
