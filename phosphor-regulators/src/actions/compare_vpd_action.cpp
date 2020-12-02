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

#include "compare_vpd_action.hpp"

#include "action_error.hpp"

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool CompareVPDAction::execute(ActionEnvironment& environment)
{
    bool isEqual{false};
    try
    {
        // Read keyword value of fru.
        std::string fruValue =
            environment.getServices().getVPDService().getValue(fru, keyword);

        // Check if actual keyword value equals expected keyword value
        isEqual = (fruValue == value);
    }
    catch (const std::exception& e)
    {
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
