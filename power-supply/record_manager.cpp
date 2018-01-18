/**
 * Copyright © 2017 IBM Corporation
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
#include <math.h>
#include "record_manager.hpp"

namespace witherspoon
{
namespace power
{
namespace history
{

int64_t RecordManager::linearToInteger(uint16_t data)
{
    //The exponent is the first 5 bits, followed by 11 bits of mantissa.
    int8_t exponent = (data & 0xF800) >> 11;
    int16_t mantissa = (data & 0x07FF);

    //If exponent's MSB on, then it's negative.
    //Convert from two's complement.
    if (exponent & 0x10)
    {
        exponent = (~exponent) & 0x1F;
        exponent = (exponent + 1) * -1;
    }

    //If mantissa's MSB on, then it's negative.
    //Convert from two's complement.
    if (mantissa & 0x400)
    {
        mantissa = (~mantissa) & 0x07FF;
        mantissa = (mantissa + 1) * -1;
    }

    auto value = static_cast<float>(mantissa) * pow(2, exponent);
    return value;
}

}
}
}
