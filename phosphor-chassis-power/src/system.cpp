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
#include "system.hpp"

#include "utility.hpp"

#include <phosphor-logging/lg2.hpp>

#include <ranges>

namespace phosphor::power::chassis
{

using namespace phosphor::power::util;

void System::checkLatchedFaults()
{
    for (const auto& curChassis : chassis)
    {
        curChassis->startLatchedFaultCheck();
    }
}

Chassis* System::getPrimaryChassis()
{
    unsigned int bmcPosition = 0;

    try
    {
        constexpr auto systemPath = "/xyz/openbmc_project/inventory/system";
        constexpr auto positionIntf =
            "xyz.openbmc_project.Inventory.Decorator.Position";
        constexpr auto positionProp = "Position";

        auto service =
            getService(systemPath, positionIntf, services.getBus(), false);
        if (!service.empty())
        {
            getProperty(positionIntf, positionProp, systemPath, service,
                        services.getBus(), bmcPosition);
            bmcPosition++;
        }
        else
        {
            lg2::error("Unable to get service for BMC position");
            return nullptr;
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Error getting BMC position: {ERROR}", "ERROR", e);
        return nullptr;
    }

    const auto primaryChassis =
        std::ranges::find_if(chassis, [bmcPosition](const auto& curChassis) {
            return curChassis->getNumber() == bmcPosition;
        });
    if (primaryChassis == chassis.end())
    {
        lg2::error("Unable to find chassis matching BMC position {POSITION}",
                   "POSITION", bmcPosition);
        return nullptr;
    }

    return primaryChassis->get();
}

void System::initializePresence()
{
    initializedPresence = true;
    auto* curChassis = getPrimaryChassis();

    if (curChassis != nullptr)
    {
        curChassis->initializePresence();
    }
}

void System::initializePowerSystemInputs()
{
    initializedPowerSystemInputs = true;
    auto* curChassis = getPrimaryChassis();

    if (curChassis != nullptr)
    {
        curChassis->initializePowerSystemInputsInterface(
            PowerSystemInputs::Status::Good);
    }
}

void System::monitor()
{
    if (!initializedPresence)
    {
        initializePresence();
    }
    if (!initializedPowerSystemInputs)
    {
        initializePowerSystemInputs();
    }
    for (const auto& curChassis : chassis)
    {
        curChassis->monitor();
    }
}

void System::initializeStatusMonitors()
{
    // Create system-level status monitor
    try
    {
        phosphor::power::util::ChassisStatusMonitorOptions options;
        options.isPowerGoodMonitored = true;

        std::string systemInventoryPath =
            "/xyz/openbmc_project/inventory/system/chassis";

        systemMonitor = services.createChassisStatusMonitor(
            0, systemInventoryPath, options);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to initialize system status monitor: {ERROR}",
                   "ERROR", e);
    }

    // Pass system monitor to all chassis
    for (const auto& curChassis : chassis)
    {
        curChassis->setSystemStatusMonitor(systemMonitor);
    }
}

void System::clearErrorHistory()
{
    // Clear error history for all chassis
    for (const auto& curChassis : chassis)
    {
        curChassis->clearErrorHistory();
    }
}

} // namespace phosphor::power::chassis
