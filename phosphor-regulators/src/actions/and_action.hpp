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
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class AndAction
 *
 * Executes a sequence of actions and tests whether all of them returned true.
 *
 * Implements the "and" action in the JSON config file.
 */
class AndAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    AndAction() = delete;
    AndAction(const AndAction&) = delete;
    AndAction(AndAction&&) = delete;
    AndAction& operator=(const AndAction&) = delete;
    AndAction& operator=(AndAction&&) = delete;
    virtual ~AndAction() = default;

    /**
     * Constructor.
     *
     * @param actions actions to execute
     */
    explicit AndAction(std::vector<std::unique_ptr<Action>> actions) :
        actions{std::move(actions)}
    {}

    /**
     * Executes the actions specified in the constructor.
     *
     * Returns true if all of the actions returned true, otherwise returns
     * false.
     *
     * Note: All of the actions will be executed even if an action before the
     * end returns false.  This ensures that actions with beneficial
     * side-effects are always executed, such as a register read that clears
     * latched fault bits.
     *
     * Throws an exception if an error occurs and an action cannot be
     * successfully executed.
     *
     * @param environment action execution environment
     * @return true if all actions returned true, otherwise returns false
     */
    virtual bool execute(ActionEnvironment& environment) override
    {
        bool returnValue{true};
        for (std::unique_ptr<Action>& action : actions)
        {
            if (action->execute(environment) == false)
            {
                returnValue = false;
            }
        }
        return returnValue;
    }

    /**
     * Returns the actions to execute.
     *
     * @return actions to execute
     */
    const std::vector<std::unique_ptr<Action>>& getActions() const
    {
        return actions;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override
    {
        return "and: [ ... ]";
    }

  private:
    /**
     * Actions to execute.
     */
    std::vector<std::unique_ptr<Action>> actions{};
};

} // namespace phosphor::power::regulators
