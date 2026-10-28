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

#include "compare_vpd_byte_action.hpp"

#include "action_error.hpp"

#include <exception>
#include <ios>
#include <sstream>
#include <vector>

namespace phosphor::power::regulators
{

bool CompareVPDByteAction::execute(ActionEnvironment& environment)
{
    bool isEqual{false};
    try
    {
        // Get actual VPD keyword byte values
        std::vector<uint8_t> actualValues =
            environment.getServices().getVPD().getValue(fru, keyword);

        // Position must be within actual byte values
        if (position < actualValues.size())
        {
            // Modify actual byte value to only include bits specified in mask
            uint8_t actualValue = actualValues[position] & mask;

            // Check if actual byte value equals expected byte value
            isEqual = (actualValue == value);
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

std::string CompareVPDByteAction::toString() const
{
    std::ostringstream ss;
    ss << "compare_vpd_byte: { ";
    ss << "fru: " << fru << ", ";
    ss << "keyword: " << keyword << ", ";
    ss << "position: " << position << ", ";
    ss << "value: 0x" << std::hex << std::uppercase
       << static_cast<uint16_t>(value) << ", ";
    ss << "mask: 0x" << static_cast<uint16_t>(mask) << " }";
    return ss.str();
}

} // namespace phosphor::power::regulators
