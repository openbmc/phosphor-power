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

    auto devices = data.find("PSU_DEVICES");
    if (devices == data.end())
    {
        log<level::WARNING>("Unable to find PSU_DEVICES");
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
    for (const auto& fru : data.at("fruConfigs"))
    {
        if (fru.at("propertyName") == "Version")
        {
            versionStr = fru.at("fileName");
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
} // namespace utils

namespace version
{
constexpr auto FW_VERSION = "fw_version";

std::string getVersion(const std::string& psuInventoryPath)
{
    const auto& [devicePath, type, versionStr] =
        utils::getVersionInfo(psuInventoryPath);
    if (devicePath.empty() || versionStr.empty())
    {
        return {};
    }
    phosphor::pmbus::PMBus pmbus(devicePath);
    auto version = pmbus.readString(versionStr, type);
    return version;
}

} // namespace version
