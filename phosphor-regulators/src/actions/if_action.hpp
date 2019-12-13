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
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class IfAction
 *
 * Performs actions based on whether a condition is true.
 *
 * Implements the "if" action in the JSON config file.  The "if" action provides
 * a standard if/then/else structure within the JSON config file.
 *
 * The "if" action contains three parts:
 *   - condition
 *   - then clause
 *   - else clause (optional)
 *
 * The condition is a single action.  The action is executed to determine if the
 * condition is true.
 *
 * If the condition is true, the actions in the "then" clause are executed.
 *
 * If the condition is false, the actions in the "else" clause are executed (if
 * specified).
 */
class IfAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    IfAction() = delete;
    IfAction(const IfAction&) = delete;
    IfAction(IfAction&&) = delete;
    IfAction& operator=(const IfAction&) = delete;
    IfAction& operator=(IfAction&&) = delete;
    virtual ~IfAction() = default;

    /**
     * Constructor.
     *
     * @param conditionAction action that tests whether condition is true
     * @param thenActions actions to perform if condition is true
     * @param elseActions actions to perform if condition is false (optional)
     */
    explicit IfAction(std::unique_ptr<Action> conditionAction,
                      std::vector<std::unique_ptr<Action>> thenActions,
                      std::vector<std::unique_ptr<Action>> elseActions =
                          std::vector<std::unique_ptr<Action>>{}) :
        conditionAction{std::move(conditionAction)},
        thenActions{std::move(thenActions)}, elseActions{std::move(elseActions)}
    {}

    /**
     * Executes the condition action specified in the constructor.
     *
     * If the condition action returns true, the actions in the "then" clause
     * will be executed.  Returns the return value of the last action in the
     * "then" clause.
     *
     * If the condition action returns false, the actions in the "else" clause
     * will be executed.  Returns the return value of the last action in the
     * "else" clause.  If no "else" clause was specified, returns false.
     *
     * Throws an exception if an error occurs and an action cannot be
     * successfully executed.
     *
     * @param environment action execution environment
     * @return return value from last action in "then" or "else" clause
     */
    virtual bool execute(ActionEnvironment& environment) override;

    /**
     * Returns the action that tests whether the condition is true.
     *
     * @return condition action
     */
    const std::unique_ptr<Action>& getConditionAction() const
    {
        return conditionAction;
    }

    /**
     * Returns the actions in the "then" clause.
     *
     * These actions are executed if the condition is true.
     *
     * @return then clause actions
     */
    const std::vector<std::unique_ptr<Action>>& getThenActions() const
    {
        return thenActions;
    }

    /**
     * Returns the actions in the "else" clause.
     *
     * These actions are executed if the condition is false.
     *
     * @return else clause actions
     */
    const std::vector<std::unique_ptr<Action>>& getElseActions() const
    {
        return elseActions;
    }

  private:
    /**
     * Action that tests whether the condition is true.
     */
    std::unique_ptr<Action> conditionAction{};

    /**
     * Actions in the "then" clause.  Executed if condition is true.
     */
    std::vector<std::unique_ptr<Action>> thenActions{};

    /**
     * Actions in the "else" clause.  Executed if condition is false.  Optional.
     */
    std::vector<std::unique_ptr<Action>> elseActions{};
};

} // namespace phosphor::power::regulators
