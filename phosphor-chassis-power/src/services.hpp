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
#include <string>

namespace phosphor::power::chassis
{

/**
 * @class Services
 *
 * Abstract base class providing an interface to system services like GPIOs.
 */
class Services
{
  public:
    Services() = default;
    Services(const Services&) = delete;
    Services(Services&&) = delete;
    Services& operator=(const Services&) = delete;
    Services& operator=(Services&&) = delete;
    virtual ~Services() = default;

    /**
     * Creates a GPIO object.
     *
     * @param name GPIO name
     * @param direction GPIO direction
     * @param polarity GPIO polarity
     * @param defaultValue optional default value
     * @return unique pointer to Gpio
     */
    virtual std::unique_ptr<Gpio> createGPIO(
        const std::string& name, GpioDirection direction, GpioPolarity polarity,
        std::optional<uint8_t> defaultValue = std::nullopt) = 0;
};

/**
 * @class BMCServices
 *
 * Implementation of the Services interface using standard BMC system services.
 */
class BMCServices : public Services
{
  public:
    BMCServices() = default;
    BMCServices(const BMCServices&) = delete;
    BMCServices(BMCServices&&) = delete;
    BMCServices& operator=(const BMCServices&) = delete;
    BMCServices& operator=(BMCServices&&) = delete;
    ~BMCServices() override = default;

    /** @copydoc Services::createGPIO() */
    std::unique_ptr<Gpio> createGPIO(
        const std::string& name, GpioDirection direction, GpioPolarity polarity,
        std::optional<uint8_t> defaultValue = std::nullopt) override;
};

} // namespace phosphor::power::chassis
