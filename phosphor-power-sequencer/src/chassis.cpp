/**
 * Copyright © 2025 IBM Corporation
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

namespace phosphor::power::sequencer
{

std::tuple<bool, std::string> Chassis::canSetPowerState(
    PowerState newPowerState)
{
    verifyMonitoringInitialized();
    try
    {
        if (powerState && (powerState == newPowerState))
        {
            return {false, "Chassis is already at requested state"};
        }

        if (!isPresent())
        {
            return {false, "Chassis is not present"};
        }

        // Do not allow power on for chassis in hardware isolation; power off OK
        if (!isEnabled() && (newPowerState == PowerState::ON))
        {
            return {false, "Chassis is not enabled"};
        }

        if (!isInputPowerGood())
        {
            return {false, "Chassis does not have input power"};
        }

        // Check Available last. This D-Bus property is based on a list of
        // factors including some of the preceding properties.
        if (!isAvailable())
        {
            return {false, "Chassis is not available"};
        }
    }
    catch (const std::exception& e)
    {
        return {false,
                std::format("Error determining chassis status: {}", e.what())};
    }

    return {true, ""};
}

void Chassis::setPowerState(PowerState newPowerState, Services& services)
{
    verifyMonitoringInitialized();
    auto [canSet, reason] = canSetPowerState(newPowerState);
    if (!canSet)
    {
        throw std::runtime_error{
            std::format("Unable to set chassis {} to state {}: {}", number,
                        PowerInterface::toString(newPowerState), reason)};
    }

    powerState = newPowerState;
    if (powerState == PowerState::ON)
    {
        powerOn(services);
    }
    else
    {
        powerOff(services);
    }
}

void Chassis::monitor(Services& services)
{
    verifyMonitoringInitialized();

    if (!isPresent() || !isInputPowerGood())
    {
        powerState = PowerState::OFF;
        powerGood = PowerGood::OFF;
        closeDevices();
        return;
    }

    if (isPresent() && isAvailable() && isInputPowerGood())
    {
        readPowerGood(services);
        setInitialPowerStateIfNeeded();
    }
}

void Chassis::closeDevices()
{
    for (auto& powerSequencer : powerSequencers)
    {
        try
        {
            if (powerSequencer->isOpen())
            {
                powerSequencer->close();
            }
        }
        catch (...)
        {
            // Ignore errors; often called when chassis goes missing/unavailable
        }
    }
}

void Chassis::readPowerGood(Services& services)
{
    // Count the number of power sequencer devices with power good on and off
    size_t powerGoodOnCount{0}, powerGoodOffCount{0};
    for (auto& powerSequencer : powerSequencers)
    {
        try
        {
            openDeviceIfNeeded(*powerSequencer, services);
            if (powerSequencer->getPowerGood())
            {
                ++powerGoodOnCount;
            }
            else
            {
                ++powerGoodOffCount;
            }
        }
        catch (...)
        {}
    }

    if (powerGoodOnCount == powerSequencers.size())
    {
        // All devices have power good on; set chassis power good to on
        powerGood = PowerGood::ON;
    }
    else if (powerGoodOffCount == powerSequencers.size())
    {
        // All devices have power good off; set chassis power good to off
        powerGood = PowerGood::OFF;
    }
}

void Chassis::setInitialPowerStateIfNeeded()
{
    if (!powerState)
    {
        if (powerGood)
        {
            if (powerGood == PowerGood::OFF)
            {
                powerState = PowerState::OFF;
            }
            else
            {
                powerState = PowerState::ON;
            }
        }
    }
}

void Chassis::powerOn(Services& services)
{
    std::string error{};
    for (auto& powerSequencer : powerSequencers)
    {
        try
        {
            openDeviceIfNeeded(*powerSequencer, services);
            powerSequencer->powerOn();
        }
        catch (const std::exception& e)
        {
            // Catch and save error so we can power on any remaining devices
            error =
                std::format("Unable to power on device {} in chassis {}: {}",
                            powerSequencer->getName(), number, e.what());
        }
    }

    if (!error.empty())
    {
        throw std::runtime_error{error};
    }
}

void Chassis::powerOff(Services& services)
{
    std::string error{};
    for (auto& powerSequencer : powerSequencers)
    {
        try
        {
            openDeviceIfNeeded(*powerSequencer, services);
            powerSequencer->powerOff();
        }
        catch (const std::exception& e)
        {
            // Catch and save error so we can power off any remaining devices
            error =
                std::format("Unable to power off device {} in chassis {}: {}",
                            powerSequencer->getName(), number, e.what());
        }
    }

    if (!error.empty())
    {
        throw std::runtime_error{error};
    }
}

} // namespace phosphor::power::sequencer
