/**
 * Copyright © 2021 IBM Corporation
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

#include "compare_vpd_action.hpp"

#include "action_error.hpp"

#include <exception>
#include <sstream>

namespace phosphor::power::regulators
{

bool CompareVPDAction::execute(ActionEnvironment& environment)
{
    bool isEqual{false};
    try
    {
        // Get actual VPD keyword value
        std::string actualValue =
            environment.getServices().getVPD().getValue(fru, keyword);

        // Check if actual value equals expected value
        isEqual = (actualValue == value);
    }
    catch (const std::exception& e)
    {
        // Nest exception within an ActionError so the caller will have both the
        // low level error information and the action information.
        std::throw_with_nested(ActionError(*this));
    }
    return isEqual;
}

std::string CompareVPDAction::toString() const
{
    std::ostringstream ss;
    ss << "compare_vpd: { ";
    ss << "fru: " << fru << ", ";
    ss << "keyword: " << keyword << ", ";
    ss << "value: " << value << " }";
    return ss.str();
}

} // namespace phosphor::power::regulators
