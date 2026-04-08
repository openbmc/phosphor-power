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
           GpioPolarity polarity) :
    name{name}, direction{direction}, polarity{polarity},
    requestFlags{(polarity == GpioPolarity::Low)
                     ? gpiod::line_request::FLAG_ACTIVE_LOW
                     : 0}
{
    /*
    line = gpiod::find_line(name);
    if (!line)
    {
        throw std::invalid_argument{"Invalid GPIO name: " + name};
    }
    */
}

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

bool Gpio::requestRead()
{
    // Only request line if we dont have it.
    if (!line.is_requested())
    {
        try
        {
            line.request(
                {consumer, gpiod::line_request::DIRECTION_INPUT, requestFlags});
            // Successful, reset failure count
            requestFailureCount = 0;

            // Setup deglitching
            if (!initializedRead)
            {
                getDeglitchedValue();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                getDeglitchedValue();
                initializedRead = true;
            }
            return true;
        }
        catch (const std::exception& e)
        {
            if (!handleRequestFail())
            {
                throw std::runtime_error(
                    "Failed to request GPIO line '" + name + "' after " +
                    std::to_string(requestFailureThreshold) + " attempts");
            }
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
                    throw std::runtime_error(
                        "Failed to request GPIO line '" + name + "' after " +
                        std::to_string(requestFailureThreshold) + " attempts");
                }
                return false;
            }
        }
        else
        {
            throw std::logic_error(
                "Cannot set value on input GPIO '" + name + "'");
        }
    }
    // Line already requested
    return true;
}

std::optional<int> Gpio::getDeglitchedValue()
{
    // Read gpio value
    int gpioValue = getValue();

    // Only Deglitch if there is a previous reading.
    if (lastReading.has_value())
    {
        // Deglitch the gpios, need 2 readings in a row to change.
        deglitchedValue =
            (gpioValue == lastReading) ? gpioValue : deglitchedValue;
        // Store the last gpio value.
        lastReading = gpioValue;
        return deglitchedValue;
    }
    else
    {
        // Set the last reading and return null.
        lastReading = gpioValue;
        return deglitchedValue;
    }
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

bool Gpio::handleRequestFail()
{
    // Only increment if we haven't reached the threshold.
    if (requestFailureCount < requestFailureThreshold)
    {
        requestFailureCount++;
    }

    if (requestFailureCount == requestFailureThreshold)
    {
        return false;
    }

    return true;
}

void Gpio::initializeRead()
{
    getDeglitchedValue();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    getDeglitchedValue();
    initializedRead = true;
}

std::unique_ptr<GpioInterface> createGPIO(
    const std::string& name, GpioDirection direction, GpioPolarity polarity)
{
    return std::make_unique<Gpio>(name, direction, polarity);
}

} // namespace phosphor::power::chassis
