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

#include <tuple>

using json = nlohmann::json;

using namespace phosphor::logging;

// PsuInfo contains the device path, pmbus read type, and the version string
using PsuVersionInfo =
    std::tuple<std::string, phosphor::pmbus::Type, std::string>;

namespace utils
{
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

} // namespace utils

namespace version
{

std::string getVersion(const std::string& psuInventoryPath)
{
    const auto& [devicePath, type, versionStr] =
        utils::getVersionInfo(psuInventoryPath);
    if (devicePath.empty() || versionStr.empty())
    {
        return {};
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

} // namespace version
