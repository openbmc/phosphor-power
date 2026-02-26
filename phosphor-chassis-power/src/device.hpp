/**
 * Copyright © 2019 IBM Corporation
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
 * @class Device
 *
 * A hardware device, such as a voltage ????? or I/O expander.
 */
class Device
{
  public:
    // Specify which compiler-generated methods we want
    Device() = delete;
    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;
    ~Device() = default;

    /**
     * Constructor.
     *
     * @param name unique device name
     * @param direction direction
     * @param polarity polarity
     */
    explicit Device(const std::string& name, const std::string& direction,
                    const std::string& polarity) :
        name{name}, direction{direction}, polarity{polarity}
    {}

    /**
     * Returns the unique name of this device.
     *
     * @return device name
     */
    const std::string& getName() const
    {
        return name;
    }

    /**
     * Returns the direction of this device.
     *
     * @return device direction
     */
    const std::string& getDirection() const
    {
        return direction;
    }

    /**
     * Returns the polarity of this device.
     *
     * @return device polarity
     */
    const std::string& getPolarity() const
    {
        return polarity;
    }

  private:
    /**
     * Unique name of this device.
     */
    const std::string name{};

    /**
     * direction of GPIO pin.
     */
    const std::string direction{};

    /**
     * polarity of GPIO pin.
     */
    const std::string polarity{};
};

} // namespace phosphor::power::chassis
