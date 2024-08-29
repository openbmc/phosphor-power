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

#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <exception>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <tuple>

using json = nlohmann::json;

using namespace phosphor::logging;

using namespace phosphor::power::util;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;

namespace version
{
namespace utils
{
constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto i2cBusProp = "I2CBus";
constexpr auto i2cAddressProp = "I2CAddress";

PsuVersionInfo getVersionInfo(const std::string& psuInventoryPath)
{
    auto data = phosphor::power::util::loadJSONFromFile(PSU_JSON_PATH);

    if (data == nullptr)
    {
        return {};
    }

    auto devices = data.find("psuDevices");
    if (devices == data.end())
    {
        log<level::WARNING>("Unable to find psuDevices");
        return {};
    }
    auto devicePath = devices->find(psuInventoryPath);
    if (devicePath == devices->end())
    {
        log<level::WARNING>("Unable to find path for PSU",
                            entry("PATH=%s", psuInventoryPath.c_str()));
        return {};
    }

    auto type = phosphor::power::util::getPMBusAccessType(data);

    std::string versionStr;
    for (const auto& fru : data["fruConfigs"])
    {
        if (fru["propertyName"] == "Version")
        {
            versionStr = fru["fileName"].get<std::string>();
            break;
        }
    }
    if (versionStr.empty())
    {
        log<level::WARNING>("Unable to find Version file");
        return {};
    }
    return std::make_tuple(*devicePath, type, versionStr);
}

// A default implemention compare the string itself
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

std::string getVersion(const std::string& psuInventoryPath)
{
    const auto& [devicePath, type, versionStr] =
        utils::getVersionInfo(psuInventoryPath);
    if (devicePath.empty() || versionStr.empty())
    {
        return "";
    }
    std::string version;
    try
    {
        phosphor::pmbus::PMBus pmbus(devicePath);
        version = pmbus.readString(versionStr, type);
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(ex.what());
    }
    return version;
}

std::string getVersion(sdbusplus::bus_t& bus,
                       const std::string& psuInventoryPath)
{
    try
    {
        constexpr auto FW_VERSION = "fw_version";
        using namespace phosphor::pmbus;
        const auto IBMCFFPS_FW_VERSION_SIZE = 12;
        const auto& [i2cbus, i2caddr] = utils::getPsuI2c(bus, psuInventoryPath);

        auto pmbusIntf = utils::getPmbusIntf(i2cbus, i2caddr);

        if (!pmbusIntf)
        {
            log<level::WARNING>("Unable to get pointer PMBus Interface");
            return "";
        }

        std::string fwVersion =
            utils::readVPDValue(*pmbusIntf, FW_VERSION, Type::HwmonDeviceDebug,
                                IBMCFFPS_FW_VERSION_SIZE);
        return fwVersion;
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(std::format("Error: {}", e.what()).c_str());
        return "";
    }
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
    return utils::getLatestDefault(versions);
}

std::string getDifferentVersion(const std::vector<std::string>& versions)
{
    try
    {
        if (versions.size() == 2)
        {
            return (versions[0] != versions[1]) ? versions[1] : versions[0];
        }
        else
        {
            throw std::runtime_error(
                "GetDifferentVersion requires two versions to compare");
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(std::format("Error: {}", e.what()).c_str());
        return "";
    }
}
} // namespace version
