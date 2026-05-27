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

#include "chassis_status_monitor.hpp"
#include "gpio.hpp"

#include <sdbusplus/bus.hpp>

#include <memory>
#include <optional>
#include <string>

namespace phosphor::power::chassis
{

using ChassisStatusMonitor = phosphor::power::util::ChassisStatusMonitor;
using BMCChassisStatusMonitor = phosphor::power::util::BMCChassisStatusMonitor;
using ChassisStatusMonitorOptions =
    phosphor::power::util::ChassisStatusMonitorOptions;

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
     * Returns the D-Bus bus object.
     *
     * @return D-Bus bus
     */
    virtual sdbusplus::bus_t& getBus() = 0;

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

    /**
     * Creates object for monitoring the status of a chassis using D-Bus
     * properties.
     *
     * @param number Chassis number within the system. Must be >= 0.
     * @param inventoryPath D-Bus inventory path of the chassis.
     * @param options Options that specify what types of monitoring are enabled.
     * @return ChassisStatusMonitor object
     */
    virtual std::unique_ptr<ChassisStatusMonitor> createChassisStatusMonitor(
        size_t number, const std::string& inventoryPath,
        const ChassisStatusMonitorOptions& options) = 0;
};

/**
 * @class BMCServices
 *
 * Implementation of the Services interface using standard BMC system services.
 */
class BMCServices : public Services
{
  public:
    BMCServices() = delete;
    BMCServices(const BMCServices&) = delete;
    BMCServices(BMCServices&&) = delete;
    BMCServices& operator=(const BMCServices&) = delete;
    BMCServices& operator=(BMCServices&&) = delete;
    ~BMCServices() override = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit BMCServices(sdbusplus::bus_t& bus) : bus{bus} {}

    /** @copydoc Services::getBus() */
    sdbusplus::bus_t& getBus() override
    {
        return bus;
    }

    /** @copydoc Services::createGPIO() */
    std::unique_ptr<Gpio> createGPIO(
        const std::string& name, GpioDirection direction, GpioPolarity polarity,
        std::optional<uint8_t> defaultValue = std::nullopt) override;

    /** @copydoc Services::createChassisStatusMonitor() */
    std::unique_ptr<ChassisStatusMonitor> createChassisStatusMonitor(
        size_t number, const std::string& inventoryPath,
        const ChassisStatusMonitorOptions& options) override;

  private:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t& bus;
};

} // namespace phosphor::power::chassis
