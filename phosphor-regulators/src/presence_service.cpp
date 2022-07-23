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
    // Initially assume hardware is not present
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
        try
        {
            util::getProperty(INVENTORY_IFACE, PRESENT_PROP, inventoryPath,
                              INVENTORY_MGR_IFACE, bus, present);
        }
        catch (const sdbusplus::exception_t& e)
        {
            // If exception type is expected and indicates hardware not present
            if (isExpectedException(e))
            {
                present = false;
            }
            else
            {
                // Re-throw unexpected exception
                throw;
            }
        }

        // Cache presence value
        cache[inventoryPath] = present;
    }

    return present;
}

bool DBusPresenceService::isExpectedException(const sdbusplus::exception_t& e)
{
    // Initially assume exception is not one of the expected types
    bool isExpected{false};

    // If the D-Bus error name is set within the exception
    if (e.name() != nullptr)
    {
        // Check if the error name is one of the expected values when hardware
        // is not present.
        //
        // Sometimes the object path does not exist.  Sometimes the object path
        // exists, but it does not implement the D-Bus interface that contains
        // the present property.  Both of these cases result in exceptions.
        //
        // In the case where the interface is not implemented, the systemd
        // documentation seems to indicate that the error name should be
        // SD_BUS_ERROR_UNKNOWN_INTERFACE.  However, in OpenBMC the
        // SD_BUS_ERROR_UNKNOWN_PROPERTY error name can occur.
        std::string name = e.name();
        if ((name == SD_BUS_ERROR_UNKNOWN_OBJECT) ||
            (name == SD_BUS_ERROR_UNKNOWN_INTERFACE) ||
            (name == SD_BUS_ERROR_UNKNOWN_PROPERTY))
        {
            isExpected = true;
        }
    }

    return isExpected;
}

} // namespace phosphor::power::regulators
