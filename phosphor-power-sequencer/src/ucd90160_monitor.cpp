/**
 * Copyright © 2022 IBM Corporation
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

#include "ucd90160_monitor.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <array>
#include <span>

namespace phosphor::power::sequencer
{

using namespace phosphor::logging;

// Names of the UCD90160 GPIOs.  The array indices correspond to the Pin IDs
// defined in the UCD90160 PMBus interface documentation.  These Pin IDs are the
// same as the libgpiod line offsets used to obtain the GPIO values.
static constexpr std::array<const char*, 26> gpioNames = {
    "FPWM1_GPIO5", "FPWM2_GPIO6",  "FPWM3_GPIO7",  "FPWM4_GPIO8",
    "FPWM5_GPIO9", "FPWM6_GPIO10", "FPWM7_GPIO11", "FPWM8_GPIO12",
    "GPI1_PWM1",   "GPI2_PWM2",    "GPI3_PWM3",    "GPI4_PWM4",
    "GPIO14",      "GPIO15",       "TDO_GPIO20",   "TCK_GPIO19",
    "TMS_GPIO22",  "TDI_GPIO21",   "GPIO1",        "GPIO2",
    "GPIO3",       "GPIO4",        "GPIO13",       "GPIO16",
    "GPIO17",      "GPIO18"};

UCD90160Monitor::UCD90160Monitor(sdbusplus::bus_t& bus, std::uint8_t i2cBus,
                                 std::uint16_t i2cAddress) :
    UCD90xMonitor(bus, i2cBus, i2cAddress, "UCD90160", 16)
{}

void UCD90160Monitor::formatGpioValues(
    const std::vector<int>& values, unsigned int numberLines,
    std::map<std::string, std::string>& additionalData) const
{
    // Verify the expected number of GPIO values were passed in
    if ((values.size() == gpioNames.size()) &&
        (numberLines == gpioNames.size()))
    {
        // Store GPIO names and values in additional data and journal.
        // Use groups of GPIOs in journal to minimize number of entries.
        unsigned int groupSize{4};
        for (unsigned int i = 0; i < gpioNames.size(); ++i)
        {
            additionalData.emplace(gpioNames[i], std::to_string(values[i]));
            if ((i % groupSize) == 0)
            {
                unsigned int gpiosLeft = gpioNames.size() - i;
                unsigned int count = std::min(groupSize, gpiosLeft);
                log<level::INFO>(
                    fmt::format("GPIO values: {}: {}",
                                std::span{gpioNames.begin() + i, count},
                                std::span{values.begin() + i, count})
                        .c_str());
            }
        }
    }
    else
    {
        // Unexpected number of GPIO values.  Store without names.
        additionalData.emplace("GPIO_VALUES", fmt::format("{}", values));
        log<level::INFO>(fmt::format("GPIO values: {}", values).c_str());
    }
}

} // namespace phosphor::power::sequencer
