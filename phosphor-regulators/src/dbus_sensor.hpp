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

#include "sensors.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Association/Definitions/server.hpp>
#include <xyz/openbmc_project/Sensor/Value/server.hpp>
#include <xyz/openbmc_project/State/Decorator/Availability/server.hpp>
#include <xyz/openbmc_project/State/Decorator/OperationalStatus/server.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * Define simple name for the generated C++ class that implements the
 * xyz.openbmc_project.Sensor.Value interface.
 */
using ValueInterface = sdbusplus::xyz::openbmc_project::Sensor::server::Value;

/**
 * Define simple name for the generated C++ class that implements the
 * xyz.openbmc_project.State.Decorator.OperationalStatus interface.
 */
using OperationalStatusInterface = sdbusplus::xyz::openbmc_project::State::
    Decorator::server::OperationalStatus;

/**
 * Define simple name for the generated C++ class that implements the
 * xyz.openbmc_project.State.Decorator.Availability interface.
 */
using AvailabilityInterface =
    sdbusplus::xyz::openbmc_project::State::Decorator::server::Availability;

/**
 * Define simple name for the generated C++ class that implements the
 * xyz.openbmc_project.Association.Definitions interface.
 */
using AssociationDefinitionsInterface =
    sdbusplus::xyz::openbmc_project::Association::server::Definitions;

/**
 * Define simple name for the sdbusplus object_t class that implements all
 * the necessary D-Bus interfaces via templates/multiple inheritance.
 */
using DBusSensorObject = sdbusplus::server::object_t<
    ValueInterface, OperationalStatusInterface, AvailabilityInterface,
    AssociationDefinitionsInterface>;

/**
 * Define simple name for the generated C++ enum that implements the
 * valid sensor Unit values on D-Bus.
 */
using Unit = sdbusplus::xyz::openbmc_project::Sensor::server::Value::Unit;

/**
 * Define simple name for the tuple used to create D-Bus associations.
 */
using AssocationTuple = std::tuple<std::string, std::string, std::string>;

/**
 * Root object path for sensors.
 */
constexpr const char* sensorsObjectPath = "/xyz/openbmc_project/sensors";

/**
 * @class DBusSensor
 *
 * This class represents a voltage regulator sensor on D-Bus.
 *
 * Each voltage rail in the system may provide multiple types of sensor data,
 * such as temperature, output voltage, and output current.  A DBusSensor tracks
 * one of these data types for a voltage rail.
 */
class DBusSensor
{
  public:
    // Specify which compiler-generated methods we want
    DBusSensor() = delete;
    DBusSensor(const DBusSensor&) = delete;
    DBusSensor(DBusSensor&&) = delete;
    DBusSensor& operator=(const DBusSensor&) = delete;
    DBusSensor& operator=(DBusSensor&&) = delete;
    virtual ~DBusSensor() = default;

    /**
     * Constructor.
     *
     * Throws an exception if an error occurs.
     *
     * @param bus D-Bus bus object
     * @param name sensor name
     * @param type sensor type
     * @param value sensor value
     * @param rail voltage rail associated with this sensor
     * @param deviceInventoryPath D-Bus inventory path of the voltage regulator
     *                            device that produces the rail
     * @param chassisInventoryPath D-Bus inventory path of the chassis that
     *                             contains the voltage regulator device
     */
    explicit DBusSensor(sdbusplus::bus_t& bus, const std::string& name,
                        SensorType type, double value, const std::string& rail,
                        const std::string& deviceInventoryPath,
                        const std::string& chassisInventoryPath);

    /**
     * Disable this sensor.
     *
     * Updates the sensor properties on D-Bus to indicate it is no longer
     * receiving value updates.
     *
     * This method is normally called when the system is being powered off.
     * Sensors are not read when the system is powered off.
     */
    void disable();

    /**
     * Return the last time this sensor was updated.
     *
     * @return last update time
     */
    const std::chrono::system_clock::time_point& getLastUpdateTime() const
    {
        return lastUpdateTime;
    }

    /**
     * Return the sensor name.
     *
     * @return sensor name
     */
    const std::string& getName() const
    {
        return name;
    }

    /**
     * Return the voltage regulator rail associated with this sensor.
     *
     * @return rail
     */
    const std::string& getRail() const
    {
        return rail;
    }

    /**
     * Return the sensor type.
     *
     * @return sensor type
     */
    SensorType getType() const
    {
        return type;
    }

    /**
     * Set this sensor to the error state.
     *
     * Updates the sensor properties on D-Bus to indicate an error occurred and
     * the sensor value could not be read.
     */
    void setToErrorState();

