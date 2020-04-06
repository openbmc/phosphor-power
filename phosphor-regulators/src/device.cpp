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

} // namespace phosphor::power::regulators
