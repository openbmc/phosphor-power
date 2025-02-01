/**
 * Copyright © 2024 IBM Corporation
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

#include "dbus_interfaces_finder.hpp"

#include <algorithm>
#include <exception>
#include <map>
#include <utility>

namespace phosphor::power::util
{

DBusInterfacesFinder::DBusInterfacesFinder(
    sdbusplus::bus_t& bus, const std::string& service,
    const std::vector<std::string>& interfaces, Callback callback) :
    bus{bus}, service{service}, interfaces{interfaces},
    callback{std::move(callback)},
    match{bus,
          sdbusplus::bus::match::rules::interfacesAdded() +
              sdbusplus::bus::match::rules::sender(service),
          std::bind(&DBusInterfacesFinder::interfacesAddedCallback, this,
                    std::placeholders::_1)}
{
    findInterfaces();
}

void DBusInterfacesFinder::interfacesAddedCallback(
    sdbusplus::message_t& message)
{
    // Exit if message is invalid
    if (!message)
    {
        return;
    }

    try
    {
        // Read the D-Bus message
        sdbusplus::message::object_path path;
        std::map<std::string, std::map<std::string, util::DbusVariant>>
            interfaces;
        message.read(path, interfaces);

        // Call callback for interfaces that we are looking for
        for (const auto& [interface, properties] : interfaces)
        {
            if (std::ranges::contains(this->interfaces, interface))
            {
                callback(path, interface, properties);
            }
        }
    }
    catch (const std::exception&)
    {
        // Error trying to read InterfacesAdded message.  One possible cause
        // could be a property whose value is an unexpected data type.
    }
}

void DBusInterfacesFinder::findInterfaces()
{
    try
    {
        // Use ObjectMapper to find interface instances that already exist
        auto objects = util::getSubTree(bus, "/", interfaces, 0);

        // Search for matching interfaces in returned objects
        for (const auto& [path, services] : objects)
        {
            for (const auto& [service, interfaces] : services)
            {
                if (service == this->service)
                {
                    for (const auto& interface : interfaces)
                    {
                        if (std::ranges::contains(this->interfaces, interface))
                        {
                            auto properties = util::getAllProperties(
                                bus, path, interface, service);
                            callback(path, interface, properties);
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Interface instances might not be available yet
    }
}

} // namespace phosphor::power::util
