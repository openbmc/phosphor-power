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

#include "utility.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <functional>
#include <string>
#include <vector>

namespace phosphor::power::util
{

/**
 * @class DBusInterfacesFinder
 *
 * Class that finds instances of one or more D-Bus interfaces.
 *
 * A D-Bus service name and one or more D-Bus interfaces are specified in the
 * constructor.  The class finds instances of those interfaces that are owned by
 * the service.
 *
 * The class finds the instances using two different methods:
 * - Registers an InterfacesAdded listener for the specified service.  Class is
 *   notified when a new interface instance is created on D-Bus.
 * - Queries the ObjectMapper to find interface instances that already exist.
 *
 * Utilizing both methods allows this class to be used before, during, or after
 * the service has created the interface instances.
 *
 * When an interface instance is found, the callback function specified in the
 * constructor is called.  This function will be called multiple times if
 * multiple instances are found.
 */
class DBusInterfacesFinder
{
  public:
    // Specify which compiler-generated methods we want
    DBusInterfacesFinder() = delete;
    DBusInterfacesFinder(const DBusInterfacesFinder&) = delete;
    DBusInterfacesFinder(DBusInterfacesFinder&&) = delete;
    DBusInterfacesFinder& operator=(const DBusInterfacesFinder&) = delete;
    DBusInterfacesFinder& operator=(DBusInterfacesFinder&&) = delete;
    ~DBusInterfacesFinder() = default;

    /**
     * Callback function that is called when an interface instance is found.
     *
     * @param path D-Bus object path that implements the interface
     * @param interface D-Bus interface that was found
     * @param properties Properties of the D-Bus interface
     */
    using Callback = std::function<void(const std::string& path,
                                        const std::string& interface,
                                        const DbusPropertyMap& properties)>;

    /**
     * Constructor.
     *
     * Note: The callback function may be called immediately by this
     * constructor.  For this reason, do not use this constructor in the
     * initialization list of constructors in other classes.  Otherwise the
     * callback may be called before the other class is fully initialized,
     * leading to unpredictable behavior.
     *
     * @param bus D-Bus bus object
     * @param service D-Bus service that owns the object paths implementing
     *                the specified interfaces
     * @param interfaces D-Bus interfaces to find
     * @param callback Callback function that is called each time an interface
     *                 instance is found
     */
    explicit DBusInterfacesFinder(
        sdbusplus::bus_t& bus, const std::string& service,
        const std::vector<std::string>& interfaces, Callback callback);

    /**
     * Refind all instances of the interfaces specified in the constructor.
     *
     * The callback specified in the constructor will be called for each
     * instance found.
     *
     * This method normally does not need to be called.  New instances are
     * automatically detected using an InterfacesAdded listener.  However, this
     * method may be useful if the caller is not currently receiving D-Bus
     * signals (such as within a loop).
     */
    void refind()
    {
        findInterfaces();
    }

    /**
     * Callback function to handle InterfacesAdded D-Bus signals
     *
     * @param message Expanded sdbusplus message data
     */
    void interfacesAddedCallback(sdbusplus::message_t& message);

  private:
    /**
     * Finds any interface instances that already exist on D-Bus.
     */
    void findInterfaces();

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t& bus;

    /**
     * D-Bus service that owns the object paths implementing the interfaces.
     */
    std::string service;

    /**
     * D-Bus interfaces to find.
     */
    std::vector<std::string> interfaces;

    /**
     * Callback function that is called each time an interface instance is
     * found.
     */
    Callback callback;

    /**
     * Match object for InterfacesAdded signals.
     */
    sdbusplus::bus::match_t match;
};

} // namespace phosphor::power::util
