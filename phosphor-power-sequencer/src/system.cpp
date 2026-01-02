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

#include "system.hpp"

#include <exception>

namespace phosphor::power::sequencer
{

void System::initializeMonitoring(Services& services)
{
    // Initialize status monitoring in all the chassis
    for (auto& curChassis : chassis)
    {
        curChassis->initializeMonitoring(services);
    }
    isMonitoringInitialized = true;
}

void System::setPowerState(PowerState newPowerState, Services& services)
{
    verifyMonitoringInitialized();
    verifyCanSetPowerState(newPowerState);

    // Get chassis that can be set to the new power state
    auto chassisToSet = getChassisForNewPowerState(newPowerState, services);
    if (chassisToSet.empty())
    {
        throw std::runtime_error{std::format(
            "Unable to set system to state {}: No chassis can be set to that state",
            PowerInterface::toString(newPowerState))};
    }

    // Set new power state
    powerState = newPowerState;

    // Save list of chassis selected for current power on/off attempt
    selectedChassis = chassisToSet;

    // Set power state for selected chassis
    for (auto& curChassis : chassis)
    {
        if (selectedChassis.contains(curChassis->getNumber()))
        {
            try
            {
                curChassis->setPowerState(newPowerState, services);
            }
            catch (const std::exception& e)
            {
                services.logErrorMsg(std::format(
                    "Unable to set chassis {} to state {}: {}",
                    curChassis->getNumber(),
                    PowerInterface::toString(newPowerState), e.what()));
            }
        }
    }
}

void System::monitor(Services& services)
{
    verifyMonitoringInitialized();

    // Monitor the status of all chassis, including those not selected for
    // current power on/off attempt. All chassis need to react to D-Bus status
    // changes.
    for (auto& curChassis : chassis)
    {
        try
        {
            curChassis->monitor(services);
        }
        catch (const std::exception& e)
        {
            services.logErrorMsg(
                std::format("Unable to monitor chassis {}: {}",
                            curChassis->getNumber(), e.what()));
        }
    }

    // Set initial set of chassis selected for power on/off if needed
    setInitialSelectedChassisIfNeeded();

    // Set the system power good based on the chassis power good values
    setPowerGood();

    // Set initial system power state based on system power good if needed
    setInitialPowerStateIfNeeded();
}

std::set<size_t> System::getChassisForNewPowerState(PowerState newPowerState,
                                                    Services& services)
{
    std::set<size_t> chassisForState;
    for (auto& curChassis : chassis)
    {
        auto [canSet, reason] = curChassis->canSetPowerState(newPowerState);
        if (canSet)
        {
            chassisForState.emplace(curChassis->getNumber());
        }
        else
        {
            services.logInfoMsg(
                std::format("Unable to set chassis {} to state {}: {}",
                            curChassis->getNumber(),
                            PowerInterface::toString(newPowerState), reason));
        }
    }
    return chassisForState;
}

void System::setInitialSelectedChassisIfNeeded()
{
    if (!selectedChassis.empty())
    {
        // Selected set of chassis is already defined
        return;
    }

    // Get the set of chassis that are powered on and off. Ignore chassis with
    // an invalid status like not present.
    std::set<size_t> chassisOn, chassisOff;
    for (auto& curChassis : chassis)
    {
        try
        {
            size_t chassisNumber = curChassis->getNumber();
            if (curChassis->isPresent() && curChassis->isAvailable() &&
                curChassis->isInputPowerGood())
            {
                if (curChassis->getPowerGood() == PowerGood::on)
                {
                    chassisOn.emplace(chassisNumber);
                }
                else
                {
                    chassisOff.emplace(chassisNumber);
                }
            }
        }
        catch (...)
        {
            // Ignore errors; chassis status/power good might not be available
        }
    }

    if (chassisOn.empty())
    {
        // No chassis with a valid status is powered on. Assume last requested
        // power state was off. Use powered off chassis as initial selected set.
        selectedChassis = chassisOff;
    }
    else
    {
        // At least one chassis with a valid status is powered on. Assume last
        // requested power state was on. Use powered on chassis as initial
        // selected set.
        selectedChassis = chassisOn;
    }
}

void System::setPowerGood()
{
    // Return if there are no chassis selected for a power on/off attempt
    if (selectedChassis.empty())
    {
        return;
    }

    // Count the number of selected chassis with power good on and off
    size_t powerGoodOnCount{0}, powerGoodOffCount{0};
    for (auto& curChassis : chassis)
    {
        if (selectedChassis.contains(curChassis->getNumber()))
        {
            try
            {
                if (curChassis->getPowerGood() == PowerGood::on)
                {
                    ++powerGoodOnCount;
                }
                else
                {
                    ++powerGoodOffCount;
                }
            }
            catch (...)
            {
                // Chassis power good might not be available
            }
        }
    }

    if (powerGoodOnCount == selectedChassis.size())
    {
        // All selected chassis are on; set system power good to on
        powerGood = PowerGood::on;
    }
    else if (powerGoodOffCount == selectedChassis.size())
    {
        // All selected chassis are off; set system power good to off
        powerGood = PowerGood::off;
    }
}

void System::setInitialPowerStateIfNeeded()
{
    if (!powerState)
    {
        if (powerGood)
        {
            if (powerGood == PowerGood::off)
            {
                powerState = PowerState::off;
            }
            else
            {
                powerState = PowerState::on;
            }
        }
    }
}

} // namespace phosphor::power::sequencer
