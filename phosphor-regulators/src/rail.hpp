/**
 * Copyright © 2019 IBM Corporation
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

#include "configuration.hpp"
#include "sensor_monitoring.hpp"
#include "services.hpp"

#include <memory>
#include <string>
#include <utility>

namespace phosphor::power::regulators
{

// Forward declarations to avoid circular dependencies
class Chassis;
class Device;
class System;

/**
 * @class Rail
 *
 * A voltage rail produced by a voltage regulator.
 *
 * Voltage regulators produce one or more rails.  Each rail typically provides a
 * different output voltage level, such as 1.1V.
 */
class Rail
{
  public:
    // Specify which compiler-generated methods we want
    Rail() = delete;
    Rail(const Rail&) = delete;
    Rail(Rail&&) = delete;
    Rail& operator=(const Rail&) = delete;
    Rail& operator=(Rail&&) = delete;
    ~Rail() = default;

    /**
     * Constructor.
     *
     * @param id unique rail ID
     * @param configuration configuration changes to apply to this rail, if any
     * @param sensorMonitoring sensor monitoring for this rail, if any
     */
    explicit Rail(
        const std::string& id,
        std::unique_ptr<Configuration> configuration = nullptr,
        std::unique_ptr<SensorMonitoring> sensorMonitoring = nullptr) :
        id{id}, configuration{std::move(configuration)},
        sensorMonitoring{std::move(sensorMonitoring)}
    {}

    /**
     * Clears all error history.
     *
     * All data on previously logged errors will be deleted.  If errors occur
     * again in the future they will be logged again.
     *
     * This method is normally called when the system is being powered on.
     */
    void clearErrorHistory();

    /**
     * Configure this rail.
     *
     * Applies the configuration changes that are defined for this rail, if any.
     *
     * This method should be called during the boot before regulators are
     * enabled.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains the chassis
     * @param chassis chassis that contains the device
     * @param device device that contains this rail
     */
    void configure(Services& services, System& system, Chassis& chassis,
                   Device& device);

    /**
     * Returns the configuration changes to apply to this rail, if any.
     *
     * @return Pointer to Configuration object.  Will equal nullptr if no
     *         configuration changes are defined for this rail.
     */
    const std::unique_ptr<Configuration>& getConfiguration() const
    {
        return configuration;
    }

    /**
     * Returns the unique ID of this rail.
     *
     * @return rail ID
     */
    const std::string& getID() const
    {
        return id;
    }

    /**
     * Monitor the sensors for this rail.
     *
     * Sensor monitoring is optional.  If sensor monitoring is defined for this
     * rail, the sensor values are read.
     *
     * This method should be called repeatedly based on a timer.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains the chassis
     * @param chassis chassis that contains the device
     * @param device device that contains this rail
     */
    void monitorSensors(Services& services, System& system, Chassis& chassis,
                        Device& device);

    /**
     * Returns the sensor monitoring for this rail, if any.
     *
     * @return Pointer to SensorMonitoring object.  Will equal nullptr if no
     *         sensor monitoring is defined for this rail.
     */
    const std::unique_ptr<SensorMonitoring>& getSensorMonitoring() const
    {
        return sensorMonitoring;
    }

  private:
    /**
     * Unique ID of this rail.
     */
    const std::string id{};

    /**
     * Configuration changes to apply to this rail, if any.  Set to nullptr if
     * no configuration changes are defined for this rail.
     */
    std::unique_ptr<Configuration> configuration{};

    /**
     * Sensor monitoring for this rail, if any.  Set to nullptr if no sensor
     * monitoring is defined for this rail.
     */
    std::unique_ptr<SensorMonitoring> sensorMonitoring{};
};

} // namespace phosphor::power::regulators
