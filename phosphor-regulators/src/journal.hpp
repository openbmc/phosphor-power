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

#include <string>

/**
 * Systemd journal interface.
 *
 * Provides functions to log messages to the systemd journal.
 *
 * This interface provides an abstraction layer so that testcases can use a mock
 * implementation and validate the logged messages.
 *
 * This interface does not currently provide the ability to specify key/value
 * pairs to provide more information in the journal entry.  It will be added
 * later if needed.
 */
namespace phosphor::power::regulators::journal
{

/**
 * Logs a message with a priority value of 'ERR' to the systemd journal.
 *
 * @param message message to log
 */
void logErr(const std::string& message);

/**
 * Logs a message with a priority value of 'INFO' to the systemd journal.
 *
 * @param message message to log
 */
void logInfo(const std::string& message);

/**
 * Logs a message with a priority value of 'DEBUG' to the systemd journal.
 *
 * @param message message to log
 */
void logDebug(const std::string& message);

} // namespace phosphor::power::regulators::journal
