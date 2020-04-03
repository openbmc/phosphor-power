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

#include "journal.hpp"

#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;

/**
 * This is the standard implementation of the journal interface.  It calls
 * phosphor-logging functions to create real systemd journal entries.
 *
 * Testcases should link with the file mock_journal.cpp instead.
 */
namespace phosphor::power::regulators::journal
{

void logErr(const std::string& message)
{
    log<level::ERR>(message.c_str());
}

void logInfo(const std::string& message)
{
    log<level::INFO>(message.c_str());
}

} // namespace phosphor::power::regulators::journal
