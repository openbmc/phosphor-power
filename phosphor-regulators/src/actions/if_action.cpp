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

#include "if_action.hpp"

#include "action_utils.hpp"

namespace phosphor::power::regulators
{

bool IfAction::execute(ActionEnvironment& environment)
{
    bool returnValue{true};

    // Execute condition action and check whether it returned true
    if (conditionAction->execute(environment) == true)
    {
        // Condition was true; execute actions in "then" clause
        returnValue = action_utils::execute(thenActions, environment);
    }
    else
    {
        // Condition was false; check if optional "else" clause was specified
        if (elseActions.size() > 0)
        {
            // Execute actions in "else" clause
            returnValue = action_utils::execute(elseActions, environment);
        }
        else
        {
            // No "else" clause specified; return value is false in this case
            returnValue = false;
        }
    }

    return returnValue;
}

} // namespace phosphor::power::regulators
