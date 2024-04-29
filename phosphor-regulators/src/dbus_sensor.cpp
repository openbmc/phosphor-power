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

#include "dbus_sensor.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace phosphor::power::regulators
{

/**
 * Constants for current sensors.
 *
 * Values are in amperes.
 */
constexpr double currentMinValue = 0.0;
constexpr double currentMaxValue = 500.0;
constexpr double currentHysteresis = 1.0;
constexpr const char* currentNamespace = "current";

/**
 * Constants for power sensors.
 *
 * Values are in watts.
 */
constexpr double powerMinValue = 0.0;
constexpr double powerMaxValue = 1000.0;
constexpr double powerHysteresis = 1.0;
constexpr const char* powerNamespace = "power";

/**
 * Constants for temperature sensors.
 *
 * Values are in degrees Celsius.
 */
constexpr double temperatureMinValue = -50.0;
constexpr double temperatureMaxValue = 250.0;
constexpr double temperatureHysteresis = 1.0;
constexpr const char* temperatureNamespace = "temperature";

/**
 * Constants for voltage sensors.
 *
 * Values are in volts.
 *
 * Note the hysteresis value is very low.  Small voltage changes can have a big
 * impact in some systems.  The sensors need to reflect these small changes.
 */
constexpr double voltageMinValue = -15.0;
constexpr double voltageMaxValue = 15.0;
constexpr double voltageHysteresis = 0.001;
constexpr const char* voltageNamespace = "voltage";

DBusSensor::DBusSensor(sdbusplus::bus_t& bus, const std::string& name,
                       SensorType type, double value, const std::string& rail,
                       const std::string& deviceInventoryPath,
                       const std::string& chassisInventoryPath) :
    bus{bus}, name{name}, type{type}, rail{rail}
{
    // Get sensor properties that are based on the sensor type
    std::string objectPath;
    Unit unit;
    double minValue, maxValue;
    getTypeBasedProperties(objectPath, unit, minValue, maxValue);

    // Get the D-Bus associations to create for this sensor
    std::vector<AssocationTuple> associations =
        getAssociations(deviceInventoryPath, chassisInventoryPath);

    // Create the sdbusplus object that implements the D-Bus sensor interfaces.
    // Skip emitting D-Bus signals until the object has been fully created.
    dbusObject = std::make_unique<DBusSensorObject>(
        bus, objectPath.c_str(), DBusSensorObject::action::defer_emit);

    // Set properties of the Value interface
    constexpr auto skipSignal = true;
    dbusObject->value(value, skipSignal);
    dbusObject->maxValue(maxValue, skipSignal);
    dbusObject->minValue(minValue, skipSignal);
    dbusObject->unit(unit, skipSignal);

    // Set properties of the OperationalStatus interface
    dbusObject->functional(true, skipSignal);

    // Set properties of the Availability interface
    dbusObject->available(true, skipSignal);

    // Set properties on the Association.Definitions interface
    dbusObject->associations(std::move(associations), skipSignal);

    // Now emit signal that object has been created
    dbusObject->emit_object_added();

    // Set the last update time
    setLastUpdateTime();
}

void DBusSensor::disable()
{
    // Set sensor value to NaN
    setValueToNaN();

    // Set the sensor to unavailable since it is disabled
    dbusObject->available(false);

    // Set the last update time
    setLastUpdateTime();
}

void DBusSensor::setToErrorState()
{
    // Set sensor value to NaN
    setValueToNaN();

    // Set the sensor to non-functional since it could not be read
    dbusObject->functional(false);

    // Set the last update time
    setLastUpdateTime();
}

void DBusSensor::setValue(double value)
{
    // Update value on D-Bus if necessary
    if (shouldUpdateValue(value))
    {
        dbusObject->value(value);
    }

    // Set the sensor to functional since it has a valid value
    dbusObject->functional(true);

    // Set the sensor to available since it is not disabled
    dbusObject->available(true);

    // Set the last update time
    setLastUpdateTime();
}

std::vector<AssocationTuple>
    DBusSensor::getAssociations(const std::string& deviceInventoryPath,
                                const std::string& chassisInventoryPath)
{
    std::vector<AssocationTuple> associations{};

    // Add an association between the sensor and the chassis.  This is used by
    // the Redfish support to find all the sensors in a chassis.
    associations.emplace_back(
        std::make_tuple("chassis", "all_sensors", chassisInventoryPath));

    // Add an association between the sensor and the voltage regulator device.
    // This is used by the Redfish support to find the hardware/inventory item
    // associated with a sensor.
    associations.emplace_back(
        std::make_tuple("inventory", "sensors", deviceInventoryPath));

    return associations;
}

void DBusSensor::getTypeBasedProperties(std::string& objectPath, Unit& unit,
                                        double& minValue, double& maxValue)
{
    const char* typeNamespace{""};
    switch (type)
    {
        case SensorType::iout:
            typeNamespace = currentNamespace;
            unit = Unit::Amperes;
            minValue = currentMinValue;
            maxValue = currentMaxValue;
            updatePolicy = ValueUpdatePolicy::hysteresis;
            hysteresis = currentHysteresis;
            break;

        case SensorType::iout_peak:
            typeNamespace = currentNamespace;
            unit = Unit::Amperes;
            minValue = currentMinValue;
            maxValue = currentMaxValue;
            updatePolicy = ValueUpdatePolicy::highest;
            break;

        case SensorType::iout_valley:
            typeNamespace = currentNamespace;
            unit = Unit::Amperes;
            minValue = currentMinValue;
            maxValue = currentMaxValue;
            updatePolicy = ValueUpdatePolicy::lowest;
            break;

        case SensorType::pout:
            typeNamespace = powerNamespace;
            unit = Unit::Watts;
            minValue = powerMinValue;
            maxValue = powerMaxValue;
            updatePolicy = ValueUpdatePolicy::hysteresis;
            hysteresis = powerHysteresis;
            break;

        case SensorType::temperature:
            typeNamespace = temperatureNamespace;
            unit = Unit::DegreesC;
            minValue = temperatureMinValue;
            maxValue = temperatureMaxValue;
            updatePolicy = ValueUpdatePolicy::hysteresis;
            hysteresis = temperatureHysteresis;
            break;

        case SensorType::temperature_peak:
            typeNamespace = temperatureNamespace;
            unit = Unit::DegreesC;
            minValue = temperatureMinValue;
            maxValue = temperatureMaxValue;
            updatePolicy = ValueUpdatePolicy::highest;
            break;

        case SensorType::vout:
            typeNamespace = voltageNamespace;
            unit = Unit::Volts;
            minValue = voltageMinValue;
            maxValue = voltageMaxValue;
            updatePolicy = ValueUpdatePolicy::hysteresis;
            hysteresis = voltageHysteresis;
            break;

        case SensorType::vout_peak:
            typeNamespace = voltageNamespace;
            unit = Unit::Volts;
            minValue = voltageMinValue;
            maxValue = voltageMaxValue;
            updatePolicy = ValueUpdatePolicy::highest;
            break;

        case SensorType::vout_valley:
        default:
            typeNamespace = voltageNamespace;
            unit = Unit::Volts;
            minValue = voltageMinValue;
            maxValue = voltageMaxValue;
            updatePolicy = ValueUpdatePolicy::lowest;
            break;
    }

    // Build object path
    objectPath = sensorsObjectPath;
    objectPath += '/';
    objectPath += typeNamespace;
    objectPath += '/';
    objectPath += name;
}

void DBusSensor::setValueToNaN()
{
    // Get current value published on D-Bus
    double currentValue = dbusObject->value();

    // Check if current value is already NaN.  We want to avoid an unnecessary
    // PropertiesChanged signal.  The generated C++ code for the Value interface
    // does check whether the new value is different from the old one.  However,
    // it uses the equality operator, and NaN always returns false when compared
    // to another NaN value.
    if (!std::isnan(currentValue))
    {
        // Set value to NaN
        dbusObject->value(std::numeric_limits<double>::quiet_NaN());
    }
}

bool DBusSensor::shouldUpdateValue(double value)
{
    // Initially assume we should update the value
    bool shouldUpdate{true};

    // Get current value published on D-Bus
    double currentValue = dbusObject->value();

    // Update sensor if the current value is NaN.  This indicates it was
    // disabled or in an error state.  Note: you cannot compare a variable to
    // NaN directly using the equality operator; it will always return false.
    if (std::isnan(currentValue))
    {
        shouldUpdate = true;
    }
    else
    {
        // Determine whether to update based on policy used by this sensor
        switch (updatePolicy)
        {
            case ValueUpdatePolicy::hysteresis:
                shouldUpdate = (std::abs(value - currentValue) >= hysteresis);
                break;
            case ValueUpdatePolicy::highest:
                shouldUpdate = (value > currentValue);
                break;
            case ValueUpdatePolicy::lowest:
                shouldUpdate = (value < currentValue);
                break;
        }
    }

    return shouldUpdate;
}

} // namespace phosphor::power::regulators