    /**
     * Set the value of this sensor.
     *
     * Do not specify the value NaN.  This special value is used internally to
     * indicate the sensor has been disabled or is in the error state.  Call the
     * disable() or setToErrorState() method instead so that all affected D-Bus
     * interfaces are updated correctly.
     *
     * @param value new sensor value
     */
    void setValue(double value);

  private:
    /**
     * Sensor value update policy.
     *
     * Determines whether a new sensor value should replace the current value on
     * D-Bus.
     */
    enum class ValueUpdatePolicy : unsigned char
    {
        /**
         * Hysteresis value update policy.
         *
         * The sensor value will only be updated if the new value differs from
         * the current value by at least the hysteresis amount.  This avoids
         * constant D-Bus traffic due to insignificant value changes.
         */
        hysteresis,

        /**
         * Highest value update policy.
         *
         * The sensor value will only be updated if the new value is higher than
         * the current value.
         *
         * Some sensors contain the highest value observed by the voltage
         * regulator, such as the highest temperature or highest output voltage.
         * The regulator internally calculates this value since it can poll the
         * value very quickly and can catch transient events.
         *
         * When the sensor is read from the regulator, the regulator will often
         * clear its internal value.  It will begin calculating a new highest
         * value.  For this reason, the D-Bus sensor value is set to the highest
         * value that has been read across all monitoring cycles.
         *
         * The D-Bus sensor value is cleared when the sensor is disabled.  This
         * normally occurs when the system is powered off.  Thus, the D-Bus
         * sensor value is normally the highest value read since the system was
         * powered on.
         */
        highest,

        /**
         * Lowest value update policy.
         *
         * The sensor value will only be updated if the new value is lower than
         * the current value.
         *
         * Some sensors contain the lowest value observed by the voltage
         * regulator, such as the lowest output current or lowest output
         * voltage.  The regulator internally calculates this value since it can
         * poll the value very quickly and can catch transient events.
         *
         * When the sensor is read from the regulator, the regulator will often
         * clear its internal value.  It will begin calculating a new lowest
         * value.  For this reason, the D-Bus sensor value is set to the lowest
         * value that has been read across all monitoring cycles.
         *
         * The D-Bus sensor value is cleared when the sensor is disabled.  This
         * normally occurs when the system is powered off.  Thus, the D-Bus
         * sensor value is normally the lowest value read since the system was
         * powered on.
         */
        lowest
    };

    /**
     * Get the D-Bus associations to create for this sensor.
     *
     * @param deviceInventoryPath D-Bus inventory path of the voltage regulator
     *                            device that produces the rail
     * @param chassisInventoryPath D-Bus inventory path of the chassis that
     *                             contains the voltage regulator device
     */
    std::vector<AssocationTuple> getAssociations(
        const std::string& deviceInventoryPath,
        const std::string& chassisInventoryPath);

    /**
     * Get sensor properties that are based on the sensor type.
     *
     * The properties are returned in output parameters.
     *
     * Also initializes some data members whose value is based on the sensor
     * type.
     *
     * @param objectPath returns the object path of this sensor
     * @param unit returns the D-Bus unit for the sensor value
     * @param minValue returns the minimum sensor value
     * @param maxValue returns the maximum sensor value
     */
    void getTypeBasedProperties(std::string& objectPath, Unit& unit,
                                double& minValue, double& maxValue);

    /**
     * Set the last time this sensor was updated.
     */
    void setLastUpdateTime()
    {
        lastUpdateTime = std::chrono::system_clock::now();
    }

    /**
     * Set the sensor value on D-Bus to NaN.
     */
    void setValueToNaN();

    /**
     * Returns whether to update the sensor value on D-Bus with the specified
     * new value.
     *
     * @param value new sensor value
     * @return true if value should be updated on D-Bus, false otherwise
     */
    bool shouldUpdateValue(double value);

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t& bus [[maybe_unused]];

    /**
     * Sensor name.
     */
    std::string name{};

    /**
     * Sensor type.
     */
    SensorType type;

    /**
     * Voltage regulator rail associated with this sensor.
     */
    std::string rail{};

    /**
     * Sensor value update policy.
     */
    ValueUpdatePolicy updatePolicy{ValueUpdatePolicy::hysteresis};

    /**
     * Hysteresis value.
     *
     * Only used when updatePolicy is hysteresis.
     */
    double hysteresis{0.0};

    /**
     * sdbusplus object_t class that implements all the necessary D-Bus
     * interfaces via templates and multiple inheritance.
     */
    std::unique_ptr<DBusSensorObject> dbusObject{};

    /**
     * Last time this sensor was updated.
     */
    std::chrono::system_clock::time_point lastUpdateTime{};
};

} // namespace phosphor::power::regulators
