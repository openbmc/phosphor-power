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

#include "presence_service.hpp"

#include <gmock/gmock.h>

namespace phosphor::power::regulators
{

/**
 * @class MockPresenceService
 *
 * Mock implementation of the PresenceService interface.
 */
class MockPresenceService : public PresenceService
{
  public:
    // Specify which compiler-generated methods we want
    MockPresenceService() = default;
    MockPresenceService(const MockPresenceService&) = delete;
    MockPresenceService(MockPresenceService&&) = delete;
    MockPresenceService& operator=(const MockPresenceService&) = delete;
    MockPresenceService& operator=(MockPresenceService&&) = delete;
    virtual ~MockPresenceService() = default;

    MOCK_METHOD(void, clearCache, (), (override));

    MOCK_METHOD(bool, isPresent, (const std::string& inventoryPath),
                (override));
};

} // namespace phosphor::power::regulators
