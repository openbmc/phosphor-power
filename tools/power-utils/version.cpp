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
#include "config.h"

#include "version.hpp"

#include "pmbus.hpp"
#include "utility.hpp"
#include "utils.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <exception>
#include <tuple>

using json = nlohmann::json;

using namespace utils;
using namespace phosphor::power::util;

namespace version
{

namespace internal
{

// PsuInfo contains the device path, PMBus access type, and sysfs file name
using PsuVersionInfo =
    std::tuple<std::string, phosphor::pmbus::Type, std::string>;

/**
 * @brief Get PSU version information
 *
 * @param[in] psuInventoryPath - The PSU inventory path.
 *
 * @return tuple - device path, PMBus access type, and sysfs file name
 */
PsuVersionInfo getVersionInfo(const std::string& psuInventoryPath)
{
    auto data = loadJSONFromFile(PSU_JSON_PATH);

    if (data == nullptr)
    {
        return {};
    }

    auto devices = data.find("psuDevices");
    if (devices == data.end())
    {
        lg2::warning("Unable to find psuDevices");
        return {};
    }
    auto devicePath = devices->find(psuInventoryPath);
    if (devicePath == devices->end())
    {
        lg2::warning("Unable to find path for PSU PATH={PATH}", "PATH",
                     psuInventoryPath);
        return {};
    }

    auto type = getPMBusAccessType(data);

    std::string fileName;
    for (const auto& fru : data["fruConfigs"])
    {
        if (fru.contains("propertyName") &&
            (fru["propertyName"] == "Version") && fru.contains("fileName"))
        {
            fileName = fru["fileName"];
            break;
        }
    }
    if (fileName.empty())
    {
        lg2::warning("Unable to find Version file");
        return {};
    }
    return std::make_tuple(*devicePath, type, fileName);
}

/**
 * @brief Get the PSU version from sysfs.
 *
 * Obtain PSU information from the PSU JSON file.
 *
 * Throws an exception if an error occurs.
 *
 * @param[in] psuInventoryPath - PSU D-Bus inventory path
 *
 * @return PSU version, or "" if none found
 */
std::string getVersionJson(const std::string& psuInventoryPath)
{
    // Get PSU device path, PMBus access type, and sysfs file name from JSON
    const auto [devicePath, type, fileName] = getVersionInfo(psuInventoryPath);

    // Read version from sysfs file
    std::string version;
    if (!devicePath.empty() && !fileName.empty())
    {
        phosphor::pmbus::PMBus pmbus(devicePath);
        version = pmbus.readString(fileName, type);
    }
    return version;
}

/**
 * @brief Get the PSU version from sysfs.
 *
 * Obtain PSU information from D-Bus.
 *
 * Throws an exception if an error occurs.
 *
 * @param[in] bus - D-Bus connection
 * @param[in] psuInventoryPath - PSU D-Bus inventory path
 *
 * @return PSU version, or "" if none found
 */
std::string getVersionDbus(sdbusplus::bus_t& bus,
                           const std::string& psuInventoryPath)
{
    // Get PSU I2C bus/address and create PMBus interface
    const auto [i2cbus, i2caddr] = getPsuI2c(bus, psuInventoryPath);
    auto pmbus = getPmbusIntf(i2cbus, i2caddr);

    // Read version from sysfs file
    std::string name = "fw_version";
    auto type = phosphor::pmbus::Type::HwmonDeviceDebug;
    std::string version = pmbus->readString(name, type);
    return version;
}

/**
 * @brief Get firmware latest version
 *
 * @param[in] versions - String of versions
 *
 * @return version - latest firmware level
 */
std::string getLatestDefault(const std::vector<std::string>& versions)
{
    std::string latest;
    for (const auto& version : versions)
    {
        if (latest < version)
        {
            latest = version;
        }
    }
    return latest;
}

} // namespace internal

std::string getVersion(sdbusplus::bus_t& bus,
                       const std::string& psuInventoryPath)
{
    std::string version;
    try
    {
        if (usePsuJsonFile())
        {
            // Obtain PSU information from JSON file
            version = internal::getVersionJson(psuInventoryPath);
        }
        else
        {
            // Obtain PSU information from D-Bus
            version = internal::getVersionDbus(bus, psuInventoryPath);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Error in getVersion: {ERROR}", "ERROR", e);
    }
    return version;
}

std::string getLatest(const std::vector<std::string>& versions)
{
    // TODO: when multiple PSU/Machines are supported, add configuration options
    // to implement machine-specific logic.
    // For now IBM AC Servers and Inspur FP5280G2 are supported.
    //
    // IBM AC servers' PSU version has two types:
    // * XXXXYYYYZZZZ: XXXX is the primary version
    //                 YYYY is the secondary version
    //                 ZZZZ is the communication version
    //
    // * XXXXYYYY:     XXXX is the primary version
    //                 YYYY is the seconday version
    //
    // Inspur FP5280G2 PSU version is human readable text and a larger string
    // means a newer version.
    //
    // So just compare by strings is OK for these cases
    return internal::getLatestDefault(versions);
}

} // namespace version
