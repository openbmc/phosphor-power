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

#include <cstddef>
#include <cstdint>

namespace phosphor::power::regulators
{

/**
 * @class ErrorHistory
 *
 * This class represents the history of an error.
 *
 * ErrorHistory tracks the error count and whether the error has been logged.
 *
 * This class is often used to limit the number of journal messages and error
 * logs created by code that runs repeatedly, such as sensor monitoring.
 */
class ErrorHistory
{
  public:
    // Specify which compiler-generated methods we want
    ErrorHistory() = default;
    ErrorHistory(const ErrorHistory&) = default;
    ErrorHistory(ErrorHistory&&) = default;
    ErrorHistory& operator=(const ErrorHistory&) = default;
    ErrorHistory& operator=(ErrorHistory&&) = default;
    ~ErrorHistory() = default;

    /**
     * Clears the error history.
     */
    void clear()
    {
        count = 0;
        wasLogged = false;
    }

    /**
     * Increments the error count.
     *
     * Does nothing if the error count is already at the maximum.  This avoids
     * wrapping back to 0 again.
     */
    void incrementCount()
    {
        if (count < SIZE_MAX)
        {
            ++count;
        }
    }

    /**
     * Error count.
     */
    std::size_t count{0};

    /**
     * Indicates whether this error was logged, resulting in an error log entry.
     */
    bool wasLogged{false};
};

} // namespace phosphor::power::regulators
