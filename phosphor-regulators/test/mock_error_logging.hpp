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

#include "error_logging.hpp"

#include <gmock/gmock.h>

namespace phosphor::power::regulators
{

/**
 * @class MockErrorLogging
 *
 * Mock implementation of the ErrorLogging interface.
 */
class MockErrorLogging : public ErrorLogging
{
  public:
    // Specify which compiler-generated methods we want
    MockErrorLogging() = default;
    MockErrorLogging(const MockErrorLogging&) = delete;
    MockErrorLogging(MockErrorLogging&&) = delete;
    MockErrorLogging& operator=(const MockErrorLogging&) = delete;
    MockErrorLogging& operator=(MockErrorLogging&&) = delete;
    virtual ~MockErrorLogging() = default;

    MOCK_METHOD(void, logConfigFileError,
                (Entry::Level severity, Journal& journal), (override));

    MOCK_METHOD(void, logDBusError, (Entry::Level severity, Journal& journal),
                (override));

    MOCK_METHOD(void, logI2CError,
                (Entry::Level severity, Journal& journal,
                 const std::string& bus, uint8_t addr, int errorNumber),
                (override));

    MOCK_METHOD(void, logInternalError,
                (Entry::Level severity, Journal& journal), (override));

    MOCK_METHOD(void, logPMBusError,
                (Entry::Level severity, Journal& journal,
                 const std::string& inventoryPath),
                (override));

    MOCK_METHOD(void, logWriteVerificationError,
                (Entry::Level severity, Journal& journal,
                 const std::string& inventoryPath),
                (override));
};

} // namespace phosphor::power::regulators
