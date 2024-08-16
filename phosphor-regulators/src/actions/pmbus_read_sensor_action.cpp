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

#include "pmbus_read_sensor_action.hpp"

#include "action_error.hpp"
#include "pmbus_error.hpp"

#include <sdbusplus/exception.hpp>

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool PMBusReadSensorAction::execute(ActionEnvironment& environment)
{
    try
    {
        // Get I2C interface to current device
        i2c::I2CInterface& interface = getI2CInterface(environment);

        // Read two byte value of PMBus command code.  I2CInterface method reads
        // low byte first as required by PMBus.
        uint16_t value{0x00};
        interface.read(command, value);

        // Convert two byte PMBus value into a decimal sensor value
        double sensorValue{0.0};
        switch (format)
        {
            case pmbus_utils::SensorDataFormat::linear_11:
                sensorValue = pmbus_utils::convertFromLinear(value);
                break;
            case pmbus_utils::SensorDataFormat::linear_16:
                int8_t exponentValue = getExponentValue(environment, interface);
                sensorValue =
                    pmbus_utils::convertFromVoutLinear(value, exponentValue);
                break;
        }

        // Publish sensor value using the Sensors service
        environment.getServices().getSensors().setValue(type, sensorValue);
    }
    // Nest the following exception types within an ActionError so the caller
    // will have both the low level error information and the action information
    catch (const i2c::I2CException& e)
    {
        std::throw_with_nested(ActionError(*this));
    }
    catch (const PMBusError& e)
    {
        std::throw_with_nested(ActionError(*this));
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::throw_with_nested(ActionError(*this));
    }
    return true;
}

std::string PMBusReadSensorAction::toString() const
{
    std::ostringstream ss;
    ss << "pmbus_read_sensor: { ";
    ss << "type: " << sensors::toString(type) << ", " << std::hex
       << std::uppercase;
    ss << "command: 0x" << static_cast<uint16_t>(command) << ", " << std::dec
       << std::nouppercase;
    ss << "format: " << pmbus_utils::toString(format);

    if (exponent.has_value())
    {
        ss << ", exponent: " << static_cast<int16_t>(exponent.value());
    }

    ss << " }";

    return ss.str();
}

int8_t PMBusReadSensorAction::getExponentValue(ActionEnvironment& environment,
                                               i2c::I2CInterface& interface)
{
    // Check if an exponent value is defined for this action
    if (exponent.has_value())
    {
        return exponent.value();
    }

    // Read value of the VOUT_MODE command
    uint8_t voutModeValue;
    interface.read(pmbus_utils::VOUT_MODE, voutModeValue);

    // Parse VOUT_MODE value to get data format and parameter value
    pmbus_utils::VoutDataFormat format;
    int8_t parameter;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);

    // Verify format is linear; other formats not currently supported
    if (format != pmbus_utils::VoutDataFormat::linear)
    {
        throw PMBusError("VOUT_MODE contains unsupported data format",
                         environment.getDeviceID(),
                         environment.getDevice().getFRU());
    }

    // Return parameter value; it contains the exponent when format is linear
    return parameter;
}

} // namespace phosphor::power::regulators
