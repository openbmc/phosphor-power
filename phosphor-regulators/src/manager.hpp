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

#include "compatible_system_types_finder.hpp"
#include "services.hpp"
#include "system.hpp"

#include <interfaces/manager_interface.hpp>
#include <sdbusplus/bus.hpp>
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

using ManagerObject = sdbusplus::server::object_t<
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
    Manager(sdbusplus::bus_t& bus, const sdeventplus::Event& event);

    /**
     * Implements the D-Bus "configure" method.
     *
     * Configures all the voltage regulators in the system.
     *
     * This method should be called when the system is being powered on.  It
     * needs to occur before the regulators have been enabled/turned on.
     */
    void configure() override;

    /**
     * Implements the D-Bus "monitor" method.
     *
     * Sets whether regulator monitoring is enabled.
     *
     * When monitoring is enabled:
     *   - regulator sensors will be read and published on D-Bus
     *   - phase fault detection will be performed
     *
     * Regulator monitoring should be enabled when the system is being powered
     * on.  It needs to occur after the regulators have been configured and
     * enabled/turned on.
     *
     * Regulator monitoring should be disabled when the system is being powered
     * off.  It needs to occur before the regulators have been disabled/turned
     * off.
     *
     * Regulator monitoring can also be temporarily disabled and then re-enabled
     * while the system is powered on.  This allows other applications or tools
     * to temporarily communicate with the regulators for testing or debug.
     * Monitoring should be disabled for only short periods of time; other
     * applications, such as fan control, may be dependent on regulator sensors.
     *
     * @param enable true if monitoring should be enabled, false if it should be
     *               disabled
     */
    void monitor(bool enable) override;

    /**
     * Callback that is called when a list of compatible system types is found.
     *
     * @param types Compatible system types for the current system ordered from
     *              most to least specific
     */
    void compatibleSystemTypesFound(const std::vector<std::string>& types);

    /**
     * Phase fault detection timer expired callback function.
     */
    void phaseFaultTimerExpired();

    /**
     * Sensor monitoring timer expired callback function.
     */
    void sensorTimerExpired();

    /**
     * Callback function to handle receiving a HUP signal
     * to reload the configuration data.
     *
     * @param sigSrc sd_event_source signal wrapper
     * @param sigInfo signal info on signal fd
     */
    void sighupHandler(sdeventplus::source::Signal& sigSrc,
                       const struct signalfd_siginfo* sigInfo);

  private:
    /**
     * Clear any cached data or error history related to hardware devices.
     *
     * This method should be called when the system is powering on (booting).
     * While the system was powered off, hardware could have been added,
     * removed, or replaced.
     */
    void clearHardwareData();

    /**
     * Finds the JSON configuration file.
     *
     * Looks for a configuration file based on the list of compatible system
     * types.  If no file is found, looks for a file with the default name.
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
     * Returns whether the system is currently powered on.
     *
     * @return true if system is powered on, false otherwise
     */
    bool isSystemPoweredOn();

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
     * Waits until the JSON configuration file has been loaded.
     *
     * If the config file has not yet been loaded, waits until one of the
     * following occurs:
     * - config file is loaded
     * - maximum amount of time to wait has elapsed
     */
    void waitUntilConfigFileLoaded();

    /**
     * The D-Bus bus
     */
    sdbusplus::bus_t& bus;

    /**
     * Event to loop on
     */
    const sdeventplus::Event& eventLoop [[maybe_unused]];

    /**
     * System services like error logging and the journal.
     */
    BMCServices services;

    /**
     * Object that finds the compatible system types for the current system.
     */
    std::unique_ptr<util::CompatibleSystemTypesFinder> compatSysTypesFinder;

    /**
     * Event timer used to initiate phase fault detection.
     */
    Timer phaseFaultTimer;

    /**
     * Event timer used to initiate sensor monitoring.
     */
    Timer sensorTimer;

    /**
     * Indicates whether regulator monitoring is enabled.
     */
    bool isMonitoringEnabled{false};

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
