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

#include "pmbus_write_vout_command_action.hpp"

#include "action_error.hpp"
#include "pmbus_error.hpp"
#include "write_verification_error.hpp"

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool PMBusWriteVoutCommandAction::execute(ActionEnvironment& environment)
{
    try
    {
        // Get volts value
        double voltsValue = getVoltsValue(environment);

        // Get I2C interface to current device
        i2c::I2CInterface& interface = getI2CInterface(environment);

        // Get exponent value for converting volts value to linear format
        int8_t exponentValue = getExponentValue(environment, interface);

        // Convert volts value to linear data format
        uint16_t linearValue =
            pmbus_utils::convertToVoutLinear(voltsValue, exponentValue);

        // Write linear format value to VOUT_COMMAND.  I2CInterface method
        // writes low-order byte first as required by PMBus.
        interface.write(pmbus_utils::VOUT_COMMAND, linearValue);

        // Verify write if necessary
        if (isWriteVerified)
        {
            verifyWrite(environment, interface, linearValue);
        }
    }
    // Nest the following exception types within an ActionError so the caller
    // will have both the low level error information and the action information
    catch (const i2c::I2CException& i2cError)
    {
        std::throw_with_nested(ActionError(*this));
    }
    catch (const PMBusError& pmbusError)
    {
        std::throw_with_nested(ActionError(*this));
    }
    catch (const WriteVerificationError& verifyError)
    {
        std::throw_with_nested(ActionError(*this));
    }
    return true;
}

std::string PMBusWriteVoutCommandAction::toString() const
{
    std::ostringstream ss;
    ss << "pmbus_write_vout_command: { ";

    if (volts.has_value())
    {
        ss << "volts: " << volts.value() << ", ";
    }

    ss << "format: " << pmbus_utils::toString(format);

    if (exponent.has_value())
    {
        ss << ", exponent: " << static_cast<int16_t>(exponent.value());
    }

    ss << ", is_verified: " << std::boolalpha << isWriteVerified << " }";
    return ss.str();
}

int8_t PMBusWriteVoutCommandAction::getExponentValue(
    ActionEnvironment& environment, i2c::I2CInterface& interface)
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

double
    PMBusWriteVoutCommandAction::getVoltsValue(ActionEnvironment& environment)
{
    double voltsValue;

    if (volts.has_value())
    {
        // A volts value is defined for this action
        voltsValue = volts.value();
    }
    else if (environment.getVolts().has_value())
    {
        // A volts value is defined in the ActionEnvironment
        voltsValue = environment.getVolts().value();
    }
    else
    {
        throw ActionError(*this, "No volts value defined");
    }

    return voltsValue;
}

void PMBusWriteVoutCommandAction::verifyWrite(ActionEnvironment& environment,
                                              i2c::I2CInterface& interface,
                                              uint16_t valueWritten)
{
    // Read current value of VOUT_COMMAND.  I2CInterface method reads low byte
    // first as required by PMBus.
    uint16_t valueRead{0x00};
    interface.read(pmbus_utils::VOUT_COMMAND, valueRead);

    // Verify value read equals value written
    if (valueRead != valueWritten)
    {
        std::ostringstream ss;
        ss << "device: " << environment.getDeviceID()
           << ", register: VOUT_COMMAND, value_written: 0x" << std::hex
           << std::uppercase << valueWritten << ", value_read: 0x" << valueRead;
        throw WriteVerificationError(ss.str(), environment.getDeviceID(),
                                     environment.getDevice().getFRU());
    }
}

} // namespace phosphor::power::regulators
