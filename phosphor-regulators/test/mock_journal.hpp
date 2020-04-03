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

} // namespace phosphor::power::regulators::journal
