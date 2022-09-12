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
#pragma once

#include "error_history.hpp"

#include <string>

namespace phosphor::power::regulators
{

/**
 * Redundant phase fault type.
 *
 * A voltage regulator is sometimes called a "phase controller" because it
 * controls one or more phases that perform the actual voltage regulation.
 *
 * A regulator may have redundant phases.  If a redundant phase fails, the
 * regulator will continue to provide the desired output voltage.  However, a
 * phase fault error should be logged warning the user that the regulator has
 * lost redundancy.
 */
enum class PhaseFaultType : unsigned char
{
    /**
     * N phase fault type.
     *
     * Regulator has lost all redundant phases.  The regulator is now at
     * redundancy level N.
     */
    n,

    /**
     * N+1 phase fault type.
     *
     * An "N+2" regulator has lost one redundant phase.  The regulator is now at
     * redundancy level "N+1".
     */
    n_plus_1
};

/**
 * Returns the ErrorType that corresponds to the specified PhaseFaultType.
 *
 * The ErrorType enum is used with the ErrorHistory class.
 *
 * @param type phase fault type
 * @return error type
 */
inline ErrorType toErrorType(PhaseFaultType type)
{
    ErrorType errorType{ErrorType::phaseFaultN};
    switch (type)
    {
        case PhaseFaultType::n:
            errorType = ErrorType::phaseFaultN;
            break;
        case PhaseFaultType::n_plus_1:
            errorType = ErrorType::phaseFaultNPlus1;
            break;
    }
    return errorType;
}

/**
 * Returns the name of the specified PhaseFaultType.
 *
 * @param type phase fault type
 * @return phase fault type name
 */
inline std::string toString(PhaseFaultType type)
{
    std::string name{};
    switch (type)
    {
        case PhaseFaultType::n:
            name = "n";
            break;
        case PhaseFaultType::n_plus_1:
            name = "n+1";
            break;
    }
    return name;
}

} // namespace phosphor::power::regulators
