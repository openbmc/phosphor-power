/**
 * Copyright © 2026 IBM Corporation
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
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/Chassis/server.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace phosphor::power::chassis
{
/*
 * @class Manager
 *
 * This class is responsible for monitoring chassis.
 *
 */
class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /**
     * Constructor to initialize the Manager object.
     *
     * Throws an exception if an error occurs during initialization.
     *
     * @param event - Dbus event object.
     * @param services - Platform services provider
     */
    Manager(const sdeventplus::Event& event, Services& services);

    /**
     * Callback that is called when a list of compatible system types is found.
     *
     * @param types Compatible system types for the current system ordered from
     *              most to least specific
     */
    void compatibleSystemTypesFound(const std::vector<std::string>& types);

    /**
     * Returns whether the JSON configuration file has been loaded.
     *
     * @return true if config file loaded, false otherwise
     */
    bool isConfigFileLoaded() const
    {
        // If System object exists, the config file has been loaded
        return (system != nullptr);
    }

    /**
     * Returns the list of compatible system types.
     *
     * @return list of compatible system types
     */
    const std::vector<std::string>& getCompatibleSystemTypes()
    {
        return compatibleSystemTypes;
    }

    /**
     * Clears the compatible system types and config file state.
     *
     * This method is intended for testing only. It allows tests to reset the
     * Manager state so multiple test scenarios can be run with a single
     * Manager instance.
     */
    void clearCompatibleSystemTypes()
    {
        compatibleSystemTypes.clear();
        system.reset();
    }

    /**
     * Clears hardware error history for all chassis.
     */
    void clearErrorHistory();

  private:
    /**
     * Callback to begin failure process after not finding a Compatible system.
     */
    void compatibleSystemTypesNotFoundCallback();

    /**
     * Callback to monitor the system.
     */
    void monitor();

    /**
     * Finds the JSON configuration file.
     *
     * Looks for a configuration file based on the list of compatible system
     * types.  If no file is found, logs an error to the journal.
     *
     * Looks for the file in the test directory and standard directory.
     *
     * Throws an exception if an operating system error occurs while checking
     * for the existence of a file.
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
     * Callback function to handle chassis power state changes.
     *
     * @param msg D-Bus message containing the property change
     */
    void chassisPowerStateChanged(sdbusplus::message_t& msg);

    /**
     * Handle BMC reset for all chassis.
     * Called during initialization to set up GPIO states.
     */
    void handleBMCReset();

    /**
     * Setup D-Bus matches for monitoring chassis power states.
     */
    void setupChassisMatches();

    /**
     * Event to loop on
     */
    const sdeventplus::Event& eventLoop [[maybe_unused]];

    /**
     * Timer to wait for a Compatible system.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>
        compatibleSystemsTimer;

    /**
     * Timer that triggers periodic system monitoring.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> monitorTimer;

    /**
     * Object that finds the compatible system types for the current system.
     */

    std::unique_ptr<util::CompatibleSystemTypesFinder> compatSysTypesFinder;

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
    std::unique_ptr<System> system;

    /**
     * Platform services provider.
     */
    Services& services;

    /**
     * D-Bus match object for monitoring chassis 0 power state changes.
     */
    std::unique_ptr<sdbusplus::match> chassisPowerStateMatch;
};

} // namespace phosphor::power::chassis
