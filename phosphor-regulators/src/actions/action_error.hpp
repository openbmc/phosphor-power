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

#include "action.hpp"

#include <exception>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class ActionError
 *
 * An error that occurred while executing an action.
 *
 * This exception describes the action that failed.  If the cause of the failure
 * was another exception (such as I2CException), the other exception can be
 * nested inside the ActionError using std::throw_with_nested().
 */
class ActionError : public std::exception
{
  public:
    // Specify which compiler-generated methods we want
    ActionError() = delete;
    ActionError(const ActionError&) = default;
    ActionError(ActionError&&) = default;
    ActionError& operator=(const ActionError&) = default;
    ActionError& operator=(ActionError&&) = default;
    virtual ~ActionError() = default;

    /**
     * Constructor.
     *
     * @param action action that was executed
     * @param error error message
     */
    explicit ActionError(const Action& action, const std::string& error = "") :
        message{"ActionError: " + action.toString()}
    {
        if (error.length() > 0)
        {
            message += ": ";
            message += error;
        }

        // Note: Do not store a reference or pointer to the Action.  It may be
        // destructed (out of scope) before the exception is caught.
    }

    /**
     * Returns the description of this error.
     *
     * @return error description
     */
    const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    /**
     * Message describing this exception.
     */
    std::string message{};
};

} // namespace phosphor::power::regulators
