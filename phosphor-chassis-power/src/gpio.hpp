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
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::chassis
{

/**
 * @enum GpioDirection
 *
 * GPIO pin direction
 */
enum class GpioDirection
{
    Input,
    Output
};

/**
 * @enum GpioPolarity
 *
 * GPIO pin polarity
 */
enum class GpioPolarity
{
    Low,
    High
};

/**
 * @class Gpio
 *
 * An object that represents GPIO hardware.
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
     * @param direction GPIO direction
     * @param polarity GPIO polarity
     * @param defaultValue optional default value for deglitching GPIO
     */
    explicit Gpio(const std::string& name, GpioDirection direction,
                  GpioPolarity polarity,
                  std::optional<uint8_t> defaultValue = std::nullopt) :
        name{name}, direction{direction}, polarity{polarity},
        defaultValue{defaultValue}
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
    GpioDirection getDirection() const
    {
        return direction;
    }

    /**
     * Returns the polarity of this GPIO.
     *
     * @return GPIO polarity
     */
    GpioPolarity getPolarity() const
    {
        return polarity;
    }

    /**
     * Returns the default value of this GPIO.
     *
     * @return optional GPIO default value, or std::nullopt if not set
     */
    std::optional<uint8_t> getDefaultValue() const
    {
        return defaultValue;
    }

  private:
    /**
     * Unique name of this GPIO.
     */
    const std::string name{};

    /**
     * Direction of GPIO pin.
     */
    const GpioDirection direction{};

    /**
     * Polarity of GPIO pin.
     */
    const GpioPolarity polarity{};

    /**
     * Optional default value of GPIO pin.
     */
    const std::optional<uint8_t> defaultValue{};
};

} // namespace phosphor::power::chassis
