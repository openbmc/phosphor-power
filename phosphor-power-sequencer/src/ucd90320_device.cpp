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

#include "ucd90320_device.hpp"

#include "format_utils.hpp"
#include "standard_device.hpp"

#include <array>
#include <format>
#include <span>

namespace phosphor::power::sequencer
{

/**
 * Group of GPIO values that should be formatted together.
 */
struct GPIOGroup
{
    std::string additionalDataName;
    std::string journalName;
    unsigned int offset;
    unsigned int count;
};

/**
 * UCD90320-specific groups of GPIO values.
 *
 * The offsets correspond to the Pin IDs defined in the UCD90320 PMBus interface
 * documentation.  These Pin IDs are the same as the libgpiod line offsets used
 * to obtain the GPIO values.
 */
static const std::array<GPIOGroup, 5> gpioGroups = {
    GPIOGroup{"MAR01_24_GPIO_VALUES", "MAR01-24", 0, 24},
    GPIOGroup{"EN1_32_GPIO_VALUES", "EN1-32", 24, 32},
    GPIOGroup{"LGP01_16_GPIO_VALUES", "LGP01-16", 56, 16},
    GPIOGroup{"DMON1_8_GPIO_VALUES", "DMON1-8", 72, 8},
    GPIOGroup{"GPIO1_4_GPIO_VALUES", "GPIO1-4", 80, 4}};

void UCD90320Device::storeGPIOValues(
    Services& services, const std::vector<int>& values,
    std::map<std::string, std::string>& additionalData)
{
    // Verify the expected number of GPIO values were passed in
    unsigned int expectedCount =
        gpioGroups.back().offset + gpioGroups.back().count;
    if (values.size() != expectedCount)
    {
        // Unexpected number of values; store as a plain list of integers
        StandardDevice::storeGPIOValues(services, values, additionalData);
        return;
    }

    // Store GPIO groups in additional data and journal
    services.logInfoMsg(std::format("Device {} GPIO values:", name));
    auto span = std::span{values};
    std::string valuesStr;
    for (const GPIOGroup& group : gpioGroups)
    {
        valuesStr =
            format_utils::toString(span.subspan(group.offset, group.count));
        additionalData.emplace(group.additionalDataName, valuesStr);
        services.logInfoMsg(
            std::format("{}: {}", group.journalName, valuesStr));
    }
}

} // namespace phosphor::power::sequencer
