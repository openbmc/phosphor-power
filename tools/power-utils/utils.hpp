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

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility> // for std::pair

/**
 * @namespace utils
 *
 * Contains utility functions used within the psutils tool.
 */
namespace utils
{

// PsuI2cInfo contains the device i2c bus and i2c address
using PsuI2cInfo = std::tuple<std::uint64_t, std::uint64_t>;

/**
 * @brief Get i2c bus and address
 *
 * @param[in] bus - Systemd bus connection
 * @param[in] psuInventoryPath - The PSU inventory path.
 *
 * @return tuple - i2cBus and i2cAddr.
 */
PsuI2cInfo getPsuI2c(sdbusplus::bus_t& bus,
                     const std::string& psuInventoryPath);

/**
 * @brief Get PMBus interface pointer
 *
 * @param[in] i2cBus - PSU i2c bus
 * @param[in] i2cAddr - PSU i2c address
 *
 * @return Pointer to PSU PMBus interface
 */
std::unique_ptr<phosphor::pmbus::PMBusBase> getPmbusIntf(std::uint64_t i2cBus,
                                                         std::uint64_t i2cAddr);

/**
 * @brief Reads a VPD value from PMBus, corrects size, and contents.
 *
 * If the VPD data read is not the passed in size, resize and fill with
 * spaces. If the data contains a non-alphanumeric value, replace any of
 * those values with spaces.
 *
 * @param[in] pmbusIntf - PMBus Interface.
 * @param[in] vpdName - The name of the sysfs "file" to read data from.
 * @param[in] type - The HWMON file type to read from.
 * @param[in] vpdSize - The expected size of the data for this VPD/property
 *
 * @return A string containing the VPD data read, resized if necessary
 */
std::string readVPDValue(phosphor::pmbus::PMBusBase& pmbusIntf,
                         const std::string& vpdName,
                         const phosphor::pmbus::Type& type,
                         const std::size_t& vpdSize);

/**
 * @brief Check for file existence
 *
 * @param[in] filePath - File path
 *
 * @return bool
 */
bool checkFileExists(const std::string& filePath);

/**
 * @brief Get the device name from the device path
 *
 * @param[in] devPath - PSU path
 *
 * @return device name e.g. 3-0068
 */
std::string getDeviceName(std::string devPath);

/**
 * @brief Function to get device path using DBus bus and PSU
 * inventory Path
 *
 * @param[in] bus - The sdbusplus DBus bus connection
 * @param[in] psuInventoryPath - PSU inventory path
 *
 * @return device path e.g. /sys/bus/i2c/devices/3-0068
 */
std::string getDevicePath(sdbusplus::bus_t& bus,
                          const std::string& psuInventoryPath);

/**
 * @brief Parse the device name to obtain bus and device address
 *
 * @param[in] devName - Device name
 *
 * @return bus and device address
 */
std::pair<uint8_t, uint8_t> parseDeviceName(const std::string& devName);

/**
 * @brief Wrapper to check existence of PSU JSON file
 *
 * @return true or false (true if using JSON file)
 */
bool usePsuJsonFile();

} // namespace utils
