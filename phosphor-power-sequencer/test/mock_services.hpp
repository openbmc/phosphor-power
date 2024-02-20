/**
 * Copyright © 2024 IBM Corporation
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

#include "mock_pmbus.hpp"
#include "services.hpp"

#include <gmock/gmock.h>

namespace phosphor::power::sequencer
{

using MockPMBus = phosphor::pmbus::MockPMBus;

/**
 * @class MockServices
 *
 * Mock implementation of the Services interface.
 */
class MockServices : public Services
{
  public:
    // Specify which compiler-generated methods we want
    MockServices() = default;
    MockServices(const MockServices&) = delete;
    MockServices(MockServices&&) = delete;
    MockServices& operator=(const MockServices&) = delete;
    MockServices& operator=(MockServices&&) = delete;
    virtual ~MockServices() = default;

    MOCK_METHOD(sdbusplus::bus_t&, getBus, (), (override));
    MOCK_METHOD(void, logErrorMsg, (const std::string& message), (override));
    MOCK_METHOD(void, logInfoMsg, (const std::string& message), (override));
    MOCK_METHOD(void, logError,
                (const std::string& message, Entry::Level severity,
                 (std::map<std::string, std::string> & additionalData)),
                (override));
    MOCK_METHOD(bool, isPresent, (const std::string& inventoryPath),
                (override));
    MOCK_METHOD(std::vector<int>, getGPIOValues, (const std::string& chipLabel),
                (override));

    virtual std::unique_ptr<PMBusBase>
        createPMBus(uint8_t, uint16_t, const std::string&, size_t) override
    {
        return std::make_unique<MockPMBus>();
    }

    MOCK_METHOD(void, clearCache, (), (override));
};

} // namespace phosphor::power::sequencer
