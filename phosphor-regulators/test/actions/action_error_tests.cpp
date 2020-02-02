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
#include "action_error.hpp"
#include "run_rule_action.hpp"

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(ActionErrorTests, Constructor)
{
    // Test where only action is specified
    {
        RunRuleAction action{"set_voltage_rule"};
        ActionError error{action};
        EXPECT_STREQ(error.what(), "ActionError: run_rule: set_voltage_rule");
    }

    // Test where action and error message are specified
    {
        RunRuleAction action{"set_voltage_rule"};
        ActionError error{action, "Infinite loop"};
        EXPECT_STREQ(error.what(),
                     "ActionError: run_rule: set_voltage_rule: Infinite loop");
    }
}

TEST(ActionErrorTests, What)
{
    RunRuleAction action{"set_voltage_rule"};
    ActionError error{action, "Invalid rule ID"};
    EXPECT_STREQ(error.what(),
                 "ActionError: run_rule: set_voltage_rule: Invalid rule ID");
}
