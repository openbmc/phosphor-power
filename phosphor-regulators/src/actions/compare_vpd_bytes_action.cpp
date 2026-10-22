/**
 * Copyright © 2026 IBM Corporation
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

#include "compare_vpd_bytes_action.hpp"

#include "action_error.hpp"

#include <exception>
#include <ios>
#include <sstream>

namespace phosphor::power::regulators
{

bool CompareVPDBytesAction::execute(ActionEnvironment& environment)
{
    bool isEqual{true};
    try
    {
        // Get actual VPD keyword byte values
        std::vector<uint8_t> actualValues =
            environment.getServices().getVPD().getValue(fru, keyword);

        // Actual and expected number of bytes must be equal
        if (values.size() != actualValues.size())
        {
            isEqual = false;
        }
        else
        {
            // Compare actual byte values to expected byte values
            for (unsigned int i = 0; i < values.size(); ++i)
            {
                if ((actualValues[i] & masks[i]) != values[i])
                {
                    isEqual = false;
                    break;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        // Nest exception within an ActionError so the caller will have both the
        // low level error information and the action information.
        std::throw_with_nested(ActionError(*this));
    }
    return isEqual;
}

std::string CompareVPDBytesAction::toString() const
{
    std::ostringstream ss;
    ss << "compare_vpd_bytes: { ";
    ss << "fru: " << fru << ", ";
    ss << "keyword: " << keyword << ", ";
    ss << "values: [ ";
    ss << std::hex << std::uppercase;
    for (unsigned int i = 0; i < values.size(); ++i)
    {
        ss << ((i > 0) ? ", " : "") << "0x" << static_cast<uint16_t>(values[i]);
    }
    ss << " ], ";
    ss << "masks: [ ";
    for (unsigned int i = 0; i < masks.size(); ++i)
    {
        ss << ((i > 0) ? ", " : "") << "0x" << static_cast<uint16_t>(masks[i]);
    }
    ss << " ] }";
    return ss.str();
}

} // namespace phosphor::power::regulators
