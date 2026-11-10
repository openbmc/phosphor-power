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

#include <variant>

namespace phosphor::power::regulators
{

std::vector<uint8_t> DBusVPD::getValue(const std::string& inventoryPath,
                                       const std::string& interface,
                                       const std::string& keyword)
{
    std::vector<uint8_t> value{};

    // Get cached keywords for the inventory path
    KeywordMap& cachedKeywords = cache[inventoryPath];

    // Check if the keyword value from this interface is already cached
    auto key = std::make_pair(interface, keyword);
    auto it = cachedKeywords.find(key);
    if (it != cachedKeywords.end())
    {
        // Get keyword value from cache
        value = it->second;
    }
    else
    {
        // Get keyword value from D-Bus interface/property
        getDBusProperty(inventoryPath, interface, keyword, value);

        // Cache keyword value
        cachedKeywords[key] = value;
    }

    return value;
}

void DBusVPD::getDBusProperty(
    const std::string& inventoryPath, const std::string& interface,
    const std::string& keyword, std::vector<uint8_t>& value)
{
    // Determine the D-Bus property name.  Normally this is the same as the VPD
    // keyword name.  However, the CCIN keyword is stored in the Model property
    // of the standard Asset interface.
    std::string property{keyword};
    if ((interface == ASSET_IFACE) && (keyword == "CCIN"))
    {
        property = "Model";
    }

    value.clear();
    try
    {
        // Property value can be either a string or an array of bytes
        std::variant<std::string, std::vector<uint8_t>> propertyValue;
        auto method =
            bus.new_method_call(INVENTORY_MGR_IFACE, inventoryPath.c_str(),
                                util::PROPERTY_INTF, "Get");
        method.append(interface, property);
        auto reply = bus.call(method);
        reply.read(propertyValue);

        if (std::holds_alternative<std::string>(propertyValue))
        {
            const std::string& stringValue =
                std::get<std::string>(propertyValue);
            value.assign(stringValue.begin(), stringValue.end());
        }
        else
        {
            value = std::get<std::vector<uint8_t>>(propertyValue);
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
