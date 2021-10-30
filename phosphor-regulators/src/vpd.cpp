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

std::vector<uint8_t> DBusVPD::getValue(const std::string& inventoryPath,
                                       const std::string& keyword)
{
    std::vector<uint8_t> value{};

    // Get cached keywords for the inventory path
    KeywordMap& cachedKeywords = cache[inventoryPath];

    // Check if the keyword value is already cached
    auto it = cachedKeywords.find(keyword);
    if (it != cachedKeywords.end())
    {
        // Get keyword value from cache
        value = it->second;
    }
    else
    {
        // Get keyword value from D-Bus interface/property
        getDBusProperty(inventoryPath, keyword, value);

        // Cache keyword value
        cachedKeywords[keyword] = value;
    }

    return value;
}

void DBusVPD::getDBusProperty(const std::string& inventoryPath,
                              const std::string& keyword,
                              std::vector<uint8_t>& value)
{
    // Determine the D-Bus property name.  Normally this is the same as the VPD
    // keyword name.  However, the CCIN keyword is stored in the Model property.
    std::string property{(keyword == "CCIN") ? "Model" : keyword};

    value.clear();
    try
    {
        if (property == "HW")
        {
            // HW property in non-standard interface and has byte vector value
            util::getProperty("com.ibm.ipzvpd.VINI", property, inventoryPath,
                              INVENTORY_MGR_IFACE, bus, value);
        }
        else
        {
            // Other properties in standard interface and have string value
            std::string stringValue;
            util::getProperty(ASSET_IFACE, property, inventoryPath,
                              INVENTORY_MGR_IFACE, bus, stringValue);
            value.insert(value.begin(), stringValue.begin(), stringValue.end());
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        // If exception indicates VPD interface or property doesn't exist
        if (isUnknownPropertyException(e))
        {
            // Treat this as an empty keyword value
            value.clear();
        }
        else
        {
            // Re-throw other exceptions
            throw;
        }
    }
}

bool DBusVPD::isUnknownPropertyException(const sdbusplus::exception_t& e)
{
    // Initially assume exception was due to some other type of error
    bool isUnknownProperty{false};

    // If the D-Bus error name is set within the exception
    if (e.name() != nullptr)
    {
        // Check if the error name indicates the specified interface or property
        // does not exist on the specified object path
        std::string name = e.name();
        if ((name == SD_BUS_ERROR_UNKNOWN_INTERFACE) ||
            (name == SD_BUS_ERROR_UNKNOWN_PROPERTY))
        {
            isUnknownProperty = true;
        }
    }

    return isUnknownProperty;
}

} // namespace phosphor::power::regulators
