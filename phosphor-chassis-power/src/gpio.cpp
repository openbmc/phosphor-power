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

BMCGpio::BMCGpio(const std::string& name, GpioDirection direction,
                 GpioPolarity polarity, std::optional<uint8_t> defaultValue) :
    name{name}, direction{direction}, polarity{polarity},
    defaultValue{defaultValue},
    requestFlags{(polarity == GpioPolarity::Low)
                     ? gpiod::line_request::FLAG_ACTIVE_LOW
                     : 0}
{}

BMCGpio::~BMCGpio()
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

bool BMCGpio::findLine()
{
    try
    {
        line = gpiod::find_line(name);
        if (!line)
        {
            if (!shouldSuppressError(lineNotFoundCount))
            {
                lg2::error("Invalid GPIO name: {NAME}", "NAME", name);
            }
        }
        else
        {
            lineFound = true;
            lineNotFoundCount = 0;
        }
    }
    catch (const std::exception& e)
    {
        if (!shouldSuppressError(lineNotFoundCount))
        {
            lg2::error("Failed to find GPIO line '{NAME}': {ERROR}", "NAME",
                       name, "ERROR", e.what());
        }
    }

    return lineFound;
}

bool BMCGpio::requestRead()
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
                if (!shouldSuppressError(requestFailureCount))
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

bool BMCGpio::requestWrite(int initialValue)
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
                if (!shouldSuppressError(requestFailureCount))
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

int BMCGpio::getValue()
{
    if (gpioValue.has_value())
    {
        previousValue = gpioValue;
    }

    try
    {
        gpioValue = line.get_value();
        // Successful read, reset failure count
        readWriteFailureCount = 0;
    }
    catch (const std::exception& e)
    {
        // If this is the first read attempt and we have a default value, use
        // it.
        if (firstRead && defaultValue.has_value())
        {
            gpioValue = defaultValue.value();

            lg2::error(
                "Failed to read GPIO line '{NAME}', using default value: {ERROR}",
                "NAME", name, "ERROR", e.what());
        }
        else
        {
            if (!shouldSuppressError(readWriteFailureCount))
            {
                lg2::error(
                    "Failed to read GPIO line '{NAME}' after {ATTEMPTS} attempts: {ERROR}",
                    "NAME", name, "ATTEMPTS", failureThreshold, "ERROR",
                    e.what());
            }

            firstRead = false;
            throw;
        }
    }

    firstRead = false;
    return gpioValue.value();
}

int BMCGpio::getPreviousValue()
{
    if (previousValue.has_value())
    {
        return previousValue.value();
    }
    throw std::logic_error(
        "Cannot get previous value on GPIO '" + name +
        "' before getValue() has been succesfully called at least once");
}

void BMCGpio::setValue(int value)
{
    try
    {
        line.set_value(value);
        // Successful write, reset failure count
        readWriteFailureCount = 0;
    }
    catch (const std::exception& e)
    {
        if (!shouldSuppressError(readWriteFailureCount))
        {
            lg2::error(
                "Failed to write GPIO line '{NAME}' after {ATTEMPTS} attempts: {ERROR}",
                "NAME", name, "ATTEMPTS", failureThreshold, "ERROR", e.what());
        }
        throw;
    }
}

void BMCGpio::release()
{
    line.release();
}

void BMCGpio::clearErrorHistory()
{
    requestFailureCount = 0;
    lineNotFoundCount = 0;
    readWriteFailureCount = 0;
}

bool BMCGpio::shouldSuppressError(int& counter)
{
    if (counter <= failureThreshold)
    {
        ++counter;
    }

    if (counter == failureThreshold)
    {
        return false;
    }

    return true;
}

} // namespace phosphor::power::chassis
