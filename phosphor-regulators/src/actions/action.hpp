/**
 * Copyright © 2019 IBM Corporation
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

#include "action_environment.hpp"

namespace phosphor
{
namespace power
{
namespace regulators
{

/**
 * @class Action
 *
 * Action to execute.
 *
 * All regulator actions are derived from this abstract base class.
 *
 * Actions are executed to perform regulator operations, such as configuring a
 * regulator or reading sensor values.
 */
class Action
{
  public:
    // Specify which compiler-generated methods we want
    Action() = default;
    Action(const Action&) = delete;
    Action(Action&&) = delete;
    Action& operator=(const Action&) = delete;
    Action& operator=(Action&&) = delete;
    virtual ~Action() = default;

    /**
     * Executes this action.
     *
     * Throws an exception if an error occurs and the action cannot be
     * successfully executed.
     *
     * @param environment Action execution environment.
     * @return A boolean value whose meaning is defined by the action type.  For
     *         example, CompareByteAction returns true if the actual register
     *         value matches the expected value.  The return value does NOT
     *         indicate if the action was successfully executed.
     */
    virtual bool execute(ActionEnvironment& environment) = 0;
};

} // namespace regulators
} // namespace power
} // namespace phosphor
