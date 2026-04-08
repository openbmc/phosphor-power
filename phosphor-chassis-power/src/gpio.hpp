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

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

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
 * @class GpioInterface
 *
 * Abstract base class for a General-Purpose Input/Output pin.
 */
class GpioInterface
{
  public:
    GpioInterface() = default;
    GpioInterface(const GpioInterface&) = delete;
    GpioInterface(GpioInterface&&) = delete;
    GpioInterface& operator=(const GpioInterface&) = delete;
    GpioInterface& operator=(GpioInterface&&) = delete;
    virtual ~GpioInterface() = default;

    /**
     * Request ownership of the GPIO.
     *
     * Throws an exception if an error occurs.
     */
    virtual void requestLine() = 0;

    /**
     * Request ownership of the GPIO with an initial value.
     *
     * Throws an exception if an error occurs.
     *
     * @param initialValue initial value for output GPIOs
     */
    virtual void requestLine(int initialValue) = 0;

    /**
     * Gets the value of the GPIO pin.
     *
     * Throws an exception if an error occurs.
     *
     * @return GPIO pin value
     */
    virtual int getDeglitchedValue() = 0;

    /**
     * Sets the value of the GPIO pin.
     *
     * Throws an exception if an error occurs.
     *
     * @param value new value
     */
    virtual void setValue(int value) = 0;

    /**
     * Release ownership of the GPIO.
     *
     * Throws an exception if an error occurs.
     */
    virtual void release() = 0;

    /**
     * Returns the unique name of this GPIO.
     *
     * @return GPIO name
     */
    virtual const std::string& getName() const = 0;

    /**
     * Returns the direction of this GPIO.
     *
     * @return GPIO direction
     */
    virtual GpioDirection getDirection() const = 0;

    /**
     * Returns the polarity of this GPIO.
     *
     * @return GPIO polarity
     */
    virtual GpioPolarity getPolarity() const = 0;

  private:
    /**
     * Gets the value of the GPIO pin.
     *
     * Throws an exception if an error occurs.
     *
     * @return GPIO pin value
     */
    virtual int getValue() = 0;
};

/**
 * @class Gpio
 *
 * Implementation of the GpioInterface using the standard BMC API (libgpiod).
 */
class Gpio : public GpioInterface
{
  public:
    // Specify which compiler-generated methods we want
    Gpio() = delete;
    Gpio(const Gpio&) = delete;
    Gpio(Gpio&&) = delete;
    Gpio& operator=(const Gpio&) = delete;
    Gpio& operator=(Gpio&&) = delete;
    ~Gpio();

    /**
     * Constructor.
     *
     * @param name unique GPIO name
     * @param direction GPIO direction
     * @param polarity GPIO polarity
     * @param validateHardware if true, validate GPIO exists in hardware during
     *                         construction. Set to false for testing when
     *                         hardware may not be available. Will Default to
     *                         true when dts is updated.
     */
    explicit Gpio(const std::string& name, GpioDirection direction,
                  GpioPolarity polarity, bool validateHardware = true);

    /** @copydoc GpioInterface::requestLine() */
    virtual void requestLine() override;

    /** @copydoc GpioInterface::requestLine(int) */
    virtual void requestLine(int initialValue) override;

    /** @copydoc GpioInterface::getDeglitchedValue() */
    virtual int getDeglitchedValue() override;

    /** @copydoc GpioInterface::setValue() */
    virtual void setValue(int value) override;

    /** @copydoc GpioInterface::release() */
    virtual void release() override;

    /** @copydoc GpioInterface::getName() */
    virtual const std::string& getName() const override
    {
        return name;
    }

    /** @copydoc GpioInterface::getDirection() */
    virtual GpioDirection getDirection() const override
    {
        return direction;
    }

    /** @copydoc GpioInterface::getPolarity() */
    virtual GpioPolarity getPolarity() const override
    {
        return polarity;
    }

  private:
    /** @copydoc GpioInterface::getValue() */
    virtual int getValue() override;

    /**
     * Increments the request fail counter, and logs an error
     * if it reaches the threshold.
     */
    void handleRequestFail();
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
     * Consumer value specified when requesting the GPIO line.
     */
    inline static const std::string consumer{"phosphor-chassis-power"};

    /**
     * Request flags based on GPIO polarity.
     * Set to FLAG_ACTIVE_LOW if polarity is Low, else 0 (default active high).
     */
    const ::std::bitset<32> requestFlags;

    /**
     * GPIO line to access the pin.
     */
    gpiod::line line;

    /**
     * Last reading from this GPIO.
     */
    std::optional<int> lastReading{};

    /**
     * Last reading from this GPIO. Set to be opposite of polarity on
     * initialization.
     */
    std::optional<int> deglitchedValue{};

    /**
     * Counter for number of request failures.
     */
    int requestFailureCount = 0;

    /**
     * Threshold for number of request failures before logging an error.
     */
    static constexpr int requestFailureThreshold = 10;
};

} // namespace phosphor::power::chassis
