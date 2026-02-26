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

#include "gpio.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::chassis
{

// Forward declarations to avoid circular dependencies
class System;

/**
 * @class Chassis
 *
 * A chassis within the system.
 *
 * Chassis are large enclosures that can be independently powered off and on by
 * the BMC.  Small and mid-sized systems may contain a single chassis.  In a
 * large rack-mounted system, each drawer may correspond to a chassis.
 *
 * A C++ Chassis object only needs to be created if the physical chassis
 * contains chassis that need to be configured or monitored.
 */
class Chassis
{
  public:
    // Specify which compiler-generated methods we want
    Chassis() = delete;
    Chassis(const Chassis&) = delete;
    Chassis(Chassis&&) = delete;
    Chassis& operator=(const Chassis&) = delete;
    Chassis& operator=(Chassis&&) = delete;
    ~Chassis() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param number Chassis number within the system.  Chassis numbers start at
     *               1 because chassis 0 represents the entire system.
     * @param presencePath Presence path for this chassis
     * @param GPIOs within this chassis, if any.  The vector should
     *                contain GPIOs to perform operations on.
     */
    explicit Chassis(unsigned int number,
                     std::optional<std::string> presencePath = std::nullopt,
                     std::vector<std::unique_ptr<Gpio>> gpioPins =
                         std::vector<std::unique_ptr<Gpio>>{}) :
        number{std::move(number)}, presencePath{std::move(presencePath)},
        gpios{std::move(gpioPins)}
    {
        if (number < 1)
        {
            throw std::invalid_argument{
                "Invalid chassis number: " + std::to_string(number)};
        }
    }

    /**
     * Returns the chassis number within the system.
     *
     * @return chassis number
     */
    unsigned int getNumber() const
    {
        return number;
    }

    /**
     * Returns the Presence path for this chassis, if any.
     *
     * @return presence path, or std::nullopt if not specified
     */
    const std::optional<std::string>& getPresencePath() const
    {
        return presencePath;
    }

    /**
     * Returns the GPIO objects within this chassis, if any.
     *
     * The vector contains GPIO objects to perform operations.
     *
     * @return GPIO objects in chassis
     */
    const std::vector<std::unique_ptr<Gpio>>& getGpios() const
    {
        return gpios;
    }

  private:
    /**
     * Chassis number within the system.
     *
     * Chassis numbers start at 1 because chassis 0 represents the entire
     * system.
     */
    const unsigned int number{};

    /**
     * Presence path for this chassis, if any.
     */
    const std::optional<std::string> presencePath{};

    /**
     * GPIO objects within this chassis, if any.
     *
     * The vector contains GPIO objects to perform operations.
     */
    std::vector<std::unique_ptr<Gpio>> gpios{};
};

} // namespace phosphor::power::chassis
