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

#include "system.hpp"

namespace phosphor::power::regulators
{

void System::buildIDMap()
{
    // Add rules to the map
    for (std::unique_ptr<Rule>& rule : rules)
    {
        idMap.addRule(*rule);
    }

    // Add devices and rails in each chassis to the map
    for (std::unique_ptr<Chassis>& oneChassis : chassis)
    {
        oneChassis->addToIDMap(idMap);
    }
}

void System::clearCache()
{
    // Clear any cached data in each chassis
    for (std::unique_ptr<Chassis>& oneChassis : chassis)
    {
        oneChassis->clearCache();
    }
}

void System::closeDevices(Services& services)
{
    // Close devices in each chassis
    for (std::unique_ptr<Chassis>& oneChassis : chassis)
    {
        oneChassis->closeDevices(services);
    }
}

void System::configure(Services& services)
{
    // Configure devices in each chassis
    for (std::unique_ptr<Chassis>& oneChassis : chassis)
    {
        oneChassis->configure(services, *this);
    }
}

void System::monitorSensors(Services& services)
{
    // Monitor sensors in each chassis
    for (std::unique_ptr<Chassis>& oneChassis : chassis)
    {
        oneChassis->monitorSensors(services, *this);
    }
}

} // namespace phosphor::power::regulators
