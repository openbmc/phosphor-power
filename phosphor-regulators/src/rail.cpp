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

#include "rail.hpp"

#include "chassis.hpp"
#include "device.hpp"
#include "system.hpp"

namespace phosphor::power::regulators
{

void Rail::configure(System& system, Chassis& chassis, Device& device)
{
    // If configuration changes are defined for this rail, apply them
    if (configuration)
    {
        configuration->execute(system, chassis, device, *this);
    }
}

} // namespace phosphor::power::regulators
