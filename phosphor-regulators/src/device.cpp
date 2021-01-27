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

#include "device.hpp"

#include "chassis.hpp"
#include "exception_utils.hpp"
#include "system.hpp"

#include <exception>

namespace phosphor::power::regulators
{

void Device::addToIDMap(IDMap& idMap)
{
    // Add this device to the map
    idMap.addDevice(*this);

    // Add rails to the map
    for (std::unique_ptr<Rail>& rail : rails)
    {
        idMap.addRail(*rail);
    }
}

void Device::clearCache()
{
    // If presence detection is defined for this device
    if (presenceDetection)
    {
        // Clear cached presence data
        presenceDetection->clearCache();
    }
}

void Device::close(Services& services)
{
    try
    {
        // Close I2C interface if it is open
        if (i2cInterface->isOpen())
        {
            i2cInterface->close();
        }
    }
    catch (const std::exception& e)
    {
        // Log error messages in journal
        services.getJournal().logError(exception_utils::getMessages(e));
        services.getJournal().logError("Unable to close device " + id);

        // TODO: Create error log entry
    }
}

void Device::configure(Services& services, System& system, Chassis& chassis)
{
    // If configuration changes are defined for this device, apply them
    if (configuration)
    {
        configuration->execute(services, system, chassis, *this);
    }

    // Configure rails
    for (std::unique_ptr<Rail>& rail : rails)
    {
        rail->configure(services, system, chassis, *this);
    }
}

void Device::monitorSensors(Services& services, System& system,
                            Chassis& chassis)
{

    // Monitor sensors in each rail
    for (std::unique_ptr<Rail>& rail : rails)
    {
        rail->monitorSensors(services, system, chassis, *this);
    }
}

} // namespace phosphor::power::regulators
