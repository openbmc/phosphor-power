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

#include "pmbus_utils.hpp"

namespace phosphor::power::regulators::pmbus_utils
{

void parseVoutMode(uint8_t voutModeValue, VoutDataFormat& format,
                   int8_t& parameter)
{
    // Get the mode field from bits [6:5] in the VOUT_MODE value
    uint8_t modeField = (voutModeValue & 0b0110'0000u) >> 5;

    // Get data format from mode field
    switch (modeField)
    {
        case 0b00u:
            format = VoutDataFormat::linear;
            break;
        case 0b01u:
            format = VoutDataFormat::vid;
            break;
        case 0b10u:
            format = VoutDataFormat::direct;
            break;
        case 0b11u:
            format = VoutDataFormat::ieee;
            break;
    }

    // Get the parameter field from bits [4:0] in the VOUT_MODE value
    uint8_t parameterField = voutModeValue & 0b0001'1111u;

    // Get parameter value from parameter field
    if (format == VoutDataFormat::linear)
    {
        // Extend sign bit if necessary because parameter is an exponent in
        // two's complement format
        if (parameterField & 0b0001'0000u)
        {
            parameterField |= 0b1110'0000u;
        }
    }
    parameter = static_cast<int8_t>(parameterField);
}

} // namespace phosphor::power::regulators::pmbus_utils
