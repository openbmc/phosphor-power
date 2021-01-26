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
#pragma once

#include "services.hpp"
#include "system.hpp"

#include <interfaces/manager_interface.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace phosphor::power::regulators
{

using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

using ManagerObject = sdbusplus::server::object::object<
    phosphor::power::regulators::interface::ManagerInterface>;

class Manager : public ManagerObject
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /**
     * Constructor
     * Creates a manager over the regulators.
     *
     * @param bus the D-Bus bus
     * @param event the sdevent event
     */
    Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /**
     * Overridden manager object's configure method
     */
    void configure() override;

    /**
     * Callback function to handle interfacesAdded D-Bus signals
     *
     * @param msg Expanded sdbusplus message data
     */
    void interfacesAddedHandler(sdbusplus::message::message& msg);

    /**
     * Overridden manager object's monitor method
     *
     * @param enable Enable or disable regulator monitoring
     */
    void monitor(bool enable) override;

    /**
     * Callback function to handle receiving a HUP signal
     * to reload the configuration data.
     *
     * @param sigSrc sd_event_source signal wrapper
     * @param sigInfo signal info on signal fd
     */
    void sighupHandler(sdeventplus::source::Signal& sigSrc,
                       const struct signalfd_siginfo* sigInfo);

    /**
     * Timer expired callback function
     */
    void timerExpired();

  private:
    /**
     * Clear any cached data or error history related to hardware devices.
     *
     * This method should be called when the system is powering on (booting).
     * While the system was powered off (at standby), hardware could have been
     * added, removed, or replaced.
     */
    void clearHardwareData();

    /**
     * Finds the list of compatible system types using D-Bus methods.
     *
     * This list is used to find the correct JSON configuration file for the
     * current system.
     *
     * Note that some systems do not support the D-Bus compatible interface.
     *
     * If a list of compatible system types is found, it is stored in the
     * compatibleSystemTypes data member.
     */
    void findCompatibleSystemTypes();

    /**
     * Finds the JSON configuration file.
     *
     * Looks for a configuration file based on the list of compatable system
     * types.  If no file is found, looks for a file with the default name.
     *
     * Looks for the file in the test directory and standard directory.
     *
     * Throws an exception if an operating system error occurs while checking
     * for the existance of a file.
     *
     * @return absolute path to config file, or an empty path if none found
     */
    std::filesystem::path findConfigFile();

    /**
     * Loads the JSON configuration file.
     *
     * Looks for the config file using findConfigFile().
     *
     * If the config file is found, it is parsed and the resulting information
     * is stored in the system data member.  If parsing fails, an error is
     * logged.
     */
    void loadConfigFile();

    /**
     * The D-Bus bus
     */
    sdbusplus::bus::bus& bus;

    /**
     * Event to loop on
     */
    sdeventplus::Event eventLoop;

    /**
     * System services like error logging and the journal.
     */
    BMCServices services;

    /**
     * List of event timers
     */
    std::vector<Timer> timers{};

    /**
     * List of dbus signal matches
     */
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> signals{};

    /**
     * List of compatible system types for the current system.
     *
     * Used to find the JSON configuration file.
     */
    std::vector<std::string> compatibleSystemTypes{};

    /**
     * Computer system being controlled and monitored by the BMC.
     *
     * Contains the information loaded from the JSON configuration file.
     * Contains nullptr if the configuration file has not been loaded.
     */
    std::unique_ptr<System> system{};
};

} // namespace phosphor::power::regulators
