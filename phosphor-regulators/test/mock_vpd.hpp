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

#include "vpd.hpp"

#include <gmock/gmock.h>

namespace phosphor::power::regulators
{

/**
 * @class MockVPD
 *
 * Mock implementation of the VPD interface.
 */
class MockVPD : public VPD
{
  public:
    // Specify which compiler-generated methods we want
    MockVPD() = default;
    MockVPD(const MockVPD&) = delete;
    MockVPD(MockVPD&&) = delete;
    MockVPD& operator=(const MockVPD&) = delete;
    MockVPD& operator=(MockVPD&&) = delete;
    virtual ~MockVPD() = default;

    MOCK_METHOD(void, clearCache, (), (override));

    MOCK_METHOD(std::vector<uint8_t>, getValue,
                (const std::string& inventoryPath, const std::string& keyword),
                (override));
};

} // namespace phosphor::power::regulators
