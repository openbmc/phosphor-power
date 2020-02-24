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

#include "i2c_write_byte_action.hpp"

#include "action_error.hpp"
#include "i2c_interface.hpp"

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool I2CWriteByteAction::execute(ActionEnvironment& environment)
{
    try
    {
        i2c::I2CInterface& interface = getI2CInterface(environment);
        uint8_t valueToWrite;
        if (mask == 0xFF)
        {
            valueToWrite = value;
        }
        else
        {
            // Read current value of device register
            uint8_t currentValue{0x00};
            interface.read(reg, currentValue);

            // Combine value to write with current value
            valueToWrite = (value & mask) | (currentValue & (~mask));
        }

        // Write value to device register
        interface.write(reg, valueToWrite);
    }
    catch (const i2c::I2CException& e)
    {
        // Nest I2CException within an ActionError so caller will have both the
        // low level I2C error information and the action information
        std::throw_with_nested(ActionError(*this));
    }
    return true;
}

std::string I2CWriteByteAction::toString() const
{
    std::ostringstream ss;
    ss << "i2c_write_byte: { register: 0x" << std::hex << std::uppercase
       << static_cast<uint16_t>(reg) << ", value: 0x"
       << static_cast<uint16_t>(value) << ", mask: 0x"
       << static_cast<uint16_t>(mask) << " }";
    return ss.str();
}

} // namespace phosphor::power::regulators
