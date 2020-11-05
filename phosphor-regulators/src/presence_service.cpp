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

#include "presence_service.hpp"

#include "types.hpp"
#include "utility.hpp"

namespace phosphor::power::regulators
{

bool DBusPresenceService::isPresent(const std::string& inventoryPath)
{
    bool present{false};

    // Try to find cached presence value
    auto it = cache.find(inventoryPath);
    if (it != cache.end())
    {
        present = it->second;
    }
    else
    {
        // Get presence from D-Bus interface/property
        util::getProperty(INVENTORY_IFACE, PRESENT_PROP, inventoryPath,
                          INVENTORY_MGR_IFACE, bus, present);

        // Cache presence value
        cache[inventoryPath] = present;
    }

    return present;
}

} // namespace phosphor::power::regulators
