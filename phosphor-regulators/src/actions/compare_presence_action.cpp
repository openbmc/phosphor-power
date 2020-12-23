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

#include "compare_presence_action.hpp"

#include "action_error.hpp"

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool ComparePresenceAction::execute(ActionEnvironment& environment)
{
    bool isEqual{false};
    try
    {
        // Get actual presence value for FRU.
        bool isPresent =
            environment.getServices().getPresenceService().isPresent(fru);

        // Check if actual presence value equals expected presence value.
        isEqual = (isPresent == value);
    }
    catch (const std::exception& e)
    {
        // Nest exception within an ActionError so the caller will have both the
        // low level error information and the action information.
        std::throw_with_nested(ActionError(*this));
    }
    return isEqual;
}

std::string ComparePresenceAction::toString() const
{
    std::ostringstream ss;
    ss << "compare_presence: { ";
    ss << "fru: " << fru << ", ";
    ss << "value: " << std::boolalpha << value << " }";

    return ss.str();
}

} // namespace phosphor::power::regulators
