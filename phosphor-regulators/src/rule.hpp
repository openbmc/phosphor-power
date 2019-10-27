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

#include <string>

namespace phosphor
{
namespace power
{
namespace regulators
{

using std::string;

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
    /**
     * Constructor.
     *
     * @param id unique rule ID
     */
    Rule(const string& id) : id{id}
    {
    }

    /**
     * Returns the unique ID of this rule.
     *
     * @return rule ID
     */
    const string& getID() const
    {
        return id;
    }

  private:
    /**
     * Unique ID of this rule.
     */
    string id{};
};

} // namespace regulators
} // namespace power
} // namespace phosphor
