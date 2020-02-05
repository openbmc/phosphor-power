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

namespace phosphor::power::regulators
{

/**
 * @class NotAction
 *
 * Executes an action and negates its return value.
 *
 * Implements the "not" action in the JSON config file.
 */
class NotAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    NotAction() = delete;
    NotAction(const NotAction&) = delete;
    NotAction(NotAction&&) = delete;
    NotAction& operator=(const NotAction&) = delete;
    NotAction& operator=(NotAction&&) = delete;
    virtual ~NotAction() = default;

    /**
     * Constructor.
     *
     * @param action action to execute
     */
    explicit NotAction(std::unique_ptr<Action> action) :
        action{std::move(action)}
    {
    }

    /**
     * Executes the action specified in the constructor.
     *
     * Returns the opposite of the return value from the action.  For example,
     * if the action returned true, then false will be returned.
     *
     * Throws an exception if an error occurs and the action cannot be
     * successfully executed.
     *
     * @param environment action execution environment
     * @return negated return value from action executed
     */
    virtual bool execute(ActionEnvironment& environment) override
    {
        return !(action->execute(environment));
    }

    /**
     * Returns the action to execute.
     *
     * @return action
     */
    const std::unique_ptr<Action>& getAction() const
    {
        return action;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override
    {
        return "not: { ... }";
    }

  private:
    /**
     * Action to execute.
     */
    std::unique_ptr<Action> action;
};

} // namespace phosphor::power::regulators
