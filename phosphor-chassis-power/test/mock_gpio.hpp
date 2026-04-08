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

#include <gmock/gmock.h>

namespace phosphor::power::chassis
{

/**
 * @class MockGpio
 *
 * Mock implementation of GpioInterface for testing.
 */
class MockGpio : public GpioInterface
{
  public:
    MockGpio(const std::string& name, GpioDirection direction,
             GpioPolarity polarity,
             std::optional<uint8_t> defaultValue = std::nullopt) :
        name{name}, direction{direction}, polarity{polarity},
        defaultValue{defaultValue}
    {
        ON_CALL(*this, getName()).WillByDefault(testing::ReturnRef(this->name));
        ON_CALL(*this, getDirection())
            .WillByDefault(testing::Return(this->direction));
        ON_CALL(*this, getPolarity())
            .WillByDefault(testing::Return(this->polarity));
        ON_CALL(*this, getDefaultValue())
            .WillByDefault(testing::Return(this->defaultValue));
    }

    ~MockGpio() override = default;

    MOCK_METHOD(bool, findLine, (), (override));
    MOCK_METHOD(bool, foundLine, (), (const, override));
    MOCK_METHOD(bool, requestRead, (), (override));
    MOCK_METHOD(bool, requestWrite, (int initialValue), (override));
    MOCK_METHOD(int, getValue, (), (override));
    MOCK_METHOD(int, getPreviousValue, (), (override));
    MOCK_METHOD(void, setValue, (int value), (override));
    MOCK_METHOD(void, release, (), (override));
    MOCK_METHOD(const std::string&, getName, (), (const, override));
    MOCK_METHOD(GpioDirection, getDirection, (), (const, override));
    MOCK_METHOD(GpioPolarity, getPolarity, (), (const, override));
    MOCK_METHOD(std::optional<uint8_t>, getDefaultValue, (), (const, override));
    MOCK_METHOD(void, clearErrorHistory, (), (override));
    MOCK_METHOD(int, getRequestFailureCount, (), (const, override));
    MOCK_METHOD(int, getLineNotFoundCount, (), (const, override));

  private:
    const std::string name;
    const GpioDirection direction;
    const GpioPolarity polarity;
    const std::optional<uint8_t> defaultValue;
};

} // namespace phosphor::power::chassis
