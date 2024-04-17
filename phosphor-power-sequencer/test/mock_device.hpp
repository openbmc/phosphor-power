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

#include "power_sequencer_device.hpp"

#include <gmock/gmock.h>

namespace phosphor::power::sequencer
{

/**
 * @class MockDevice
 *
 * Mock implementation of the PowerSequencerDevice interface.
 */
class MockDevice : public PowerSequencerDevice
{
  public:
    // Specify which compiler-generated methods we want
    MockDevice() = default;
    MockDevice(const MockDevice&) = delete;
    MockDevice(MockDevice&&) = delete;
    MockDevice& operator=(const MockDevice&) = delete;
    MockDevice& operator=(MockDevice&&) = delete;
    virtual ~MockDevice() = default;

    MOCK_METHOD(const std::string&, getName, (), (const, override));
    MOCK_METHOD(const std::vector<std::unique_ptr<Rail>>&, getRails, (),
                (const, override));
    MOCK_METHOD(std::vector<int>, getGPIOValues, (), (override));
    MOCK_METHOD(uint16_t, getStatusWord, (uint8_t page), (override));
    MOCK_METHOD(uint8_t, getStatusVout, (uint8_t page), (override));
    MOCK_METHOD(double, getReadVout, (uint8_t page), (override));
    MOCK_METHOD(double, getVoutUVFaultLimit, (uint8_t page), (override));
    MOCK_METHOD(std::string, findPgoodFault,
                (Services & services, const std::string& powerSupplyError,
                 (std::map<std::string, std::string> & additionalData)),
                (override));
};

} // namespace phosphor::power::sequencer
