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
#include "id_map.hpp"
#include "mock_action.hpp"
#include "rule.hpp"

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

TEST(RuleTests, Constructor)
{
    // Build vector of actions
    std::vector<std::unique_ptr<Action>> actions{};
    actions.push_back(std::make_unique<MockAction>());
    actions.push_back(std::make_unique<MockAction>());

    // Create rule and verify data members
    Rule rule("set_voltage_rule", std::move(actions));
    EXPECT_EQ(rule.getID(), "set_voltage_rule");
    EXPECT_EQ(rule.getActions().size(), 2);
}

TEST(RuleTests, Execute)
{
    // Create ActionEnvironment
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};

    // Test where an action throws an exception
    try
    {
        std::vector<std::unique_ptr<Action>> actions{};
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute)
            .Times(1)
            .WillOnce(Throw(std::logic_error{"Communication error"}));
        actions.push_back(std::move(action));

        Rule rule("set_voltage_rule", std::move(actions));
        rule.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& error)
    {
        EXPECT_STREQ(error.what(), "Communication error");
    }

    // Test where all actions are executed
    try
    {
        std::vector<std::unique_ptr<Action>> actions{};
        std::unique_ptr<MockAction> action;

        // First action will return true
        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        actions.push_back(std::move(action));

        // Second action will return false
        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));
        actions.push_back(std::move(action));

        Rule rule("set_voltage_rule", std::move(actions));
        EXPECT_EQ(rule.execute(env), false);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(RuleTests, GetActions)
{
    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action2});

    Rule rule("set_voltage_rule", std::move(actions));
    EXPECT_EQ(rule.getActions().size(), 2);
    EXPECT_EQ(rule.getActions()[0].get(), action1);
    EXPECT_EQ(rule.getActions()[1].get(), action2);
}

TEST(RuleTests, GetID)
{
    Rule rule("read_sensor_values", std::vector<std::unique_ptr<Action>>{});
    EXPECT_EQ(rule.getID(), "read_sensor_values");
}
