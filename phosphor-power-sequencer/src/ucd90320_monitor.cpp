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

#include <string>

namespace phosphor::power::sequencer
{

const std::string driverName = "ucd9000";

UCD90320Monitor::UCD90320Monitor(sdbusplus::bus::bus& bus, std::uint8_t i2cBus,
                                 std::uint16_t i2cAddress) :
    PowerSequencerMonitor(),
    bus{bus}, interface {
    fmt::format("/sys/bus/i2c/devices/{}-{:04x}", i2cBus, i2cAddress).c_str(),
        driverName, 0
}
{}

} // namespace phosphor::power::sequencer
