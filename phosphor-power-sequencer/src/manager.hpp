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
#pragma once

#include "compatible_system_types_finder.hpp"
#include "services.hpp"
#include "system.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class Manager
 *
 * This is the top level class within the phosphor-power-sequencer application.
 *
 * This class is responsible for the following:
 * - Finds system type if system-specific JSON configuration file is being used
 * - Finds and loads the JSON configuration file
 * - Monitors the state of the system and chassis once per second
 */
class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /**
     * Constructor.

     * @param bus D-Bus bus object
     * @param event Event object
     */
    Manager(sdbusplus::bus_t& bus, const sdeventplus::Event& event);

    /**
     * Callback that is called when a list of compatible system types is found.
     *
     * @param types Compatible system types for the current system ordered from
     *              most to least specific
     */
    void compatibleSystemTypesFound(const std::vector<std::string>& types);

    /**
     * Monitors the system and chassis.
     *
     * Detects the following in the system and chassis:
     * - Power good changes
     * - Chassis status changes, such as presence and availability
     * - Power good faults
     *
     * MUST be called once per second so that delays and timeouts within the
     * monitoring code will work properly.
     */
    void monitor();

  private:
    /**
     * Finds the JSON configuration file.
     *
     * If the default config file is being used, the path is found, and the file
     * is loaded.
     *
     * If a system-specific file is being used, the D-Bus Compatible interface
     * will be found. The compatible names from the interface will be used to
     * find the system-specific config file path and load it.
     */
    void findConfigFile();

    /**
     * Loads the JSON configuration file.
     *
     * The file is parsed, and the resulting information is stored in the system
     * data member.
     */
    void loadConfigFile();

    /**
     * The D-Bus bus object
     */
    sdbusplus::bus_t& bus;

    /**
     * D-Bus object manager.
     *
     * Causes this application to implement the
     * org.freedesktop.DBus.ObjectManager interface.
     */
    sdbusplus::server::manager_t objectManager;

    /**
     * System services like hardware presence and the journal.
     */
    BMCServices services;

    /**
     * Defines how often the timer should call the monitor() function.
     *
     * MUST be 1 second in duration. See monitor() for more information.
     */
    static constexpr std::chrono::seconds monitorInterval{1};

    /**
     * Timer that calls monitor() to monitor the system and chassis.
     *
     * See monitor() for more information.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> monitorTimer;

    /**
     * JSON configuration file path.
     */
    std::filesystem::path configFilePath;

    /**
     * Object that finds the compatible system types for the current system.
     */
    std::unique_ptr<util::CompatibleSystemTypesFinder> compatSysTypesFinder;

    /**
     * Computer system being controlled and monitored by the BMC.
     *
     * Contains the information loaded from the JSON configuration file.
     * Contains nullptr if the configuration file has not been loaded.
     */
    std::unique_ptr<System> system;
};

} // namespace phosphor::power::sequencer
