/**
 * Copyright © 2020 IBM Corporation
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

#include <string>

namespace phosphor::power::regulators
{

/**
 * @class ComparePresenceAction
 *
 * Compares a hardware component's presence to an expected value.
 *
 * Implements the compare_presence action in the JSON config file.
 */
class ComparePresenceAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    ComparePresenceAction() = delete;
    ComparePresenceAction(const ComparePresenceAction&) = delete;
    ComparePresenceAction(ComparePresenceAction&&) = delete;
    ComparePresenceAction& operator=(const ComparePresenceAction&) = delete;
    ComparePresenceAction& operator=(ComparePresenceAction&&) = delete;
    virtual ~ComparePresenceAction() = default;

    /**
     * Constructor.
     *
     * @param fru Field-Replaceable Unit (FRU)
     * @param value Expected presence value
     */
    explicit ComparePresenceAction(const std::string& fru, bool value) :
        fru{fru}, value{value}
    {
    }

    /**
     * Executes this action.
     *
     * TODO: Not implemented yet
     *
     * @param environment Action execution environment.
     * @return true
     */
    virtual bool execute(ActionEnvironment& /* environment */) override
    {
        // TODO: Not implemented yet
        return true;
    }

    /**
     * Returns the FRU.
     *
     * @return FRU
     */
    const std::string& getFRU() const
    {
        return fru;
    }

    /**
     * Returns the value.
     *
     * @return value
     */
    bool getValue() const
    {
        return value;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override
    {
        // TODO: Not implemented yet
        return "compare_presence_action: {}";
    }

  private:
    /**
     * Field-Replaceable Unit (FRU) for this action.
     *
     * Specify the D-Bus inventory path of the FRU.
     */
    const std::string fru{};

    /**
     * Expected presence value.
     */
    const bool value{};
};

} // namespace phosphor::power::regulators
