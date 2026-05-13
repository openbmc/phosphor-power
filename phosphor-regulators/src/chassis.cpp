/**
 * Copyright © 2020 IBM Corporation
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

#include "system.hpp"

namespace phosphor::power::regulators
{

void Chassis::addToIDMap(IDMap& idMap)
{
    // Add devices and their rails to the map
    for (std::unique_ptr<Device>& device : devices)
    {
        device->addToIDMap(idMap);
    }
}
bool Chassis::canConfigure(Services& services)
{
    try
    {
        if (!isPresent())
        {
            services.getJournal().logInfo(std::format(
                "Unable to configure chassis {}: Chassis is not present",
                number));
            return false;
        }

        if (!isEnabled())
        {
            services.getJournal().logInfo(std::format(
                "Unable to configure chassis {}: Chassis is not enabled",
                number));
            return false;
        }

        if (!isAvailable())
        {
            services.getJournal().logInfo(std::format(
                "Unable to configure chassis {}: Chassis is not available",
                number));
            return false;
        }
    }
    catch (const std::exception& e)
    {
        services.getJournal().logError(std::format(
            "Unable to configure chassis {}: {}", number, e.what()));
        return false;
    }

    return true;
}

bool Chassis::canMonitor(Services& services)
{
    bool canMonitor{false};

    try
    {
        if (isPresent() && isPoweredOn() && isAvailable())
        {
            canMonitor = true;
        }

        // Clear error count since we were able to determine the chassis status
        statusErrorCount = 0;
    }
    catch (const std::exception& e)
    {
        if (statusErrorCount < maxStatusErrorCount)
        {
            ++statusErrorCount;
            services.getJournal().logError(
                std::format("Unable to determine status of chassis {}: {}",
                            number, e.what()));
        }
    }

    return canMonitor;
}

void Chassis::clearCache()
{
    // Clear any cached data in each device
    for (std::unique_ptr<Device>& device : devices)
    {
        device->clearCache();
    }
}

void Chassis::clearErrorHistory()
{
    statusErrorCount = 0;

    // Clear error history in each device
    for (std::unique_ptr<Device>& device : devices)
    {
        device->clearErrorHistory();
    }
}

void Chassis::closeDevices(Services& services)
{
    // Log debug message in journal
    services.getJournal().logDebug(
        "Closing devices in chassis " + std::to_string(number));

    // Ignore errors when closing devices if chassis status is not valid. For
    // example, if chassis is not present, closing a device will likely fail.
    // However, we still need to clean up any associated resources.
    bool ignoreErrors = !canMonitor(services);

    // Close devices
    closeDevices(services, ignoreErrors);
}

void Chassis::configure(Services& services, System& system)
{
    if (!canConfigure(services))
    {
        return;
    }

    // Log info message in journal; important for verifying success of boot
    services.getJournal().logInfo(
        "Configuring chassis " + std::to_string(number));

    // Configure devices
    for (std::unique_ptr<Device>& device : devices)
    {
        device->configure(services, system, *this);
    }
}

void Chassis::detectPhaseFaults(Services& services, System& system)
{
    if (!canMonitor(services))
    {
        bool ignoreErrors{true};
        closeDevices(services, ignoreErrors);
        return;
    }

    // Detect phase faults in each device
    for (std::unique_ptr<Device>& device : devices)
    {
        device->detectPhaseFaults(services, system, *this);
    }
}

void Chassis::monitorSensors(Services& services, System& system)
{
    if (!canMonitor(services))
    {
        bool ignoreErrors{true};
        closeDevices(services, ignoreErrors);
        return;
    }

    // Monitor sensors in each device
    for (std::unique_ptr<Device>& device : devices)
    {
        device->monitorSensors(services, system, *this);
    }
}

void Chassis::closeDevices(Services& services, bool ignoreErrors)
{
    for (std::unique_ptr<Device>& device : devices)
    {
        device->close(services, ignoreErrors);
    }
}

} // namespace phosphor::power::regulators
