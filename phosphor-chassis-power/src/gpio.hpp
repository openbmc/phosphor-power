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
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::chassis
{

// Forward declarations to avoid circular dependencies
class Chassis;
class System;

/**
 * @enum gpioDirection
 *
 * GPIO pin direction
 */
enum class gpioDirection
{
    Input,
    Output
};

/**
 * @enum gpioPolarity
 *
 * GPIO pin polarity
 */
enum class gpioPolarity
{
    Low,
    High
};

/**
 * @class Gpio
 *
 * A hardware GPIO, such as a voltage regulator or I/O expander.
 */
class Gpio
{
  public:
    // Specify which compiler-generated methods we want
    Gpio() = delete;
    Gpio(const Gpio&) = delete;
    Gpio(Gpio&&) = delete;
    Gpio& operator=(const Gpio&) = delete;
    Gpio& operator=(Gpio&&) = delete;
    ~Gpio() = default;

    /**
     * Constructor.
     *
     * @param name unique GPIO name
     * @param gpioDirection direction Output,Input
     * @param gpioPolarity polarity Low,High
     */
    explicit Gpio(const std::string& name, gpioDirection direction,
                  gpioPolarity polarity) :
        name{std::move(name)}, direction{std::move(direction)},
        polarity{std::move(polarity)}
    {}

    /**
     * Returns the unique name of this GPIO.
     *
     * @return GPIO name
     */
    const std::string& getName() const
    {
        return name;
    }

    /**
     * Returns the direction of this GPIO.
     *
     * @return GPIO direction
     */
    gpioDirection getDirection() const
    {
        return direction;
    }

    /**
     * Returns the polarity of this GPIO.
     *
     * @return GPIO polarity
     */
    gpioPolarity getPolarity() const
    {
        return polarity;
    }

  private:
    /**
     * Unique name of this GPIO.
     */
    const std::string name{};

    /**
     * Direction of GPIO pin.
     */
    const gpioDirection direction{};

    /**
     * Polarity of GPIO pin.
     */
    const gpioPolarity polarity{};
};

/**
 * Parses a string to a gpioDirection enum value.
 *
 * @param directionStr Direction string ("Input" or "Output")
 * @return gpioDirection enum value
 * @throws std::invalid_argument if string is invalid
 */
gpioDirection parseDirection(const std::string& directionStr);

/**
 * Parses a string to a gpioPolarity enum value.
 *
 * @param polarityStr Polarity string ("Low" or "High")
 * @return gpioPolarity enum value
 * @throws std::invalid_argument if string is invalid
 */
gpioPolarity parsePolarity(const std::string& polarityStr);

} // namespace phosphor::power::chassis
