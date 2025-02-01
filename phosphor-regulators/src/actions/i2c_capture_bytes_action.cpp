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

#include "i2c_capture_bytes_action.hpp"

#include "action_error.hpp"
#include "i2c_interface.hpp"

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool I2CCaptureBytesAction::execute(ActionEnvironment& environment)
{
    try
    {
        // Read device register values.  Use I2C mode where the number of bytes
        // to read is explicitly specified.
        i2c::I2CInterface& interface = getI2CInterface(environment);
        uint8_t size{count}; // byte count parameter is input/output
        uint8_t values[UINT8_MAX];
        interface.read(reg, size, values, i2c::I2CInterface::Mode::I2C);

        // Store error data in action environment as a string key/value pair
        std::string key = getErrorDataKey(environment);
        std::string value = getErrorDataValue(values);
        environment.addAdditionalErrorData(key, value);
    }
    catch (const i2c::I2CException& e)
    {
        // Nest I2CException within an ActionError so caller will have both the
        // low level I2C error information and the action information
        std::throw_with_nested(ActionError(*this));
    }
    return true;
}

std::string I2CCaptureBytesAction::toString() const
{
    std::ostringstream ss;
    ss << "i2c_capture_bytes: { register: 0x" << std::hex << std::uppercase
       << static_cast<uint16_t>(reg) << ", count: " << std::dec
       << static_cast<uint16_t>(count) << " }";
    return ss.str();
}

std::string I2CCaptureBytesAction::getErrorDataKey(
    ActionEnvironment& environment) const
{
    // Additional error data key format: <deviceID>_register_<register>
    std::ostringstream ss;
    ss << environment.getDeviceID() << "_register_0x" << std::hex
       << std::uppercase << static_cast<uint16_t>(reg);
    std::string key = ss.str();

    // Verify key does not already exist in the action environment.  This occurs
    // when the same device and register is captured multiple times.
    if (environment.getAdditionalErrorData().contains(key))
    {
        // Add counter suffix to key and loop until unused key is found
        int counter = 2;
        std::string keyWithSuffix;
        do
        {
            keyWithSuffix = key + '_' + std::to_string(counter);
            if (!environment.getAdditionalErrorData().contains(keyWithSuffix))
            {
                key = keyWithSuffix;
                break;
            }
            ++counter;
        } while (true);
    }

    return key;
}

std::string I2CCaptureBytesAction::getErrorDataValue(
    const uint8_t* values) const
{
    // Additional error data value format: [ <byte 0>, <byte 1>, ... ]
    std::ostringstream ss;
    ss << "[ " << std::hex << std::uppercase;
    for (unsigned int i = 0; i < count; ++i)
    {
        ss << ((i > 0) ? ", " : "") << "0x" << static_cast<uint16_t>(values[i]);
    }
    ss << " ]";
    return ss.str();
}

} // namespace phosphor::power::regulators
