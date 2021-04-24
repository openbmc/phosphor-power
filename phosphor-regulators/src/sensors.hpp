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

#include "services.hpp"

#include <string>

namespace phosphor::power::regulators
{

/**
 * Voltage regulator sensor type.
 */
enum class SensorType : unsigned char
{
    /**
     * Output current.
     */
    iout,

    /**
     * Highest output current.
     */
    iout_peak,

    /**
     * Lowest output current.
     */
    iout_valley,

    /**
     * Output power.
     */
    pout,

    /**
     * Temperature.
     */
    temperature,

    /**
     * Highest temperature.
     */
    temperature_peak,

    /**
     * Output voltage.
     */
    vout,

    /**
     * Highest output voltage.
     */
    vout_peak,

    /**
     * Lowest output voltage.
     */
    vout_valley
};

/**
 * @namespace sensors
 *
 * Contains utility functions related to voltage regulator sensors.
 */
namespace sensors
{

/**
 * Returns the name of the specified SensorType.
 *
 * The returned string will exactly match the enumerator name, such as
 * "temperature_peak".
 *
 * @param type sensor type
 * @return sensor type name
 */
inline std::string toString(SensorType type)
{
    std::string name{};
    switch (type)
    {
        case SensorType::iout:
            name = "iout";
            break;
        case SensorType::iout_peak:
            name = "iout_peak";
            break;
        case SensorType::iout_valley:
            name = "iout_valley";
            break;
        case SensorType::pout:
            name = "pout";
            break;
        case SensorType::temperature:
            name = "temperature";
            break;
        case SensorType::temperature_peak:
            name = "temperature_peak";
            break;
        case SensorType::vout:
            name = "vout";
            break;
        case SensorType::vout_peak:
            name = "vout_peak";
            break;
        case SensorType::vout_valley:
            name = "vout_valley";
            break;
    }
    return name;
}

} // namespace sensors

/**
 * @class Sensors
 *
 * Abstract base class for a service that maintains a list of voltage regulator
 * sensors.
 *
 * This service makes the voltage regulator sensors available to other BMC
 * applications.  For example, the Redfish support obtains sensor data from this
 * service.
 *
 * Each voltage rail in the system may provide multiple types of sensor data,
 * such as temperature, output voltage, and output current (see SensorType).  A
 * sensor tracks one of these data types for a voltage rail.
 *
 * Voltage regulator sensors are typically read frequently based on a timer.
 * Reading all the sensors once is called a monitoring cycle.  The application
 * will loop through all voltage rails, reading all supported sensor types for
 * each rail.  During a monitoring cycle, the following sensor service methods
 * should be called in the specified order:
 * - startCycle() // At the start of a sensor monitoring cycle
 * - startRail()  // Before reading all the sensors for one rail
 * - setValue()   // To set the value of one sensor for the current rail
 * - endRail()    // After reading all the sensors for one rail
 * - endCycle()   // At the end of a sensor monitoring cycle
 *
 * This service can be enabled or disabled.  It is typically enabled when the
 * system is powered on and voltage regulators begin producing output.  It is
 * typically disabled when the system is powered off.  It can also be
 * temporarily disabled if other BMC applications need to communicate with the
 * voltage regulator devices.  When the service is disabled, the sensors still
 * exist but are in an inactive state since their values are not being updated.
 */
class Sensors
{
  public:
    // Specify which compiler-generated methods we want
    Sensors() = default;
    Sensors(const Sensors&) = delete;
    Sensors(Sensors&&) = delete;
    Sensors& operator=(const Sensors&) = delete;
    Sensors& operator=(Sensors&&) = delete;
    virtual ~Sensors() = default;

    /**
     * Enable the sensors service.
     *
     * While the service is enabled, the sensors that it provides will be in an
     * active state.  This indicates that their value is being updated
     * periodically.
     *
     * @param services system services
     */
    virtual void enable(Services& services) = 0;

    /**
     * Notify the sensors service that the current sensor monitoring cycle has
     * ended.
     *
     * @param services system services
     */
    virtual void endCycle(Services& services) = 0;

    /**
     * Notify the sensors service that sensor monitoring has ended for the
     * current voltage rail.
     *
     * @param errorOccurred specifies whether an error occurred while trying to
     *                      read all the sensors for the current rail
     * @param services system services
     */
    virtual void endRail(bool errorOccurred, Services& services) = 0;

    /**
     * Disable the sensors service.
     *
     * While the service is disabled, the sensors that it provides will be in an
     * inactive state.  This indicates that their value is not being updated.
     *
     * @param services system services
     */
    virtual void disable(Services& services) = 0;

    /**
     * Sets the value of one sensor for the current voltage rail.
     *
     * Throws an exception if an error occurs.
     *
     * @param type sensor type
     * @param value sensor value
     * @param services system services
     */
    virtual void setValue(SensorType type, double value,
                          Services& services) = 0;

    /**
     * Notify the sensors service that a sensor monitoring cycle is starting.
     *
     * @param services system services
     */
    virtual void startCycle(Services& services) = 0;

    /**
     * Notify the sensors service that sensor monitoring is starting for the
     * specified voltage rail.
     *
     * Calls to setValue() will update sensors for this rail.
     *
     * @param rail unique rail ID
     * @param deviceInventoryPath D-Bus inventory path of the voltage regulator
     *                            device that produces the rail
     * @param chassisInventoryPath D-Bus inventory path of the chassis that
     *                             contains the voltage regulator device
     * @param services system services
     */
    virtual void startRail(const std::string& rail,
                           const std::string& deviceInventoryPath,
                           const std::string& chassisInventoryPath,
                           Services& services) = 0;
};

} // namespace phosphor::power::regulators
