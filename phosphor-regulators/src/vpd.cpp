/**
 * Copyright © 2021 IBM Corporation
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

#include "vpd.hpp"

#include "types.hpp"
#include "utility.hpp"

namespace phosphor::power::regulators
{

std::string DBusVPD::getValue(const std::string& inventoryPath,
                              const std::string& keyword)
{
    std::string value{};

    // Get cached keywords for the inventory path
    KeywordMap& cachedKeywords = cache[inventoryPath];

    // Check if the keyword value is already cached
    auto it = cachedKeywords.find(keyword);
    if (it != cachedKeywords.end())
    {
        value = it->second;
    }
    else
    {
        // Get keyword value from D-Bus interface/property.  The property name
        // is normally the same as the VPD keyword name.  However, the CCIN
        // keyword is stored in the Model property.
        std::string property{(keyword == "CCIN") ? "Model" : keyword};
        util::getProperty(ASSET_IFACE, property, inventoryPath,
                          INVENTORY_MGR_IFACE, bus, value);

        // Cache keyword value
        cachedKeywords[keyword] = value;
    }

    return value;
}

} // namespace phosphor::power::regulators
