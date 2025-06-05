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

#include "utils.hpp"

#include "utility.hpp"

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <cassert>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <ios>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

using namespace phosphor::power::util;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;
namespace fs = std::filesystem;

namespace utils
{

constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto i2cBusProp = "I2CBus";
constexpr auto i2cAddressProp = "I2CAddress";

PsuI2cInfo getPsuI2c(sdbusplus::bus_t& bus, const std::string& psuInventoryPath)
{
    auto depth = 0;
    auto objects = getSubTree(bus, "/", IBMCFFPSInterface, depth);
    if (objects.empty())
    {
        throw std::runtime_error("Supported Configuration Not Found");
    }

    std::optional<std::uint64_t> i2cbus;
    std::optional<std::uint64_t> i2caddr;

    // GET a map of objects back.
    // Each object will have a path, a service, and an interface.
    for (const auto& [path, services] : objects)
    {
        auto service = services.begin()->first;

        if (path.empty() || service.empty())
        {
            continue;
        }

        // Match the PSU identifier in the path with the passed PSU inventory
        // path. Compare the last character of both paths to find the PSU bus
        // and address. example: PSU path:
        // /xyz/openbmc_project/inventory/system/board/Nisqually_Backplane/Power_Supply_Slot_0
        // PSU inventory path:
        // /xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0
        if (path.back() == psuInventoryPath.back())
        {
            // Retrieve i2cBus and i2cAddress from array of properties.
            auto properties =
                getAllProperties(bus, path, IBMCFFPSInterface, service);
            for (const auto& property : properties)
            {
                try
                {
                    if (property.first == i2cBusProp)
                    {
                        i2cbus = std::get<uint64_t>(properties.at(i2cBusProp));
                    }
                    else if (property.first == i2cAddressProp)
                    {
                        i2caddr =
                            std::get<uint64_t>(properties.at(i2cAddressProp));
                    }
                }
                catch (const std::exception& e)
                {
                    lg2::warning("Error reading property {PROPERTY}: {ERROR}",
                                 "PROPERTY", property.first, "ERROR", e);
                }
            }

            if (i2cbus.has_value() && i2caddr.has_value())
            {
                break;
            }
        }
    }

    if (!i2cbus.has_value() || !i2caddr.has_value())
    {
        throw std::runtime_error("Failed to get I2C bus or address");
    }

    return std::make_tuple(*i2cbus, *i2caddr);
}

std::unique_ptr<phosphor::pmbus::PMBusBase> getPmbusIntf(std::uint64_t i2cBus,
                                                         std::uint64_t i2cAddr)
{
    std::stringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << i2cAddr;
    return phosphor::pmbus::createPMBus(i2cBus, ss.str());
}

std::string readVPDValue(phosphor::pmbus::PMBusBase& pmbusIntf,
                         const std::string& vpdName,
                         const phosphor::pmbus::Type& type,
                         const std::size_t& vpdSize)
{
    std::string vpdValue;
    const std::regex illegalVPDRegex =
        std::regex("[^[:alnum:]]", std::regex::basic);

    try
    {
        vpdValue = pmbusIntf.readString(vpdName, type);
    }
    catch (const ReadFailure& e)
    {
        // Ignore the read failure, let pmbus code indicate failure.
    }

    if (vpdValue.size() != vpdSize)
    {
        lg2::info(" {VPDNAME} resize needed. size: {SIZE}", "VPDNAME", vpdName,
                  "SIZE", vpdValue.size());
        vpdValue.resize(vpdSize, ' ');
    }

    // Replace any illegal values with space(s).
    std::regex_replace(vpdValue.begin(), vpdValue.begin(), vpdValue.end(),
                       illegalVPDRegex, " ");

    return vpdValue;
}

bool checkFileExists(const std::string& filePath)
{
    try
    {
        return std::filesystem::exists(filePath);
    }
    catch (const std::exception& e)
    {
        lg2::error("Unable to check for existence of {FILEPATH}: {ERROR}",
                   "FILEPATH", filePath, "ERROR", e);
    }
    return false;
}

std::string getDeviceName(std::string devPath)
{
    if (devPath.empty())
    {
        return devPath;
    }
    if (devPath.back() == '/')
    {
        devPath.pop_back();
    }
    return fs::path(devPath).stem().string();
}

std::string getDevicePath(sdbusplus::bus_t& bus,
                          const std::string& psuInventoryPath)
{
    try
    {
        if (usePsuJsonFile())
        {
            auto data = loadJSONFromFile(PSU_JSON_PATH);
            if (data == nullptr)
            {
                return {};
            }
            auto devicePath = data["psuDevices"][psuInventoryPath];
            if (devicePath.empty())
            {
                lg2::warning("Unable to find psu devices or path");
            }
            return devicePath;
        }
        else
        {
            const auto [i2cbus, i2caddr] = getPsuI2c(bus, psuInventoryPath);
            const auto DevicePath = "/sys/bus/i2c/devices/";
            std::ostringstream ss;
            ss << std::hex << std::setw(4) << std::setfill('0') << i2caddr;
            std::string addrStr = ss.str();
            std::string busStr = std::to_string(i2cbus);
            std::string devPath = DevicePath + busStr + "-" + addrStr;
            return devPath;
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Error in getDevicePath: {ERROR}", "ERROR", e);
        return {};
    }
    catch (...)
    {
        lg2::error("Unknown error occurred in getDevicePath");
        return {};
    }
}

std::pair<uint8_t, uint8_t> parseDeviceName(const std::string& devName)
{
    // Get I2C bus and device address, e.g. 3-0068
    // is parsed to bus 3, device address 0x68
    auto pos = devName.find('-');
    assert(pos != std::string::npos);
    uint8_t busId = std::stoi(devName.substr(0, pos));
    uint8_t devAddr = std::stoi(devName.substr(pos + 1), nullptr, 16);
    return {busId, devAddr};
}

bool usePsuJsonFile()
{
    return checkFileExists(PSU_JSON_PATH);
}

} // namespace utils
