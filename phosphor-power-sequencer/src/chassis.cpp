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

/**
 * General error logged when a timeout occurs during a power on attempt.
 *
 * Logged when no specific voltage rail was found that caused the timeout.
 */
const std::string powerOnTimeoutError =
    "xyz.openbmc_project.Power.Error.PowerOnTimeout";

/**
 * General error logged when a timeout occurs during a power off attempt.
 */
const std::string powerOffTimeoutError =
    "xyz.openbmc_project.Power.Error.PowerOffTimeout";

/**
 * General error logged when a power good fault occurs.
 *
 * Logged when no specific voltage rail was found that caused the fault.
 */
const std::string shutdownError = "xyz.openbmc_project.Power.Error.Shutdown";

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
        if (!isEnabled() && (newPowerState == PowerState::on))
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

    services.logInfoMsg(
        std::format("Powering {} chassis {}",
                    PowerInterface::toString(newPowerState), number));

    powerState = newPowerState;
    isInStateTransition = true;
    powerGoodTimeoutTime = getCurrentTime() + powerGoodTimeout;

    if (powerState == PowerState::on)
    {
        clearErrorHistory();
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
    updatePowerGood(services);
    updateInPowerStateTransition();
    checkForPowerGoodError(services);
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

void Chassis::updatePowerGood(Services& services)
{
    if (!isPresent() || !isInputPowerGood())
    {
        powerState = PowerState::off;
        powerGood = PowerGood::off;
        closeDevices();
    }
    else if (isPresent() && isAvailable() && isInputPowerGood())
    {
        readPowerGood(services);
        setInitialPowerStateIfNeeded();
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
        powerGood = PowerGood::on;
    }
    else if (powerGoodOffCount == powerSequencers.size())
    {
        // All devices have power good off; set chassis power good to off
        powerGood = PowerGood::off;
    }
    else if (!isInStateTransition && (powerGoodOffCount > 0))
    {
        // If we are not in a state transition and any devices are off, then set
        // chassis power good to off
        powerGood = PowerGood::off;
    }
}

void Chassis::setInitialPowerStateIfNeeded()
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

void Chassis::updateInPowerStateTransition()
{
    if (isInStateTransition && powerState && powerGood)
    {
        bool bothOff =
            ((powerState == PowerState::off) && (powerGood == PowerGood::off));

        bool bothOn =
            ((powerState == PowerState::on) && (powerGood == PowerGood::on));

        if (bothOff || bothOn)
        {
            isInStateTransition = false;
        }
    }
}

void Chassis::checkForPowerGoodError(Services& services)
{
    auto now = getCurrentTime();

    // Log power good fault if one was detected, logging was delayed, and delay
    // has now elapsed
    if (powerGoodFault)
    {
        if (!powerGoodFault->wasLogged && (now >= powerGoodFault->logTime))
        {
            logPowerGoodFault(services);
        }
    }

    // Log error if state transition did not succeed within timeout
    if (isInStateTransition)
    {
        if (now >= powerGoodTimeoutTime)
        {
            handlePowerGoodTimeout(services);
        }
    }

    // Detect power good fault if chassis has valid status to read power good,
    // power state/power good have valid values, not in state transition, fault
    // not previously detected, and power good is off when it should be on
    if (isPresent() && isAvailable() && isInputPowerGood() && powerState &&
        powerGood && !isInStateTransition && !powerGoodFault)
    {
        if ((powerState == PowerState::on) && (powerGood == PowerGood::off))
        {
            handlePowerGoodFault(services);
        }
    }
}

void Chassis::handlePowerGoodTimeout(Services& services)
{
    services.logErrorMsg(
        std::format("Power {} failed in chassis {}: Timeout",
                    PowerInterface::toString(*powerState), number));
    isInStateTransition = false;

    if (powerState == PowerState::on)
    {
        // Power on timeout is a type of power good fault; log power good fault
        powerGoodFault = PowerGoodFault{};
        powerGoodFault->wasTimeout = true;
        logPowerGoodFault(services);
    }
    else
    {
        // Power off timeout is not a power good fault; log power off error
        logPowerOffTimeout(services);
    }
}

void Chassis::handlePowerGoodFault(Services& services)
{
    services.logErrorMsg(std::format("Power good fault in chassis {}", number));

    // Create PowerGoodFault object. Delay logging error to allow the power
    // supplies and other hardware time to complete failure processing.
    powerGoodFault = PowerGoodFault{};
    powerGoodFault->logTime = getCurrentTime() + powerGoodFaultLogDelay;
}

void Chassis::logPowerOffTimeout(Services& services)
{
    std::map<std::string, std::string> additionalData{};
    services.logError(powerOffTimeoutError, Entry::Level::Critical,
                      additionalData);
}

void Chassis::logPowerGoodFault(Services& services)
{
    // Try to find which voltage rail caused power good fault
    std::map<std::string, std::string> additionalData{};
    std::string error = findPowerGoodFaultInRail(additionalData, services);

    // If voltage rail was not found, log a more general error
    if (error.empty())
    {
        if (!powerSupplyError.empty())
        {
            error = powerSupplyError;
        }
        else if (powerGoodFault->wasTimeout)
        {
            error = powerOnTimeoutError;
        }
        else
        {
            error = shutdownError;
        }
    }

    services.logError(error, Entry::Level::Critical, additionalData);
    powerGoodFault->wasLogged = true;
}

std::string Chassis::findPowerGoodFaultInRail(
    std::map<std::string, std::string>& additionalData, Services& services)
{
    std::string error{};
    try
    {
        for (auto& powerSequencer : powerSequencers)
        {
            openDeviceIfNeeded(*powerSequencer, services);
            error = powerSequencer->findPgoodFault(services, powerSupplyError,
                                                   additionalData);
            if (!error.empty())
            {
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        services.logErrorMsg(std::format(
            "Unable to find rail that caused power good fault in chassis {}: {}",
            number, e.what()));
        additionalData.emplace("ERROR", e.what());
    }
    return error;
}

} // namespace phosphor::power::sequencer
