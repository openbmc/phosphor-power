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

#include "gpio.hpp"

#include <gmock/gmock.h>

namespace phosphor::power::sequencer
{

/**
 * @class MockGPIO
 *
 * Mock implementation of the GPIO interface.
 */
class MockGPIO : public GPIO
{
  public:
    MockGPIO() = default;
    MockGPIO(const MockGPIO&) = delete;
    MockGPIO(MockGPIO&&) = delete;
    MockGPIO& operator=(const MockGPIO&) = delete;
    MockGPIO& operator=(MockGPIO&&) = delete;
    virtual ~MockGPIO() = default;

    MOCK_METHOD(void, requestRead, (), (override));
    MOCK_METHOD(void, requestWrite, (int initialValue), (override));
    MOCK_METHOD(int, getValue, (), (override));
    MOCK_METHOD(void, setValue, (int value), (override));
    MOCK_METHOD(void, release, (), (override));
};

} // namespace phosphor::power::sequencer
