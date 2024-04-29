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

#include "action_environment.hpp"
#include "i2c_action.hpp"
#include "i2c_interface.hpp"
#include "pmbus_utils.hpp"
#include "sensors.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class PMBusReadSensorAction
 *
 * Reads one sensor for a PMBus regulator rail. Communicates with the device
 * directly using the I2C interface.
 *
 * Implements the pmbus_read_sensor action in the JSON config file.
 *
 * Currently supports the linear_11 and linear_16 sensor data formats.
 *
 * The linear_16 data format requires an exponent value.  The exponent value
 * can be specified in the constructor.  Otherwise the exponent value will be
 * obtained from the PMBus VOUT_MODE command.  Note that some PMBus devices do
 * not support the VOUT_MODE command.  The exponent value for a device is often
 * found in the device documentation (data sheet).
 */
class PMBusReadSensorAction : public I2CAction
{
  public:
    // Specify which compiler-generated methods we want
    PMBusReadSensorAction() = delete;
    PMBusReadSensorAction(const PMBusReadSensorAction&) = delete;
    PMBusReadSensorAction(PMBusReadSensorAction&&) = delete;
    PMBusReadSensorAction& operator=(const PMBusReadSensorAction&) = delete;
    PMBusReadSensorAction& operator=(PMBusReadSensorAction&&) = delete;
    virtual ~PMBusReadSensorAction() = default;

    /**
     * Constructor.
     *
     * @param type Sensor type.
     * @param command PMBus command code.
     * @param format Data format of the sensor value returned by the device.
     * @param exponent Exponent value for linear_16 data format.
     *                 Can be positive or negative. If not specified, the
     *                 exponent value will be read from VOUT_MODE.
     *                 Should not be specified if the data format is linear_11.
     */
    explicit PMBusReadSensorAction(SensorType type, uint8_t command,
                                   pmbus_utils::SensorDataFormat format,
                                   std::optional<int8_t> exponent) :
        type{type}, command{command}, format{format}, exponent{exponent}
    {}

    /**
     * Executes this action.
     *
     * Reads one sensor using the I2C interface.
     *
     * The sensor type is specified in the constructor.
     *
     * The PMBus command code is specified in the constructor.
     * It is the register to read on the device.
     *
     * The sensor data format is specified in the constructor. Currently
     * the linear_11 and linear_16 formats are supported.
     *
     * The linear_16 data format requires an exponent value.
     * If an exponent value was specified in the constructor, that
     * value will be used.  Otherwise the exponent value will be obtained from
     * the VOUT_MODE command.
     *
     * The device is obtained from the ActionEnvironment.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment Action execution environment.
     * @return true
     */
    virtual bool execute(ActionEnvironment& environment) override;

    /**
     * Returns the PMBus command code.
     *
     * @return command
     */
    uint8_t getCommand() const
    {
        return command;
    }

    /**
     * Returns the optional exponent value for linear_16 data format.
     *
     * @return optional exponent value
     */
    std::optional<int8_t> getExponent() const
    {
        return exponent;
    }

    /**
     * Returns the data format of the sensor value returned by the device.
     *
     * @return data format
     */
    pmbus_utils::SensorDataFormat getFormat() const
    {
        return format;
    }

    /**
     * Returns the sensor type.
     *
     * @return sensor type.
     */
    SensorType getType() const
    {
        return type;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override;

  private:
    /**
     * Gets the exponent value to use to convert a linear_16 format value to a
     * decimal volts value.
     *
     * If an exponent value is defined for this action, that value is returned.
     * Otherwise VOUT_MODE is read from the current device to obtain the
     * exponent value.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @param interface I2C interface to the current device
     * @return exponent value
     */
    int8_t getExponentValue(ActionEnvironment& environment,
                            i2c::I2CInterface& interface);

    /**
     * Sensor type.
     */
    const SensorType type{};

    /**
     * PMBus command code.
     */
    const uint8_t command{};

    /**
     * Data format of the sensor value returned by the device.
     */
    const pmbus_utils::SensorDataFormat format{};

    /**
     * Optional exponent value for linear_16 data format.
     */
    const std::optional<int8_t> exponent{};
};

} // namespace phosphor::power::regulators
