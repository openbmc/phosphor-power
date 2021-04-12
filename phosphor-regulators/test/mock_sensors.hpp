/**
 * Copyright © 2021 IBM Corporation
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

#include "sensors.hpp"
#include "services.hpp"

#include <string>

#include <gmock/gmock.h>

namespace phosphor::power::regulators
{

/**
 * @class MockSensors
 *
 * Mock implementation of the Sensors interface.
 */
class MockSensors : public Sensors
{
  public:
    // Specify which compiler-generated methods we want
    MockSensors() = default;
    MockSensors(const MockSensors&) = delete;
    MockSensors(MockSensors&&) = delete;
    MockSensors& operator=(const MockSensors&) = delete;
    MockSensors& operator=(MockSensors&&) = delete;
    virtual ~MockSensors() = default;

    MOCK_METHOD(void, enable, (Services & services), (override));

    MOCK_METHOD(void, endCycle, (Services & services), (override));

    MOCK_METHOD(void, endRail, (bool errorOccurred, Services& services),
                (override));

    MOCK_METHOD(void, disable, (Services & services), (override));

    MOCK_METHOD(void, setValue,
                (SensorType type, double value, Services& services),
                (override));

    MOCK_METHOD(void, startCycle, (Services & services), (override));

    MOCK_METHOD(void, startRail,
                (const std::string& rail,
                 const std::string& deviceInventoryPath,
                 const std::string& chassisInventoryPath, Services& services),
                (override));
};

} // namespace phosphor::power::regulators
