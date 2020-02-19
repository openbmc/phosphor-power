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

#include "i2c_compare_bit_action.hpp"

#include "action_error.hpp"
#include "i2c_interface.hpp"

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool I2CCompareBitAction::execute(ActionEnvironment& environment)
{
    bool isEqual{false};
    try
    {
        // Read actual value of device register
        uint8_t registerValue{0x00};
        i2c::I2CInterface& interface = getI2CInterface(environment);
        interface.read(reg, registerValue);

        // Get actual bit value
        uint8_t actualValue = (registerValue >> position) & 0x01;

        // Check if actual bit value equals expected bit value
        isEqual = (actualValue == value);
    }
    catch (const i2c::I2CException& e)
    {
        // Nest I2CException within an ActionError so caller will have both the
        // low level I2C error information and the action information
        std::throw_with_nested(ActionError(*this));
    }
    return isEqual;
}

std::string I2CCompareBitAction::toString() const
{
    std::ostringstream ss;
    ss << "i2c_compare_bit: { register: 0x" << std::hex << std::uppercase
       << static_cast<uint16_t>(reg) << ", position: " << std::dec
       << static_cast<uint16_t>(position)
       << ", value: " << static_cast<uint16_t>(value) << " }";
    return ss.str();
}

} // namespace phosphor::power::regulators
