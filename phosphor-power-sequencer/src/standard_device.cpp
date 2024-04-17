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

#include "standard_device.hpp"

#include "format_utils.hpp"

#include <exception>
#include <format>
#include <span>
#include <stdexcept>

namespace phosphor::power::sequencer
{

std::string StandardDevice::findPgoodFault(
    Services& services, const std::string& powerSupplyError,
    std::map<std::string, std::string>& additionalData)
{
    std::string error{};
    try
    {
        prepareForPgoodFaultDetection(services);

        // Get all GPIO values (if possible) from device.  They may be slow to
        // obtain, so obtain them once and then pass values to each Rail object.
        std::vector<int> gpioValues = getGPIOValuesIfPossible();

        // Loop through all the rails checking if any detected a pgood fault.
        // The rails are in power-on-sequence order.
        for (std::unique_ptr<Rail>& rail : rails)
        {
            if (rail->hasPgoodFault(*this, services, gpioValues,
                                    additionalData))
            {
                services.logErrorMsg(std::format(
                    "Pgood fault found in rail monitored by device {}", name));

                // If this is a PSU rail and a PSU error was previously detected
                if (rail->isPowerSupplyRail() && !powerSupplyError.empty())
                {
                    // Return power supply error as root cause
                    error = powerSupplyError;
                }
                else
                {
                    // Return pgood fault as root cause
                    error =
                        "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault";
                }

                storePgoodFaultDebugData(services, gpioValues, additionalData);
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to determine if a pgood fault occurred in device {}: {}",
            name, e.what())};
    }
    return error;
}

std::vector<int> StandardDevice::getGPIOValuesIfPossible()
{
    std::vector<int> values{};
    try
    {
        values = getGPIOValues();
    }
    catch (...)
    {}
    return values;
}

void StandardDevice::storePgoodFaultDebugData(
    Services& services, const std::vector<int>& gpioValues,
    std::map<std::string, std::string>& additionalData)
{
    additionalData.emplace("DEVICE_NAME", name);
    if (!gpioValues.empty())
    {
        std::string valuesStr = format_utils::toString(std::span(gpioValues));
        services.logInfoMsg(
            std::format("Device {} GPIO values: {}", name, valuesStr));
        additionalData.emplace("GPIO_VALUES", valuesStr);
    }
}

} // namespace phosphor::power::sequencer
