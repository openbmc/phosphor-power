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

#include "action.hpp"
#include "error_history.hpp"
#include "services.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

// Forward declarations to avoid circular dependencies
class Chassis;
class Device;
class Rail;
class System;

/**
 * @class SensorMonitoring
 *
 * Defines how to read the sensors for a voltage rail, such as voltage output,
 * current output, and temperature.
 *
 * Sensor values are measured, actual values rather than target values.
 *
 * Sensors are read repeatedly based on a timer.  The sensor values are stored
 * on D-Bus, making them available to external interfaces like Redfish.
 *
 * Sensors are read by executing actions, such as PMBusReadSensorAction.  To
 * read multiple sensors for a rail, multiple actions need to be executed.
 */
class SensorMonitoring
{
  public:
    // Specify which compiler-generated methods we want
    SensorMonitoring() = delete;
    SensorMonitoring(const SensorMonitoring&) = delete;
    SensorMonitoring(SensorMonitoring&&) = delete;
    SensorMonitoring& operator=(const SensorMonitoring&) = delete;
    SensorMonitoring& operator=(SensorMonitoring&&) = delete;
    ~SensorMonitoring() = default;

    /**
     * Constructor.
     *
     * @param actions actions that read the sensors for a rail
     */
    explicit SensorMonitoring(std::vector<std::unique_ptr<Action>> actions) :
        actions{std::move(actions)}
    {}

    /**
     * Clears all error history.
     *
     * All data on previously logged errors will be deleted.  If errors occur
     * again in the future they will be logged again.
     *
     * This method is normally called when the system is being powered on.
     */
    void clearErrorHistory()
    {
        errorHistory.clear();
        errorCount = 0;
    }

    /**
     * Executes the actions to read the sensors for a rail.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains the chassis
     * @param chassis chassis that contains the device
     * @param device device that contains the rail
     * @param rail rail associated with the sensors
     */
    void execute(Services& services, System& system, Chassis& chassis,
                 Device& device, Rail& rail);

    /**
     * Returns the actions that read the sensors for a rail.
     *
     * @return actions
     */
    const std::vector<std::unique_ptr<Action>>& getActions() const
    {
        return actions;
    }

  private:
    /**
     * Actions that read the sensors for a rail.
     */
    std::vector<std::unique_ptr<Action>> actions{};

    /**
     * History of which error types have been logged.
     *
     * Since sensor monitoring runs repeatedly based on a timer, each error type
     * is only logged once.
     */
    ErrorHistory errorHistory{};

    /**
     * Number of consecutive errors that have occurred.
     */
    unsigned short errorCount{0};
};

} // namespace phosphor::power::regulators
