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

#include <phosphor-logging/lg2.hpp>

#include <exception>
#include <stdexcept>

namespace phosphor::power::chassis
{

Gpio::Gpio(const std::string& name, GpioDirection direction,
           GpioPolarity polarity, bool validateHardware) :
    name{name}, direction{direction}, polarity{polarity},
    requestFlags{(polarity == GpioPolarity::Low)
                     ? gpiod::line_request::FLAG_ACTIVE_LOW
                     : 0},
    deglitchedValue{(polarity == GpioPolarity::Low) ? 1 : 0}
{
    if (validateHardware)
    {
        line = gpiod::find_line(name);
        if (!line)
        {
            throw std::invalid_argument{"Invalid GPIO name: " + name};
        }
    }
}

Gpio::~Gpio()
{
    // Destructors must not throw exceptions
    try
    {
        if (line.is_requested())
        {
            release();
        }
    }
    catch (...)
    {}
}

void Gpio::requestLine()
{
    // Only request line if we dont have it.
    if (!line.is_requested())
    {
        try
        {
            if (direction == GpioDirection::Input)
            {
                line.request({consumer, gpiod::line_request::DIRECTION_INPUT,
                              requestFlags});
            }
            else
            {
                line.request({consumer, gpiod::line_request::DIRECTION_OUTPUT,
                              requestFlags});
            }
            // Successful, reset failure count
            requestFailureCount = 0;
        }
        catch (const std::exception& e)
        {
            handleRequestFail();
        }
    }
}

void Gpio::requestLine(int initialValue)
{
    // Only request line if we dont have it.
    if (!line.is_requested())
    {
        if (direction == GpioDirection::Output)
        {
            try
            {
                line.request({consumer, gpiod::line_request::DIRECTION_OUTPUT,
                              requestFlags},
                             initialValue);
                // Successful, reset failure count
                requestFailureCount = 0;
            }
            catch (const std::exception& e)
            {
                handleRequestFail();
            }
        }
        else
        {
            throw std::logic_error(
                "Cannot set value on input GPIO '" + name + "'");
        }
    }
}

int Gpio::getDeglitchedValue()
{
    // Read gpio value
    int gpioValue = getValue();

    // Deglitch the gpios, need 2 readings in a row to change.
    deglitchedValue = (gpioValue == lastReading) ? gpioValue : deglitchedValue;

    // Store the last gpio value.
    lastReading = gpioValue;

    return deglitchedValue.value();
}

int Gpio::getValue()
{
    // Read gpio value
    return line.get_value();
}

void Gpio::setValue(int value)
{
    line.set_value(value);
}

void Gpio::release()
{
    line.release();
}

void Gpio::handleRequestFail()
{
    requestFailureCount++;
    if (requestFailureCount == requestFailureThreshold)
    {
        lg2::error("Requested line failed for GPIO {NAME}, {COUNT} times",
                   "NAME", name, "COUNT", requestFailureThreshold);
    }
}
} // namespace phosphor::power::chassis
