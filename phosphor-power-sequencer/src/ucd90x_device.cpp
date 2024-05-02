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

#include "ucd90x_device.hpp"

#include "pmbus.hpp"

#include <exception>
#include <format>
#include <stdexcept>

namespace phosphor::power::sequencer
{

using namespace pmbus;

uint64_t UCD90xDevice::getMfrStatus()
{
    uint64_t value{0};
    try
    {
        std::string fileName{"mfr_status"};
        value = pmbusInterface->read(fileName, Type::HwmonDeviceDebug);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to read MFR_STATUS for device {}: {}", name, e.what())};
    }
    return value;
}

void UCD90xDevice::storePgoodFaultDebugData(
    Services& services, const std::vector<int>& gpioValues,
    std::map<std::string, std::string>& additionalData)
{
    // Store manufacturer-specific MFR_STATUS command value
    try
    {
        uint64_t value = getMfrStatus();
        services.logInfoMsg(
            std::format("Device {} MFR_STATUS: {:#014x}", name, value));
        additionalData.emplace("MFR_STATUS", std::format("{:#014x}", value));
    }
    catch (...)
    {
        // Ignore error; don't interrupt pgood fault handling
    }

    // Call parent class method to store standard data
    PMBusDriverDevice::storePgoodFaultDebugData(services, gpioValues,
                                                additionalData);
}

} // namespace phosphor::power::sequencer
