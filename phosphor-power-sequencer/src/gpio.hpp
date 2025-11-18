/**
 * Copyright © 2025 IBM Corporation
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

#include <string>

namespace phosphor::power::sequencer
{

/**
 * @class GPIO
 *
 * Abstract base class for a General-Purpose Input/Output pin.
 */
class GPIO
{
  public:
    GPIO() = default;
    GPIO(const GPIO&) = delete;
    GPIO(GPIO&&) = delete;
    GPIO& operator=(const GPIO&) = delete;
    GPIO& operator=(GPIO&&) = delete;
    virtual ~GPIO() = default;

    enum class RequestType
    {
        Read,
        Write
    };

    /**
     * Request ownership of the GPIO.
     *
     * Throws an exception if an error occurs.
     *
     * This is required before getting or setting the GPIO value.
     *
     * @param type specifies whether requesting to read or write the GPIO
     */
    virtual void request(RequestType type) = 0;

    /**
     * Gets the value of the GPIO.
     *
     * Throws an exception if an error occurs.
     *
     * @return 0 or 1
     */
    virtual int getValue() = 0;

    /**
     * Sets the value of the GPIO.
     *
     * Throws an exception if an error occurs.
     *
     * @param value new value (0 or 1)
     */
    virtual void setValue(int value) = 0;

    /**
     * Release ownership of the GPIO.
     *
     * Throws an exception if an error occurs.
     */
    virtual void release() = 0;
};

/**
 * @class BMCGPIO
 *
 * Implementation of the GPIO interface using the standard BMC API (libgpiod).
 */
class BMCGPIO : public GPIO
{
  public:
    BMCGPIO() = delete;
    BMCGPIO(const BMCGPIO&) = delete;
    BMCGPIO(BMCGPIO&&) = delete;
    BMCGPIO& operator=(const BMCGPIO&) = delete;
    BMCGPIO& operator=(BMCGPIO&&) = delete;
    virtual ~BMCGPIO() = default;

    /**
     * Constructor.
     *
     * Throws an exception if a GPIO with the specified name cannot be found.
     *
     * @param name GPIO name
     */
    explicit BMCGPIO(const std::string& name)
    {
        line = gpiod::find_line(name);
        if (!line)
        {
            throw std::invalid_argument{"Invalid GPIO name: " + name};
        }
    }

    virtual void request(RequestType type) override
    {
        int lineRequestType;
        if (type == RequestType::Read)
        {
            lineRequestType = gpiod::line_request::DIRECTION_INPUT;
        }
        else
        {
            lineRequestType = gpiod::line_request::DIRECTION_OUTPUT;
        }
        line.request({"phosphor-power-control", lineRequestType, 0});
    }

    virtual int getValue() override
    {
        return line.get_value();
    }

    virtual void setValue(int value) override
    {
        line.set_value(value);
    }

    virtual void release() override
    {
        line.release();
    }

  private:
    /**
     * GPIO line to access the pin.
     */
    gpiod::line line;
};

} // namespace phosphor::power::sequencer
