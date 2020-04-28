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

#include <cstdint>
#include <optional>
#include <stdexcept>
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
     * @param type Sensor value type.
     * @param command PMBus command code.
     * @param format Data format of the sensor value returned by the device.
     * @param exponent Exponent value for linear_16 data format.
     *                 Can be positive or negative. If not specified, the
     *                 exponent value will be read from VOUT_MODE.
     *                 Should not be specified if the data format is linear_11.
     */
    explicit PMBusReadSensorAction(pmbus_utils::SensorValueType type,
                                   uint8_t command,
                                   pmbus_utils::SensorDataFormat format,
                                   std::optional<int8_t> exponent) :
        type{type},
        command{command}, format{format}, exponent{exponent}
    {
    }

    /**
     * Executes this action.
     *
     * TODO: Not implemented yet
     *
     * @param environment Action execution environment.
     * @return true
     */
    virtual bool execute(ActionEnvironment& /* environment */) override
    {
        // TODO: Not implemented yet
        return true;
    }

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
     * Returns the sensor value type.
     *
     * @return sensor value type.
     */
    pmbus_utils::SensorValueType getType() const
    {
        return type;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override
    {
        std::ostringstream ss;
        ss << "pmbus_read_sensor: { ";
        ss << "type: " << pmbus_utils::toString(type) << ", " << std::hex
           << std::uppercase;
        ss << "command: 0x" << static_cast<uint16_t>(command) << ", "
           << std::dec << std::nouppercase;
        ss << "format: " << pmbus_utils::toString(format);

        if (exponent.has_value())
        {
            ss << ", exponent: " << static_cast<int16_t>(exponent.value());
        }

        ss << " }";

        return ss.str();
    }

  private:
    /**
     * Sensor value type.
     */
    const pmbus_utils::SensorValueType type{};

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
