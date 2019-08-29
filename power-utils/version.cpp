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

using json = nlohmann::json;

using namespace witherspoon::pmbus;
using namespace phosphor::logging;

namespace utils
{
std::string getDevicePath(const std::string& psuInventoryPath)
{
    auto data =
        witherspoon::power::util::loadJsonFromFile(PSU_UTILS_CONFIG_FILE);

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
    auto p = devices->find(psuInventoryPath);
    if (p == devices->end())
    {
        log<level::WARNING>("Unable to find path for PSU",
                            entry("PATH=%s", psuInventoryPath.c_str()));
        return {};
    }
    return *p;
}
} // namespace utils

namespace version
{
constexpr auto FW_VERSION = "fw_version";

std::string getVersion(const std::string& psuInventoryPath)
{
    auto devicePath = utils::getDevicePath(psuInventoryPath);
    if (devicePath.empty())
    {
        return {};
    }
    witherspoon::pmbus::PMBus pmbus(devicePath);
    auto version = pmbus.readString(FW_VERSION, Type::HwmonDeviceDebug);
    return version;
}

} // namespace version
