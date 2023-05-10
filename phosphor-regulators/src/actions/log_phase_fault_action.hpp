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

#include "action.hpp"
#include "action_environment.hpp"
#include "phase_fault.hpp"

#include <string>

namespace phosphor::power::regulators
{

/**
 * @class LogPhaseFaultAction
 *
 * Logs a redundant phase fault error for a voltage regulator.
 *
 * Implements the log_phase_fault action in the JSON config file.
 */
class LogPhaseFaultAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    LogPhaseFaultAction() = delete;
    LogPhaseFaultAction(const LogPhaseFaultAction&) = delete;
    LogPhaseFaultAction(LogPhaseFaultAction&&) = delete;
    LogPhaseFaultAction& operator=(const LogPhaseFaultAction&) = delete;
    LogPhaseFaultAction& operator=(LogPhaseFaultAction&&) = delete;
    virtual ~LogPhaseFaultAction() = default;

    /**
     * Constructor.
     *
     * @param type phase fault type
     */
    explicit LogPhaseFaultAction(PhaseFaultType type) : type{type} {}

    /**
     * Executes this action.
     *
     * Adds the phase fault to the set that have been detected.
     *
     * @param environment action execution environment
     * @return true
     */
    virtual bool execute(ActionEnvironment& environment) override
    {
        environment.addPhaseFault(type);
        return true;
    }

    /**
     * Returns the phase fault type.
     *
     * @return phase fault type
     */
    PhaseFaultType getType() const
    {
        return type;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override
    {
        return "log_phase_fault: { type: " + regulators::toString(type) + " }";
    }

  private:
    /**
     * Phase fault type.
     */
    const PhaseFaultType type;
};

} // namespace phosphor::power::regulators
