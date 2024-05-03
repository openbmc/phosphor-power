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

#include "ucd90160_device.hpp"

#include "format_utils.hpp"
#include "standard_device.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <span>

namespace phosphor::power::sequencer
{

/**
 * UCD90160 GPIO names.
 *
 * The array indices correspond to the Pin IDs defined in the UCD90160 PMBus
 * interface documentation.  These Pin IDs are the same as the libgpiod line
 * offsets used to obtain the GPIO values.
 */
static constexpr std::array<const char*, 26> gpioNames = {
    "FPWM1_GPIO5", "FPWM2_GPIO6",  "FPWM3_GPIO7",  "FPWM4_GPIO8",
    "FPWM5_GPIO9", "FPWM6_GPIO10", "FPWM7_GPIO11", "FPWM8_GPIO12",
    "GPI1_PWM1",   "GPI2_PWM2",    "GPI3_PWM3",    "GPI4_PWM4",
    "GPIO14",      "GPIO15",       "TDO_GPIO20",   "TCK_GPIO19",
    "TMS_GPIO22",  "TDI_GPIO21",   "GPIO1",        "GPIO2",
    "GPIO3",       "GPIO4",        "GPIO13",       "GPIO16",
    "GPIO17",      "GPIO18"};

void UCD90160Device::storeGPIOValues(
    Services& services, const std::vector<int>& values,
    std::map<std::string, std::string>& additionalData)
{
    // Verify the expected number of GPIO values were passed in
    if (values.size() != gpioNames.size())
    {
        // Unexpected number of values; store as a plain list of integers
        StandardDevice::storeGPIOValues(services, values, additionalData);
        return;
    }

    // Store GPIO names and values in additional data and journal.
    // Use groups of GPIOs in journal to minimize number of entries.
    services.logInfoMsg(std::format("Device {} GPIO values:", name));
    unsigned int groupSize{4};
    auto namesSpan = std::span{gpioNames};
    auto valuesSpan = std::span{values};
    std::string namesStr, valuesStr;
    for (unsigned int i = 0; i < gpioNames.size(); ++i)
    {
        additionalData.emplace(gpioNames[i], std::format("{}", values[i]));
        if ((i % groupSize) == 0)
        {
            unsigned int gpiosLeft = gpioNames.size() - i;
            unsigned int count = std::min(groupSize, gpiosLeft);
            namesStr = format_utils::toString(namesSpan.subspan(i, count));
            valuesStr = format_utils::toString(valuesSpan.subspan(i, count));
            services.logInfoMsg(std::format("{}: {}", namesStr, valuesStr));
        }
    }
}

} // namespace phosphor::power::sequencer
