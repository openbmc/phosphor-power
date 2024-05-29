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
#pragma once

#include "dbus_interfaces_finder.hpp"
#include "utility.hpp"

#include <sdbusplus/bus.hpp>

#include <functional>
#include <string>
#include <vector>

namespace phosphor::power::util
{

/**
 * @class CompatibleSystemTypesFinder
 *
 * Class that finds the compatible system types for the current system.
 *
 * The compatible system types are in a list ordered from most to least
 * specific.
 *
 * Example:
 *   - com.acme.Hardware.Chassis.Model.MegaServer4CPU
 *   - com.acme.Hardware.Chassis.Model.MegaServer
 *   - com.acme.Hardware.Chassis.Model.Server
 *
 * When a list of compatible system types is found, the callback function
 * specified in the constructor is called.  This function will be called
 * multiple times if multiple lists of compatible system types are found.
 */
class CompatibleSystemTypesFinder
{
  public:
    // Specify which compiler-generated methods we want
    CompatibleSystemTypesFinder() = delete;
    CompatibleSystemTypesFinder(const CompatibleSystemTypesFinder&) = delete;
    CompatibleSystemTypesFinder(CompatibleSystemTypesFinder&&) = delete;
    CompatibleSystemTypesFinder&
        operator=(const CompatibleSystemTypesFinder&) = delete;
    CompatibleSystemTypesFinder&
        operator=(CompatibleSystemTypesFinder&&) = delete;
    ~CompatibleSystemTypesFinder() = default;

    /**
     * Callback function that is called when a list of compatible system types
     * is found.
     *
     * @param compatibleSystemTypes Compatible system types for the current
     *                              system ordered from most to least specific
     */
    using Callback = std::function<void(
        const std::vector<std::string>& compatibleSystemTypes)>;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     * @param callback Callback function that is called each time a list of
     *                 compatible system types is found
     */
    explicit CompatibleSystemTypesFinder(sdbusplus::bus_t& bus,
                                         Callback callback);

    /**
     * Callback function that is called when a Compatible interface is found.
     *
     * @param path D-Bus object path that implements the interface
     * @param interface D-Bus interface that was found
     * @param properties Properties of the D-Bus interface
     */
    void interfaceFoundCallback(const std::string& path,
                                const std::string& interface,
                                const DbusPropertyMap& properties);

  private:
    /**
     * Callback function that is called each time a list of compatible system
     * types is found.
     */
    Callback callback;

    /**
     * Class used to find instances of the D-Bus Compatible interface.
     */
    DBusInterfacesFinder interfaceFinder;
};

} // namespace phosphor::power::util
