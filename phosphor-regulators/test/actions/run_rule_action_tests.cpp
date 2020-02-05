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
#include "action.hpp"
#include "action_environment.hpp"
#include "device.hpp"
#include "id_map.hpp"
#include "mock_action.hpp"
#include "rule.hpp"
#include "run_rule_action.hpp"

#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::Return;
using ::testing::Throw;

TEST(RunRuleActionTests, Constructor)
{
    RunRuleAction action{"set_voltage_rule"};
    EXPECT_EQ(action.getRuleID(), "set_voltage_rule");
}

TEST(RunRuleActionTests, Execute)
{
    // Test where rule ID is not in the IDMap/ActionEnvironment
    try
    {
        IDMap idMap{};
        ActionEnvironment env{idMap, ""};
        RunRuleAction runRuleAction{"set_voltage_rule"};
        runRuleAction.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& ia_error)
    {
        EXPECT_STREQ(ia_error.what(),
                     "Unable to find rule with ID \"set_voltage_rule\"");
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where a rule action throws an exception
    try
    {
        // Create rule with action that throws an exception
        std::vector<std::unique_ptr<Action>> actions{};
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute)
            .Times(1)
            .WillOnce(Throw(std::logic_error{"Communication error"}));
        actions.push_back(std::move(action));
        Rule rule("exception_rule", std::move(actions));

        // Create ActionEnvironment
        IDMap idMap{};
        idMap.addRule(rule);
        ActionEnvironment env{idMap, ""};

        // Create RunRuleAction
        RunRuleAction runRuleAction{"exception_rule"};
        runRuleAction.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& error)
    {
        EXPECT_STREQ(error.what(), "Communication error");
    }

    // Test where rule calls itself and results in infinite recursion
    try
    {
        // Create rule that calls itself
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<RunRuleAction>("infinite_rule"));
        Rule rule("infinite_rule", std::move(actions));

        // Create ActionEnvironment
        IDMap idMap{};
        idMap.addRule(rule);
        ActionEnvironment env{idMap, ""};

        // Create RunRuleAction
        RunRuleAction runRuleAction{"infinite_rule"};
        runRuleAction.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& r_error)
    {
        EXPECT_STREQ(r_error.what(),
                     "Maximum rule depth exceeded by rule infinite_rule.");
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where last action returns false
    try
    {
        // Create rule with two actions.  Last action returns false.
        std::vector<std::unique_ptr<Action>> actions{};
        std::unique_ptr<MockAction> action;

        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        actions.push_back(std::move(action));

        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));
        actions.push_back(std::move(action));

        Rule rule("set_voltage_rule", std::move(actions));

        // Create ActionEnvironment
        IDMap idMap{};
        idMap.addRule(rule);
        ActionEnvironment env{idMap, ""};

        // Create RunRuleAction
        RunRuleAction runRuleAction{"set_voltage_rule"};
        EXPECT_EQ(runRuleAction.execute(env), false);
        EXPECT_EQ(env.getRuleDepth(), 0);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where last action returns true
    try
    {
        // Create rule with two actions.  Last action returns true.
        std::vector<std::unique_ptr<Action>> actions{};
        std::unique_ptr<MockAction> action;

        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));
        actions.push_back(std::move(action));

        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        actions.push_back(std::move(action));

        Rule rule("set_voltage_rule", std::move(actions));

        // Create ActionEnvironment
        IDMap idMap{};
        idMap.addRule(rule);
        ActionEnvironment env{idMap, ""};

        // Create RunRuleAction
        RunRuleAction runRuleAction{"set_voltage_rule"};
        EXPECT_EQ(runRuleAction.execute(env), true);
        EXPECT_EQ(env.getRuleDepth(), 0);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(RunRuleActionTests, GetRuleID)
{
    RunRuleAction action{"read_sensors_rule"};
    EXPECT_EQ(action.getRuleID(), "read_sensors_rule");
}

TEST(RunRuleActionTests, ToString)
{
    RunRuleAction action{"set_voltage_rule"};
    EXPECT_EQ(action.toString(), "run_rule: set_voltage_rule");
}
