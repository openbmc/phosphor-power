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

#include "journal.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>

namespace phosphor::power::regulators
{

/**
 * @class MockJournal
 *
 * Mock implementation of the Journal interface.
 */
class MockJournal : public Journal
{
  public:
    // Specify which compiler-generated methods we want
    MockJournal() = default;
    MockJournal(const MockJournal&) = delete;
    MockJournal(MockJournal&&) = delete;
    MockJournal& operator=(const MockJournal&) = delete;
    MockJournal& operator=(MockJournal&&) = delete;
    virtual ~MockJournal() = default;

    MOCK_METHOD(void, logDebug, (const std::string& message), (override));
    MOCK_METHOD(void, logDebug, (const std::vector<std::string>& messages),
                (override));
    MOCK_METHOD(void, logError, (const std::string& message), (override));
    MOCK_METHOD(void, logError, (const std::vector<std::string>& messages),
                (override));
    MOCK_METHOD(void, logInfo, (const std::string& message), (override));
    MOCK_METHOD(void, logInfo, (const std::vector<std::string>& messages),
                (override));
};

} // namespace phosphor::power::regulators

// TODO: Remove the functional interface below once all the code has switched to
// the new Journal interface.  Also delete mock_journal.cpp and remove
// references to it in meson files.

/**
 * Extensions to the systemd journal interface.
 *
 * Provides functions to access and clear mock journal messages that have been
 * logged.  These functions should only be used by testcases.
 */
namespace phosphor::power::regulators::journal
{

/**
 * Clears all mock journal messages.
 */
void clear();

/**
 * Returns all mock journal messages with a priority value of 'ERR'.
 *
 * @return mock error messages
 */
const std::vector<std::string>& getErrMessages();

/**
 * Returns all mock journal messages with a priority value of 'INFO'.
 *
 * @return mock informational messages
 */
const std::vector<std::string>& getInfoMessages();

/**
 * Returns all mock journal messages with a priority value of 'DEBUG'.
 *
 * @return mock debug messages
 */
const std::vector<std::string>& getDebugMessages();

} // namespace phosphor::power::regulators::journal
