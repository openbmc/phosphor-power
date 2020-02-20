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

#include "i2c_compare_bytes_action.hpp"

#include "action_error.hpp"
#include "i2c_interface.hpp"

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool I2CCompareBytesAction::execute(ActionEnvironment& environment)
{
    bool isEqual{true};
    try
    {
        // Read actual device register values.  Use I2C mode where the number of
        // bytes to read is explicitly specified.
        i2c::I2CInterface& interface = getI2CInterface(environment);
        uint8_t size = values.size();
        uint8_t actualValues[UINT8_MAX];
        interface.read(reg, size, actualValues, i2c::I2CInterface::Mode::I2C);

        // Compare actual byte values to expected byte values
        for (unsigned int i = 0; i < values.size(); ++i)
        {
            if ((actualValues[i] & masks[i]) != values[i])
            {
                isEqual = false;
                break;
            }
        }
    }
    catch (const i2c::I2CException& e)
    {
        // Nest I2CException within an ActionError so caller will have both the
        // low level I2C error information and the action information
        std::throw_with_nested(ActionError(*this));
    }
    return isEqual;
}

std::string I2CCompareBytesAction::toString() const
{
    std::ostringstream ss;
    ss << "i2c_compare_bytes: { register: 0x" << std::hex << std::uppercase
       << static_cast<uint16_t>(reg) << ", values: [ ";
    for (unsigned int i = 0; i < values.size(); ++i)
    {
        ss << ((i > 0) ? ", " : "") << "0x" << static_cast<uint16_t>(values[i]);
    }
    ss << " ], masks: [ ";
    for (unsigned int i = 0; i < masks.size(); ++i)
    {
        ss << ((i > 0) ? ", " : "") << "0x" << static_cast<uint16_t>(masks[i]);
    }
    ss << " ] }";
    return ss.str();
}

} // namespace phosphor::power::regulators
