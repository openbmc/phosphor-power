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
#include "config.h"

#include "model.hpp"

#include "pmbus.hpp"
#include "utility.hpp"
#include "utils.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <exception>
#include <format>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

using namespace utils;
using namespace phosphor::logging;
using namespace phosphor::power::util;

namespace model
{

namespace internal
{

/**
 * @brief Get the PSU model from sysfs.
 *
 * Obtain PSU information from the PSU JSON file.
 *
 * Throws an exception if an error occurs.
 *
 * @param[in] psuInventoryPath - PSU D-Bus inventory path
 *
 * @return PSU model
 */
std::string getModelJson(const std::string& psuInventoryPath)
{
    // Parse PSU JSON file
    std::ifstream file{PSU_JSON_PATH};
    json data = json::parse(file);

    // Get PSU device path from JSON
    auto it = data.find("psuDevices");
    if (it == data.end())
    {
        throw std::runtime_error{"Unable to find psuDevices"};
    }
    auto device = it->find(psuInventoryPath);
    if (device == it->end())
    {
        throw std::runtime_error{std::format(
            "Unable to find device path for PSU {}", psuInventoryPath)};
    }
    std::string devicePath = *device;
    if (devicePath.empty())
    {
        throw std::runtime_error{
            std::format("Empty device path for PSU {}", psuInventoryPath)};
    }

    // Get sysfs filename from JSON for Model information
    it = data.find("fruConfigs");
    if (it == data.end())
    {
        throw std::runtime_error{"Unable to find fruConfigs"};
    }
    std::string fileName;
    for (const auto& fru : *it)
    {
        if (fru.contains("propertyName") && (fru["propertyName"] == "Model") &&
            fru.contains("fileName"))
        {
            fileName = fru["fileName"];
            break;
        }
    }
    if (fileName.empty())
    {
        throw std::runtime_error{"Unable to find file name for Model"};
    }

    // Get PMBus access type from JSON
    phosphor::pmbus::Type type = getPMBusAccessType(data);

    // Read model from sysfs file
    phosphor::pmbus::PMBus pmbus(devicePath);
    std::string model = pmbus.readString(fileName, type);
    return model;
}

/**
 * @brief Get the PSU model from sysfs.
 *
 * Obtain PSU information from D-Bus.
 *
 * Throws an exception if an error occurs.
 *
 * @param[in] bus - D-Bus connection
 * @param[in] psuInventoryPath - PSU D-Bus inventory path
 *
 * @return PSU model
 */
std::string getModelDbus(sdbusplus::bus_t& bus,
                         const std::string& psuInventoryPath)
{
    // Get PSU I2C bus/address and create PMBus interface
    const auto [i2cBus, i2cAddr] = getPsuI2c(bus, psuInventoryPath);
    auto pmbus = getPmbusIntf(i2cBus, i2cAddr);

    // Read model from sysfs file
    std::string fileName = "ccin";
    auto type = phosphor::pmbus::Type::HwmonDeviceDebug;
    std::string model = pmbus->readString(fileName, type);
    return model;
}

} // namespace internal

std::string getModel(sdbusplus::bus_t& bus, const std::string& psuInventoryPath)
{
    std::string model;
    try
    {
        if (usePsuJsonFile())
        {
            // Obtain PSU information from JSON file
            model = internal::getModelJson(psuInventoryPath);
        }
        else
        {
            // Obtain PSU information from D-Bus
            model = internal::getModelDbus(bus, psuInventoryPath);
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(std::format("Error: {}", e.what()).c_str());
    }
    return model;
}

} // namespace model
