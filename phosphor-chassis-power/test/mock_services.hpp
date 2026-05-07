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

#include "mock_gpio.hpp"
#include "services.hpp"

#include <mock_chassis_status_monitor.hpp>

#include <memory>

#include <gmock/gmock.h>

namespace phosphor::power::chassis
{

using MockChassisStatusMonitor =
    phosphor::power::util::MockChassisStatusMonitor;

/**
 * @class MockServices
 *
 * Mock implementation of the Services interface.
 */
class MockServices : public Services
{
  public:
    MockServices() = default;
    MockServices(const MockServices&) = delete;
    MockServices(MockServices&&) = delete;
    MockServices& operator=(const MockServices&) = delete;
    MockServices& operator=(MockServices&&) = delete;
    ~MockServices() override = default;

    sdbusplus::bus_t& getBus() override
    {
        return bus;
    }

    std::unique_ptr<Gpio> createGPIO(
        const std::string& name, GpioDirection direction, GpioPolarity polarity,
        std::optional<uint8_t> defaultValue = std::nullopt) override
    {
        return std::make_unique<testing::NiceMock<MockGpio>>(
            name, direction, polarity, defaultValue);
    }

    std::unique_ptr<ChassisStatusMonitor> createChassisStatusMonitor(
        size_t, const std::string&, const ChassisStatusMonitorOptions&) override
    {
        return std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
    }

    MOCK_METHOD(void, logError,
                (const std::string& message, Entry::Level severity,
                 (std::map<std::string, std::string> & additionalData)),
                (override));

  private:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t bus{sdbusplus::bus::new_default()};
};

} // namespace phosphor::power::chassis
