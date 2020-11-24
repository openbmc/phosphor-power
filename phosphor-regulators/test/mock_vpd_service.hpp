/**
 * Copyright © 2020 IBM Corporation
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

#include "vpd_service.hpp"

#include <gmock/gmock.h>

namespace phosphor::power::regulators
{

/**
 * @class MockVPDService
 *
 * Mock implementation of the VPDService interface.
 */
class MockVPDService : public VPDService
{
  public:
    // Specify which compiler-generated methods we want
    MockVPDService() = default;
    MockVPDService(const MockVPDService&) = delete;
    MockVPDService(MockVPDService&&) = delete;
    MockVPDService& operator=(const MockVPDService&) = delete;
    MockVPDService& operator=(MockVPDService&&) = delete;
    virtual ~MockVPDService() = default;

    MOCK_METHOD(void, clearCache, (), (override));

    MOCK_METHOD(std::string, getValue,
                (const std::string& inventoryPath, const std::string& keyword),
                (override));
};

} // namespace phosphor::power::regulators
