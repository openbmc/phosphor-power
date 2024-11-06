/**
 * Copyright © 2019 IBM Corporation
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

#include "pmbus.hpp"

#include <sdbusplus/bus.hpp>

#include <string>
#include <tuple>
#include <vector>

namespace version
{
namespace internal
{
// PsuInfo contains the device path, pmbus read type, and the version string
using PsuVersionInfo =
    std::tuple<std::string, phosphor::pmbus::Type, std::string>;

/**
 * @brief Get PSU version information
 *
 * @param[in] psuInventoryPath - The PSU inventory path.
 *
 * @return tuple - device path, pmbus read type and PSU version
 */
PsuVersionInfo getVersionInfo(const std::string& psuInventoryPath);

/**
 * @brief Get firmware latest version
 *
 * @param[in] versions - String of versions
 *
 * @return version - latest firmware level
 */
std::string getLatestDefault(const std::vector<std::string>& versions);

} // namespace internal

/**
 * Get the software version of the PSU using sysfs
 *
 * @param[in] bus - Systemd bus connection
 * @param[in] psuInventoryPath - The inventory path of the PSU
 *
 * @return The version of the PSU
 */
std::string getVersion(sdbusplus::bus_t& bus,
                       const std::string& psuInventoryPath);

/**
 * Get the software version of the PSU using psu.json
 *
 * @param[in] psuInventoryPath - The inventory path of the PSU
 *
 * @return The version of the PSU
 */
std::string getVersion(const std::string& psuInventoryPath);

/**
 * Get the latest version from a list of versions
 *
 * @param[in] versions - The list of PSU version strings
 *
 * @return The latest version
 */
std::string getLatest(const std::vector<std::string>& versions);

} // namespace version
