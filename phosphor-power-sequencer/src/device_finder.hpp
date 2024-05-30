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

#include <cstdint>
#include <functional>
#include <string>

namespace phosphor::power::sequencer
{

using DbusVariant = phosphor::power::util::DbusVariant;
using DbusPropertyMap = phosphor::power::util::DbusPropertyMap;
using DBusInterfacesFinder = phosphor::power::util::DBusInterfacesFinder;

/**
 * Power sequencer device properties.
 */
struct DeviceProperties
{
    std::string type;
    std::string name;
    uint8_t bus;
    uint16_t address;
};

/**
 * @class DeviceFinder
 *
 * Class that finds power sequencer devices in the system.
 *
 * When a device is found, the callback function specified in the constructor is
 * called.  This function will be called multiple times if multiple devices are
 * found.
 */
class DeviceFinder
{
  public:
    // Specify which compiler-generated methods we want
    DeviceFinder() = delete;
    DeviceFinder(const DeviceFinder&) = delete;
    DeviceFinder(DeviceFinder&&) = delete;
    DeviceFinder& operator=(const DeviceFinder&) = delete;
    DeviceFinder& operator=(DeviceFinder&&) = delete;
    ~DeviceFinder() = default;

    /**
     * Callback function that is called when a power sequencer device is found.
     *
     * @param device Device that was found
     */
    using Callback = std::function<void(const DeviceProperties& device)>;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     * @param callback Callback function that is called each time a power
     *                 sequencer device is found
     */
    explicit DeviceFinder(sdbusplus::bus_t& bus, Callback callback);

    /**
     * Callback function that is called when a D-Bus interface is found that
     * contains power sequencer device properties.
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
     * Returns the value of the D-Bus property with the specified name.
     *
     * Throws an exception if the property was not found.
     *
     * @param properties D-Bus interface properties
     * @param propertyName D-Bus property name
     * @return Property value
     */
    const DbusVariant& getPropertyValue(const DbusPropertyMap& properties,
                                        const std::string& propertyName);

    /**
     * Callback function that is called each time a power sequencer device is
     * found.
     */
    Callback callback;

    /**
     * Class used to find D-Bus interfaces that contain power sequencer device
     * properties.
     */
    DBusInterfacesFinder interfacesFinder;
};

} // namespace phosphor::power::sequencer
