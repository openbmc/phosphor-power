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
#include "rule.hpp"

#include <string>

namespace phosphor::power::regulators
{

/**
 * @class RunRuleAction
 *
 * Runs the specified rule.
 *
 * Implements the run_rule action in the JSON config file.
 */
class RunRuleAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    RunRuleAction() = delete;
    RunRuleAction(const RunRuleAction&) = delete;
    RunRuleAction(RunRuleAction&&) = delete;
    RunRuleAction& operator=(const RunRuleAction&) = delete;
    RunRuleAction& operator=(RunRuleAction&&) = delete;
    virtual ~RunRuleAction() = default;

    /**
     * Constructor.
     *
     * @param ruleID rule ID
     */
    explicit RunRuleAction(const std::string& ruleID) : ruleID{ruleID} {}

    /**
     * Executes this action.
     *
     * Runs the rule specified in the constructor.
     *
     * Returns the return value from the last action in the rule.
     *
     * Throws an exception if an error occurs and an action cannot be
     * successfully executed.
     *
     * @param environment action execution environment
     * @return return value from last action in rule
     */
    virtual bool execute(ActionEnvironment& environment) override
    {
        // Increment rule call stack depth since we are running a rule.  Rule
        // depth is used to detect infinite recursion.
        environment.incrementRuleDepth(ruleID);

        // Execute rule
        bool returnValue = environment.getRule(ruleID).execute(environment);

        // Decrement rule depth since rule has returned
        environment.decrementRuleDepth();

        return returnValue;
    }

    /**
     * Returns the rule ID.
     *
     * @return rule ID
     */
    const std::string& getRuleID() const
    {
        return ruleID;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override
    {
        return "run_rule: " + ruleID;
    }

  private:
    /**
     * Rule ID.
     */
    const std::string ruleID{};
};

} // namespace phosphor::power::regulators
