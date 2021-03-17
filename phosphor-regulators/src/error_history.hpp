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

#include <bitset>

namespace phosphor::power::regulators
{

/**
 * @enum ErrorType
 *
 * The error types tracked by the ErrorHistory class.
 *
 * The enumerators must have consecutive integer values that start at 0.  The
 * value of the last enumerator must be the number of error types.
 */
enum class ErrorType
{
    configFile = 0,
    dbus = 1,
    i2c = 2,
    internal = 3,
    pmbus = 4,
    writeVerification = 5,
    numTypes = 6
};

/**
 * @class ErrorHistory
 *
 * History of which error types have been logged.
 *
 * The class is used to avoid creating duplicate error log entries.
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
     *
     * Sets all error types to a 'not logged' state.
     */
    void clear()
    {
        // Set all bits to false
        history.reset();
    }

    /**
     * Sets whether the specified error type has been logged.
     *
     * @param errorType error type
     * @param wasLogged indicates whether an error was logged
     */
    void setWasLogged(ErrorType errorType, bool wasLogged)
    {
        // Set bit value for the specified error type
        history[static_cast<int>(errorType)] = wasLogged;
    }

    /**
     * Returns whether the specified error type has been logged.
     *
     * @param errorType error type
     * @return whether specified error type was logged
     */
    bool wasLogged(ErrorType errorType) const
    {
        // Return bit value for the specified error type
        return history[static_cast<int>(errorType)];
    }

  private:
    /**
     * Bitset used to track which error types have been logged.
     *
     * Each bit indicates whether one error type was logged.
     *
     * Each ErrorType enum value is the position of the corresponding bit in the
     * bitset.
     *
     * The numTypes enum value is the number of bits needed in the bitset.
     */
    std::bitset<static_cast<int>(ErrorType::numTypes)> history{};
};

} // namespace phosphor::power::regulators
