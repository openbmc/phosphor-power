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

#include <chrono>
#include <exception>
#include <stdexcept>
#include <thread>

namespace phosphor::power::chassis
{

Gpio::Gpio(const std::string& name, GpioDirection direction,
           GpioPolarity polarity, std::optional<uint8_t> defaultValue) :
    name{name}, direction{direction}, polarity{polarity},
    defaultValue{defaultValue},
    requestFlags{(polarity == GpioPolarity::Low)
                     ? gpiod::line_request::FLAG_ACTIVE_LOW
                     : 0},
    gpioValue{defaultValue}
{}

Gpio::~Gpio()
{
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

bool Gpio::findLine()
{
    line = gpiod::find_line(name);
    if (!line)
    {
        if (!handleLineNotFound())
        {
            lg2::error("Invalid GPIO name: {NAME}", "NAME", name);
        }
    }
    else
    {
        lineFound = true;
        lineNotFoundCount = 0;
    }

    return lineFound;
}

bool Gpio::requestRead()
{
    // Only request line if we dont have it.
    if (!line.is_requested())
    {
        // Only request a read if gpio is input.
        if (direction == GpioDirection::Input)
        {
            try
            {
                line.request({consumer, gpiod::line_request::DIRECTION_INPUT,
                              requestFlags});
                // Successful, reset failure count
                requestFailureCount = 0;
                return true;
            }
            catch (const std::exception& e)
            {
                if (!handleRequestFail())
                {
                    lg2::error(
                        "Failed to request GPIO line '{NAME}' after {ATTEMPTS} attempts",
                        "NAME", name, "ATTEMPTS", failureThreshold);
                }
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    // Line already requested
    return true;
}

bool Gpio::requestWrite(int initialValue)
{
    // Only request line if we dont have it.
    if (!line.is_requested())
    {
        // Only request a write if gpio is output.
        if (direction == GpioDirection::Output)
        {
            try
            {
                line.request({consumer, gpiod::line_request::DIRECTION_OUTPUT,
                              requestFlags},
                             initialValue);
                // Successful, reset failure count
                requestFailureCount = 0;
                return true;
            }
            catch (const std::exception& e)
            {
                if (!handleRequestFail())
                {
                    lg2::error(
                        "Failed to request GPIO line '{NAME}' after {ATTEMPTS} attempts",
                        "NAME", name, "ATTEMPTS", failureThreshold);
                }
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    // Line already requested
    return true;
}

int Gpio::getValue()
{
    if (gpioValue.has_value())
    {
        previousValue = gpioValue;
    }

    gpioValue.value() = line.get_value();

    return gpioValue.value();
}
int Gpio::getPreviousValue()
{
    if (previousValue.has_value())
    {
        return previousValue.value();
    }
    throw std::logic_error("Cannot get previous value on GPIO '" + name +
                           "' before getValue() has been called at least once");
}
void Gpio::setValue(int value)
{
    line.set_value(value);
}

void Gpio::release()
{
    line.release();
}

void Gpio::clearErrorHistory()
{
    requestFailureCount = 0;
    lineNotFoundCount = 0;
}

bool Gpio::handleRequestFail()
{
    // Only increment if we haven't reached the threshold.
    if (requestFailureCount < failureThreshold)
    {
        requestFailureCount++;
    }

    if (requestFailureCount == failureThreshold)
    {
        return false;
    }

    return true;
}

bool Gpio::handleLineNotFound()
{
    // Only increment if we haven't reached the threshold.
    if (lineNotFoundCount < failureThreshold)
    {
        lineNotFoundCount++;
    }

    if (lineNotFoundCount == failureThreshold)
    {
        return false;
    }

    return true;
}

} // namespace phosphor::power::chassis
