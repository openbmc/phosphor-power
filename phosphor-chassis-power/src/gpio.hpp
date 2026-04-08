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
 * @class Gpio
 *
 * Abstract base class for a General-Purpose Input/Output pin.
 */
class Gpio
{
  public:
    Gpio() = default;
    Gpio(const Gpio&) = delete;
    Gpio(Gpio&&) = delete;
    Gpio& operator=(const Gpio&) = delete;
    Gpio& operator=(Gpio&&) = delete;
    virtual ~Gpio() = default;

    /**
     * Find the line for the GPIO.
     *
     * Throws an exception if an error occurs.
     */
    virtual bool findLine() = 0;

    /**
     * Check if the GPIO line was found.
     *
     * @return true if line is found, false otherwise
     */
    virtual bool foundLine() const = 0;

    /**
     * Request ownership of the GPIO.
     *
     * Throws an exception if an error occurs.
     *
     * @return whether the GPIO request is successful.
     */
    virtual bool requestRead() = 0;

    /**
     * Request ownership of the GPIO with an initial value.
     *
     * Throws an exception if an error occurs.
     *
     * @param initialValue initial value for output GPIOs
     *
     * @return whether the GPIO request is successful.
     */
    virtual bool requestWrite(int initialValue) = 0;

    /**
     * Gets the Previous value of the GPIO pin.
     *
     * Throws an exception if no previous value.
     *
     * @return Previous GPIO pin value
     */
    virtual int getPreviousValue() = 0;

    /**
     * Gets the value of the GPIO pin.
     *
     * Throws an exception if an error occurs.
     *
     * @return GPIO pin value
     */
    virtual int getValue() = 0;

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

    /**
     * Returns the default value of this GPIO.
     *
     * @return optional GPIO default value, or std::nullopt if not set
     */
    virtual std::optional<uint8_t> getDefaultValue() const = 0;

    /**
     * Clears the error history for this GPIO.
     */
    virtual void clearErrorHistory() = 0;

    /**
     * Gets the current request failure count.
     *
     * @return Current failure count
     */
    virtual int getRequestFailureCount() const = 0;

    /**
     * Gets the current line not found failure count.
     *
     * @return Current failure count
     */
    virtual int getLineNotFoundCount() const = 0;

    /**
     * Gets the current read/write failure count.
     *
     * @return Current failure count
     */
    virtual int getReadWriteFailureCount() const = 0;
};

/**
 * @class BMCGpio
 *
 * Implementation of the Gpio interface using the standard BMC API (libgpiod).
 */
class BMCGpio : public Gpio
{
  public:
    // Specify which compiler-generated methods we want
    BMCGpio() = delete;
    BMCGpio(const BMCGpio&) = delete;
    BMCGpio(BMCGpio&&) = delete;
    BMCGpio& operator=(const BMCGpio&) = delete;
    BMCGpio& operator=(BMCGpio&&) = delete;
    ~BMCGpio();

    /**
     * Constructor.
     *
     * @param name unique GPIO name
     * @param direction GPIO direction
     * @param polarity GPIO polarity
     * @param defaultValue optional default value for deglitching GPIO
     */
    explicit BMCGpio(const std::string& name, GpioDirection direction,
                     GpioPolarity polarity,
                     std::optional<uint8_t> defaultValue = std::nullopt);

    /** @copydoc Gpio::findLine() */
    virtual bool findLine() override;

    /** @copydoc Gpio::requestRead() */
    virtual bool requestRead() override;

    /** @copydoc Gpio::requestWrite(int) */
    virtual bool requestWrite(int initialValue) override;

    /** @copydoc Gpio::getValue() */
    virtual int getValue() override;

    /** @copydoc Gpio::getPreviousValue() */
    virtual int getPreviousValue() override;

    /** @copydoc Gpio::setValue() */
    virtual void setValue(int value) override;

    /** @copydoc Gpio::release() */
    virtual void release() override;

    /** @copydoc Gpio::getName() */
    virtual const std::string& getName() const override
    {
        return name;
    }

    /** @copydoc Gpio::getDirection() */
    virtual GpioDirection getDirection() const override
    {
        return direction;
    }

    /** @copydoc Gpio::getPolarity() */
    virtual GpioPolarity getPolarity() const override
    {
        return polarity;
    }

    /**
     * Returns the default value of this GPIO.
     *
     * @return optional GPIO default value, or std::nullopt if not set
     */
    std::optional<uint8_t> getDefaultValue() const override
    {
        return defaultValue;
    }

    /** @copydoc Gpio::clearErrorHistory() */
    void clearErrorHistory() override;

    /** @copydoc Gpio::getRequestFailureCount() */
    int getRequestFailureCount() const override
    {
        return requestFailureCount;
    }

    /** @copydoc Gpio::getLineNotFoundCount() */
    int getLineNotFoundCount() const override
    {
        return lineNotFoundCount;
    }

    /** @copydoc Gpio::getReadWriteFailureCount() */
    int getReadWriteFailureCount() const override
    {
        return readWriteFailureCount;
    }

    /** @copydoc Gpio::foundLine() */
    bool foundLine() const override
    {
        return lineFound;
    }

  private:
    /**
     * Increments a failure counter and determines if error should be suppressed
     *
     * @param counter Reference to the counter to increment
     * @return true to suppress error logging, false to log error
     */
    bool shouldSuppressError(int& counter);

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
     * Flag to indicate if the GPIO line was found.
     */
    bool lineFound = false;

    /**
     * Previous value from this GPIO.
     */
    std::optional<int> previousValue{};

    /**
     * Value for this GPIO.
     */
    std::optional<int> gpioValue{};

    /**
     * Counter for number of request failures.
     */
    int requestFailureCount = 0;

    /**
     * Counter for number of line not found failures.
     */
    int lineNotFoundCount = 0;

    /**
     * Counter for read/write failures.
     */
    int readWriteFailureCount = 0;

    /**
     * Threshold for number of failures before logging an error.
     */
    static constexpr int failureThreshold = 10;

    /**
     * Flag to track if the first attempt to read from the GPIO has been made.
     */
    bool firstRead = true;
};

} // namespace phosphor::power::chassis
