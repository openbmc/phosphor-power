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

#include "action.hpp"
#include "action_environment.hpp"

#include <memory>
#include <vector>

namespace phosphor::power::regulators::action_utils
{

/**
 * This file contains utility functions related to the regulators action
 * framework.
 */

/**
 * Executes one or more actions in sequential order.
 *
 * Returns the return value from the last action in the vector.
 *
 * Throws an exception if an error occurs and an action cannot be
 * successfully executed.
 *
 * @param actions actions to execute
 * @param environment action execution environment
 * @return return value from last action in vector
 */
inline bool execute(std::vector<std::unique_ptr<Action>>& actions,
                    ActionEnvironment& environment)
{
    bool returnValue{true};
    for (std::unique_ptr<Action>& action : actions)
    {
        returnValue = action->execute(environment);
    }
    return returnValue;
}

} // namespace phosphor::power::regulators::action_utils
