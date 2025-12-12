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

#include "chassis_status_monitor.hpp"

#include <stddef.h> // for size_t

#include <xyz/openbmc_project/State/Decorator/PowerSystemInputs/server.hpp>

#include <string>

#include <gmock/gmock.h>

namespace phosphor::power::util
{

/**
 * @class MockChassisStatusMonitor
 *
 * Mock implementation of the ChassisStatusMonitor interface.
 */
class MockChassisStatusMonitor : public ChassisStatusMonitor
{
  public:
    MockChassisStatusMonitor() = default;
    MockChassisStatusMonitor(const MockChassisStatusMonitor&) = delete;
    MockChassisStatusMonitor(MockChassisStatusMonitor&&) = delete;
    MockChassisStatusMonitor& operator=(const MockChassisStatusMonitor&) =
        delete;
    MockChassisStatusMonitor& operator=(MockChassisStatusMonitor&&) = delete;
    virtual ~MockChassisStatusMonitor() = default;

    MOCK_METHOD(size_t, getNumber, (), (const, override));
    MOCK_METHOD(const std::string&, getInventoryPath, (), (const, override));
    MOCK_METHOD(const ChassisStatusMonitorOptions&, getOptions, (),
                (const, override));
    MOCK_METHOD(bool, isPresent, (), (override));
    MOCK_METHOD(bool, isAvailable, (), (override));
    MOCK_METHOD(bool, isEnabled, (), (override));
    MOCK_METHOD(int, getPowerState, (), (override));
    MOCK_METHOD(int, getPowerGood, (), (override));
    MOCK_METHOD(bool, isPoweredOn, (), (override));
    MOCK_METHOD(bool, isPoweredOff, (), (override));
    MOCK_METHOD(PowerSystemInputs::Status, getInputPowerStatus, (), (override));
    MOCK_METHOD(bool, isInputPowerGood, (), (override));
    MOCK_METHOD(PowerSystemInputs::Status, getPowerSuppliesStatus, (),
                (override));
    MOCK_METHOD(bool, isPowerSuppliesPowerGood, (), (override));
};

} // namespace phosphor::power::util
