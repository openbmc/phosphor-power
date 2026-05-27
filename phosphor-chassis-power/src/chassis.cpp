/**
 * Copyright © 2026 IBM Corporation
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

#include "chassis.hpp"

#include "types.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor::power::chassis
{

bool Chassis::initializePowerSystemInputsInterface(sdbusplus::bus_t& bus)
{
    auto chassisInputPowerStatusPath =
        std::format(CHASSIS_INPUT_POWER_STATUS_PATH, number);

    // Create the D-Bus interface object for this chassis
    try
    {
        // TODO: Update to set status to fault when gpio reads are
        // implemented
        powerSystemInputsInterface =
            std::make_unique<ChassisPowerSystemInterface>(
                bus, chassisInputPowerStatusPath.c_str(),
                PowerSystemInputs::Status::Good);
        return true;
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to initialize PowerSystemInputs interface for chassis {CHASSIS}: {ERROR}",
            "CHASSIS", number, "ERROR", e.what());
        return false;
    }
}

void Chassis::setSystemStatusMonitor(
    std::shared_ptr<ChassisStatusMonitor>& monitor)
{
    systemMonitor = monitor;
}

bool Chassis::isSystemPoweredOn() const
{
    if (!systemMonitor)
    {
        lg2::error("System monitor not initialized for chassis {CHASSIS}",
                   "CHASSIS", number);
        return false;
    }

    try
    {
        return systemMonitor->getPowerGood();
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

void Chassis::clearErrorHistory()
{
    for (const auto& gpio : gpios)
    {
        gpio->clearErrorHistory();
    }
}

void Chassis::monitor()
{
    for (const auto& gpio : gpios)
    {
        const std::string& name = gpio->getName();

        if (!gpio->foundLine())
        {
            if (!gpio->findLine())
            {
                continue;
            }
        }

        bool changed = false;

        if (name.contains(presenceName))
        {
            if (gpio->requestRead())
            {
                try
                {
                    changed = gpioValueChanged(*gpio, presenceGPIOValue);
                }
                catch (...)
                {
                    // gpio read fail, handle presence change
                }

                if (changed)
                {
                    // Handle presence change
                }
                // Other apps will need to read this line.
                gpio->release();
            }
        }
        else if (name.contains(faultLatchedName))
        {
            if (gpio->requestRead())
            {
                try
                {
                    changed = gpioValueChanged(*gpio, faultLatchedValue);
                }
                catch (...)
                {
                    // Handle gpio read fail
                }

                if (changed)
                {
                    // Handle fault latched change
                }
            }
        }
        else if (name.contains(faultUnlatchedName))
        {
            if (gpio->requestRead())
            {
                try
                {
                    changed = gpioValueChanged(*gpio, faultUnlatchedValue);
                }
                catch (...)
                {
                    // Handle gpio read fail
                }

                if (changed)
                {
                    // Handle fault unlatched change
                }
            }
        }
    }
}

bool Chassis::gpioValueChanged(Gpio& gpio, std::optional<int>& gpioValue)
{
    int value;
    int previousValue;

    value = gpio.getValue();

    try
    {
        previousValue = gpio.getPreviousValue();
    }
    catch (...)
    {
        // No previous value available, use current value as new value
        if (value != gpioValue)
        {
            gpioValue = value;
            return true;
        }
        return false;
    }

    // Get deglitched value: use current if it matches previous,
    // otherwise keep the cached value
    int newGPIOValue =
        (value == previousValue) ? value : gpioValue.value_or(value);

    if (newGPIOValue != gpioValue)
    {
        // Update value
        gpioValue = newGPIOValue;
        return true;
    }

    return false;
}

} // namespace phosphor::power::chassis
