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
            gpio->findLine();
        }
        else if (name.contains(presenceName))
        {
            if (gpio->requestRead())
            {
                readPresenceValue(*gpio);
            }
        }

        else if (name.contains(faultLatchedName))
        {
            if (gpio->requestRead())
            {
                readFaultLatchedValue(*gpio);
            }
        }
        else if (name.contains(faultUnlatchedName))
        {
            if (gpio->requestRead())
            {
                readFaultUnlatchedValue(*gpio);
            }
        }
    }
}

void Chassis::readPresenceValue(Gpio& gpio)
{
    int value;
    int previousValue;

    try
    {
        value = gpio.getValue();
        previousValue = gpio.getPreviousValue();
    }
    catch (...)
    {
        // Other apps will need to read this line.
        gpio.release();
        return;
    }

    // Get deglitched value.
    int newPresenceValue = (value == previousValue ? value : presenceGPIOValue);

    if (newPresenceValue != presenceGPIOValue)
    {
        // Update value
        presenceGPIOValue = newPresenceValue;

        // handle gpio change
    }
    // Other apps will need to read this line.
    gpio.release();
}

void Chassis::readFaultLatchedValue(Gpio& gpio)
{
    int value;
    int previousValue;

    try
    {
        value = gpio.getValue();
        previousValue = gpio.getPreviousValue();
    }
    catch (...)
    {
        return;
    }

    // Get deglitched value.
    int newFaultLatchedValue =
        (value == previousValue ? value : faultLatchedValue);

    if (newFaultLatchedValue != faultLatchedValue)
    {
        // Update value
        faultLatchedValue = newFaultLatchedValue;

        // handle gpio change
    }
}

void Chassis::readFaultUnlatchedValue(Gpio& gpio)
{
    int value;
    int previousValue;

    try
    {
        value = gpio.getValue();
        previousValue = gpio.getPreviousValue();
    }
    catch (...)
    {
        return;
    }

    // Get deglitched value.
    int newFaultUnLatchedValue =
        (value == previousValue ? value : faultUnlatchedValue);

    if (newFaultUnLatchedValue != faultUnlatchedValue)
    {
        // Update value
        faultUnlatchedValue = newFaultUnLatchedValue;
        lg2::info("Chassis {CHASSIS} fault-unlatched changed to {VALUE}",
                  "CHASSIS", number, "VALUE", faultUnlatchedValue);

        // handle gpio change
    }
}

} // namespace phosphor::power::chassis
