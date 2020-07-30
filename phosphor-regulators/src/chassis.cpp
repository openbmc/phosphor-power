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

#include "journal.hpp"
#include "services.hpp"
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

void Chassis::closeDevices()
{
    // Log debug message in journal
    journal::logDebug("Closing devices in chassis " + std::to_string(number));

    // Close devices
    for (std::unique_ptr<Device>& device : devices)
    {
        device->close();
    }
}

void Chassis::configure(Services& services, System& system)
{
    // Log info message in services; important for verifying success of boot
    services.getJournal().logInfo("Configuring chassis " +
                                  std::to_string(number));

    // Configure devices
    for (std::unique_ptr<Device>& device : devices)
    {
        device->configure(services, system, *this);
    }
}

void Chassis::monitorSensors(System& system)
{
    // Monitor sensors in each device
    for (std::unique_ptr<Device>& device : devices)
    {
        device->monitorSensors(system, *this);
    }
}

} // namespace phosphor::power::regulators
