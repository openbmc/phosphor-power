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
#include "action_utils.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class Rule
 *
 * A rule is a sequence of actions that can be shared by multiple voltage
 * regulators.
 *
 * Rules define a standard way to perform an operation.  For example, the
 * following action sequences might be sharable using a rule:
 * - Actions that set the output voltage of a regulator rail
 * - Actions that read all the sensors of a regulator rail
 * - Actions that detect down-level hardware using version registers
 */
class Rule
{
  public:
    // Specify which compiler-generated methods we want
    Rule() = delete;
    Rule(const Rule&) = delete;
    Rule(Rule&&) = delete;
    Rule& operator=(const Rule&) = delete;
    Rule& operator=(Rule&&) = delete;
    ~Rule() = default;

    /**
     * Constructor.
     *
     * @param id unique rule ID
     * @param actions actions in the rule
     */
    explicit Rule(const std::string& id,
                  std::vector<std::unique_ptr<Action>> actions) :
        id{id}, actions{std::move(actions)}
    {}

    /**
     * Executes the actions in this rule.
     *
     * Returns the return value from the last action.
     *
     * Throws an exception if an error occurs and an action cannot be
     * successfully executed.
     *
     * @param environment action execution environment
     * @return return value from last action in rule
     */
    bool execute(ActionEnvironment& environment)
    {
        return action_utils::execute(actions, environment);
    }

    /**
     * Returns the actions in this rule.
     *
     * @return actions in rule
     */
    const std::vector<std::unique_ptr<Action>>& getActions() const
    {
        return actions;
    }

    /**
     * Returns the unique ID of this rule.
     *
     * @return rule ID
     */
    const std::string& getID() const
    {
        return id;
    }

  private:
    /**
     * Unique ID of this rule.
     */
    const std::string id{};

    /**
     * Actions in this rule.
     */
    std::vector<std::unique_ptr<Action>> actions{};
};

} // namespace phosphor::power::regulators
