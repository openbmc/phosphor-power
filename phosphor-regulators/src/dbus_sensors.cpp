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

#include "dbus_sensors.hpp"

#include <utility>

namespace phosphor::power::regulators
{

void DBusSensors::enable()
{
    // Currently nothing to do here.  The next monitoring cycle will set the
    // values of all sensors, and that will set them to the enabled state.
}

void DBusSensors::endCycle()
{
    // Delete any sensors that were not updated during this monitoring cycle.
    // This can happen if the hardware device producing the sensors was removed
    // or replaced with a different version.
    auto it = sensors.begin();
    while (it != sensors.end())
    {
        // Increment iterator before potentially deleting the sensor.
        // map::erase() invalidates iterators/references to the erased element.
        auto& [sensorName, sensor] = *it;
        ++it;

        // Check if last update time for sensor is before cycle start time
        if (sensor->getLastUpdateTime() < cycleStartTime)
        {
            sensors.erase(sensorName);
        }
    }
}

void DBusSensors::endRail(bool errorOccurred)
{
    // If an error occurred, set all sensors for current rail to the error state
    if (errorOccurred)
    {
        for (auto& [sensorName, sensor] : sensors)
        {
            if (sensor->getRail() == rail)
            {
                sensor->setToErrorState();
            }
        }
    }

    // Clear current rail information
    rail.clear();
    deviceInventoryPath.clear();
    chassisInventoryPath.clear();
}

void DBusSensors::disable()
{
    // Disable all sensors
    for (auto& [sensorName, sensor] : sensors)
    {
        sensor->disable();
    }
}

void DBusSensors::setValue(SensorType type, double value)
{
    // Build unique sensor name based on rail and sensor type
    std::string sensorName{rail + '_' + sensors::toString(type)};

    // Check to see if the sensor already exists
    auto it = sensors.find(sensorName);
    if (it != sensors.end())
    {
        // Sensor exists; update value
        it->second->setValue(value);
    }
    else
    {
        // Sensor doesn't exist; create it and add it to the map
        auto sensor = std::make_unique<DBusSensor>(
            bus, sensorName, type, value, rail, deviceInventoryPath,
            chassisInventoryPath);
        sensors.emplace(sensorName, std::move(sensor));
    }
}

void DBusSensors::startCycle()
{
    // Store the time when this monitoring cycle started.  This is used to
    // detect sensors that were not updated during this cycle.
    cycleStartTime = std::chrono::system_clock::now();
}

void DBusSensors::startRail(const std::string& rail,
                            const std::string& deviceInventoryPath,
                            const std::string& chassisInventoryPath)
{
    // Store current rail information; used later by setValue() and endRail()
    this->rail = rail;
    this->deviceInventoryPath = deviceInventoryPath;
    this->chassisInventoryPath = chassisInventoryPath;
}

} // namespace phosphor::power::regulators
