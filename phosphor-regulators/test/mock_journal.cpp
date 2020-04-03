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

#include "mock_journal.hpp"

/**
 * This is the mock implementation of the journal interface.  It just stores the
 * journal messages in static vectors.
 *
 * Executables that need to log real systemd journal messages should link with
 * the file journal.cpp instead.
 */
namespace phosphor::power::regulators::journal
{

/**
 * Mock journal messages with a priority value of 'ERR'.
 */
static std::vector<std::string> errMessages{};

/**
 * Mock journal messages with a priority value of 'INFO'.
 */
static std::vector<std::string> infoMessages{};

void clear()
{
    errMessages.clear();
    infoMessages.clear();
}

const std::vector<std::string>& getErrMessages()
{
    return errMessages;
}

const std::vector<std::string>& getInfoMessages()
{
    return infoMessages;
}

void logErr(const std::string& message)
{
    errMessages.emplace_back(message);
}

void logInfo(const std::string& message)
{
    infoMessages.emplace_back(message);
}

} // namespace phosphor::power::regulators::journal
