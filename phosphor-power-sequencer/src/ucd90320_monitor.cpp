/**
 * Copyright © 2021 IBM Corporation
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

#include "ucd90320_monitor.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <phosphor-logging/log.hpp>

#include <span>

namespace phosphor::power::sequencer
{

using namespace phosphor::logging;

UCD90320Monitor::UCD90320Monitor(sdbusplus::bus_t& bus, std::uint8_t i2cBus,
                                 std::uint16_t i2cAddress) :
    UCD90xMonitor(bus, i2cBus, i2cAddress, "UCD90320", 32)
{}

void UCD90320Monitor::formatGpioValues(
    const std::vector<int>& values, unsigned int numberLines,
    std::map<std::string, std::string>& additionalData) const
{
    // Device has 84 GPIO pins so that value is expected
    if (numberLines == 84 && values.size() >= 84)
    {
        log<level::INFO>(fmt::format("MAR01-24 GPIO values: {}",
                                     std::span{values}.subspan(0, 24))
                             .c_str());
        additionalData.emplace(
            "MAR01_24_GPIO_VALUES",
            fmt::format("{}", std::span{values}.subspan(0, 24)));
        log<level::INFO>(fmt::format("EN1-32 GPIO values: {}",
                                     std::span{values}.subspan(24, 32))
                             .c_str());
        additionalData.emplace(
            "EN1_32_GPIO_VALUES",
            fmt::format("{}", std::span{values}.subspan(24, 32)));
        log<level::INFO>(fmt::format("LGP01-16 GPIO values: {}",
                                     std::span{values}.subspan(56, 16))
                             .c_str());
        additionalData.emplace(
            "LGP01_16_GPIO_VALUES",
            fmt::format("{}", std::span{values}.subspan(56, 16)));
        log<level::INFO>(fmt::format("DMON1-8 GPIO values: {}",
                                     std::span{values}.subspan(72, 8))
                             .c_str());
        additionalData.emplace(
            "DMON1_8_GPIO_VALUES",
            fmt::format("{}", std::span{values}.subspan(72, 8)));
        log<level::INFO>(fmt::format("GPIO1-4 GPIO values: {}",
                                     std::span{values}.subspan(80, 4))
                             .c_str());
        additionalData.emplace(
            "GPIO1_4_GPIO_VALUES",
            fmt::format("{}", std::span{values}.subspan(80, 4)));
    }
    else
    {
        log<level::INFO>(fmt::format("GPIO values: {}", values).c_str());
        additionalData.emplace("GPIO_VALUES", fmt::format("{}", values));
    }
}

} // namespace phosphor::power::sequencer
