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
#include "utils.hpp"

#include "utility.hpp"

#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <exception>
#include <iostream>
#include <regex>
#include <stdexcept>

using namespace phosphor::logging;
using namespace phosphor::power::util;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;

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
                    log<level::WARNING>(
                        std::format("Error reading property {}: {}",
                                    property.first, e.what())
                            .c_str());
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

std::unique_ptr<phosphor::pmbus::PMBusBase>
    getPmbusIntf(std::uint64_t i2cBus, std::uint64_t i2cAddr)
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
        log<level::INFO>(
            std::format(" {} resize needed. size: {}", vpdName, vpdValue.size())
                .c_str());
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
        log<level::ERR>(std::format("Unable to check for existence of {}: {}",
                                    filePath, e.what())
                            .c_str());
    }
    return false;
}

} // namespace utils
