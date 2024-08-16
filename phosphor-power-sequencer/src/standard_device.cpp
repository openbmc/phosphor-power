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
        std::vector<int> gpioValues = getGPIOValuesIfPossible(services);

        // Try to find a voltage rail where a pgood fault occurred
        Rail* rail =
            findRailWithPgoodFault(services, gpioValues, additionalData);
        if (rail != nullptr)
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

std::vector<int> StandardDevice::getGPIOValuesIfPossible(Services& services)
{
    std::vector<int> values{};
    try
    {
        values = getGPIOValues(services);
    }
    catch (...)
    {}
    return values;
}

Rail* StandardDevice::findRailWithPgoodFault(
    Services& services, const std::vector<int>& gpioValues,
    std::map<std::string, std::string>& additionalData)
{
    // Look for the first rail in the power on sequence with a pgood fault based
    // on STATUS_VOUT.  This is usually the most accurate method.  For example,
    // if a pgood fault occurs, the power sequencer device may automatically
    // shut off related rails.  Ideally the device will only set fault bits in
    // STATUS_VOUT for the rail with the pgood fault.  However, all the related
    // rails will likely appear to be faulted by the other methods.
    for (std::unique_ptr<Rail>& rail : rails)
    {
        if (rail->hasPgoodFaultStatusVout(*this, services, additionalData))
        {
            return rail.get();
        }
    }

    // Look for the first rail in the power on sequence with a pgood fault based
    // on either a GPIO or the output voltage.  Both methods check if the rail
    // is powered off.  If a pgood fault occurs during the power on sequence,
    // the power sequencer device may stop powering on rails.  As a result, all
    // rails after the faulted one in the sequence may also be powered off.
    for (std::unique_ptr<Rail>& rail : rails)
    {
        if (rail->hasPgoodFaultGPIO(*this, services, gpioValues,
                                    additionalData) ||
            rail->hasPgoodFaultOutputVoltage(*this, services, additionalData))
        {
            return rail.get();
        }
    }

    // No rail with pgood fault found
    return nullptr;
}

void StandardDevice::storePgoodFaultDebugData(
    Services& services, const std::vector<int>& gpioValues,
    std::map<std::string, std::string>& additionalData)
{
    try
    {
        additionalData.emplace("DEVICE_NAME", name);
        storeGPIOValues(services, gpioValues, additionalData);
    }
    catch (...)
    {}
}

void StandardDevice::storeGPIOValues(
    Services& services, const std::vector<int>& values,
    std::map<std::string, std::string>& additionalData)
{
    if (!values.empty())
    {
        std::string valuesStr = format_utils::toString(std::span(values));
        services.logInfoMsg(
            std::format("Device {} GPIO values: {}", name, valuesStr));
        additionalData.emplace("GPIO_VALUES", valuesStr);
    }
}

} // namespace phosphor::power::sequencer
