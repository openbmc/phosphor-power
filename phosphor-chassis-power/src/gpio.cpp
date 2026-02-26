/**
 * Copyright © 2026 IBM Corporation
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

#include "gpio.hpp"

#include "chassis.hpp"

#include <exception>
#include <stdexcept>

namespace phosphor::power::chassis
{

gpioDirection parseDirection(const std::string& directionStr)
{
    if (directionStr == "Input")
    {
        return gpioDirection::Input;
    }
    else if (directionStr == "Output")
    {
        return gpioDirection::Output;
    }
    else
    {
        throw std::invalid_argument{"Invalid direction value: " + directionStr};
    }
}

gpioPolarity parsePolarity(const std::string& polarityStr)
{
    if (polarityStr == "Low")
    {
        return gpioPolarity::Low;
    }
    else if (polarityStr == "High")
    {
        return gpioPolarity::High;
    }
    else
    {
        throw std::invalid_argument{"Invalid polarity value: " + polarityStr};
    }
}

} // namespace phosphor::power::chassis
