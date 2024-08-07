/**
 * Copyright © 2024 IBM Corporation
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
#include "utility.hpp"
#include "version.hpp"

#include <sdbusplus/bus.hpp>

#include <string>
#include <tuple>
#include <vector>

namespace utils
{
using PmbusInfo = phosphor::pmbus::PMBus;
using PsuI2cInfo = std::tuple<std::uint64_t, std::uint64_t>;

/**
 * @brief Retrieve i2c bus and address
 *
 * @param[in] bus - Systemd bus member
 * @param[in] psuInventoryPath - The PSU inventory path.
 *
 * @return tuple - i2cBus and i2cAddr.
 */
PsuI2cInfo getPSUI2c(sdbusplus::bus_t& bus,
                     const std::string& psuInventoryPath);

/**
 * @brief Obtain PMBus interface pointer
 *
 * @param[in] i2cBus - PSU i2c bus
 * @param[in] i2cAddr - PSU i2c address
 *
 * @return Pointer to PSU PMBus interface
 */
std::unique_ptr<phosphor::pmbus::PMBusBase> getPmbusIntf(std::uint64_t i2cBus,
                                                         std::uint64_t i2cAddr);

/**
 * @brief Reads a VPD value from PMBus, correct size, and contents.
 *
 * If the VPD data read is not the passed in size, resize and fill with
 * spaces. If the data contains a non-alphanumeric value, replace any of
 * those values with spaces.
 *
 * @param[in] pmbusIntf - PMBus Interface.
 * @param[in] vpdName - The name of the sysfs "file" to read data from.
 * @param[in] type - The HWMON file type to read from.
 * @param[in] vpdSize - The expacted size of the data for this VPD/property
 *
 * @return A string containing the VPD data read, resized if necessary
 */
std::string readVPDValue(phosphor::pmbus::PMBusBase* pmbusIntf,
                         const std::string& vpdName,
                         const phosphor::pmbus::Type& type,
                         const std::size_t& vpdSize);

} // namespace utils
namespace version
{

/**
 * Get the software version of the PSU
 *
 * @param[in] bus - Systemd bus member
 * @param[in] psuInventoryPath - The inventory path of the PSU
 *
 * @return The version of the PSU
 */
std::string getVersion(sdbusplus::bus_t& bus,
                       const std::string& psuInventoryPath);

/**
 * Get the latest version from a list of versions
 *
 * @param[in] versions - The list of PSU version strings
 *
 * @return The latest version
 */
std::string getLatest(const std::vector<std::string>& versions);

} // namespace version
