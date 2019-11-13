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
#include "if_action.hpp"
#include "mock_action.hpp"

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

TEST(IfActionTests, Constructor)
{
    // Test where else clause is not specified
    {
        std::unique_ptr<Action> conditionAction =
            std::make_unique<MockAction>();

        std::vector<std::unique_ptr<Action>> thenActions{};
        thenActions.push_back(std::make_unique<MockAction>());
        thenActions.push_back(std::make_unique<MockAction>());

        IfAction ifAction{std::move(conditionAction), std::move(thenActions)};
        EXPECT_NE(ifAction.getConditionAction().get(), nullptr);
        EXPECT_EQ(ifAction.getThenActions().size(), 2);
        EXPECT_EQ(ifAction.getElseActions().size(), 0);
    }

    // Test where else clause is specified
    {
        std::unique_ptr<Action> conditionAction =
            std::make_unique<MockAction>();

        std::vector<std::unique_ptr<Action>> thenActions{};
        thenActions.push_back(std::make_unique<MockAction>());
        thenActions.push_back(std::make_unique<MockAction>());

        std::vector<std::unique_ptr<Action>> elseActions{};
        elseActions.push_back(std::make_unique<MockAction>());

        IfAction ifAction{std::move(conditionAction), std::move(thenActions),
                          std::move(elseActions)};
        EXPECT_NE(ifAction.getConditionAction().get(), nullptr);
        EXPECT_EQ(ifAction.getThenActions().size(), 2);
        EXPECT_EQ(ifAction.getElseActions().size(), 1);
    }
}

TEST(IfActionTests, Execute)
{
    // Create ActionEnvironment
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};

    // Test where action throws an exception
    try
    {
        // Create condition action that will return true
        std::unique_ptr<MockAction> conditionAction =
            std::make_unique<MockAction>();
        EXPECT_CALL(*conditionAction, execute).Times(1).WillOnce(Return(true));

        // Create vector of actions for then clause
        std::vector<std::unique_ptr<Action>> thenActions{};
        std::unique_ptr<MockAction> thenAction;

        // First then action will throw an exception
        thenAction = std::make_unique<MockAction>();
        EXPECT_CALL(*thenAction, execute)
            .Times(1)
            .WillOnce(Throw(std::logic_error{"Communication error"}));
        thenActions.push_back(std::move(thenAction));

        // Second then action should not get executed
        thenAction = std::make_unique<MockAction>();
        EXPECT_CALL(*thenAction, execute).Times(0);
        thenActions.push_back(std::move(thenAction));

        IfAction ifAction{std::move(conditionAction), std::move(thenActions)};
        ifAction.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& error)
    {
        EXPECT_STREQ(error.what(), "Communication error");
    }

    // Test where condition is true: then clause returns true
    try
    {
        // Create condition action that will return true
        std::unique_ptr<MockAction> conditionAction =
            std::make_unique<MockAction>();
        EXPECT_CALL(*conditionAction, execute).Times(1).WillOnce(Return(true));

        // Create vector of actions for then clause: last action returns true
        std::vector<std::unique_ptr<Action>> thenActions{};
        std::unique_ptr<MockAction> thenAction = std::make_unique<MockAction>();
        EXPECT_CALL(*thenAction, execute).Times(1).WillOnce(Return(true));
        thenActions.push_back(std::move(thenAction));

        // Create vector of actions for else clause: should not be executed
        std::vector<std::unique_ptr<Action>> elseActions{};
        std::unique_ptr<MockAction> elseAction = std::make_unique<MockAction>();
        EXPECT_CALL(*elseAction, execute).Times(0);
        elseActions.push_back(std::move(elseAction));

        IfAction ifAction{std::move(conditionAction), std::move(thenActions),
                          std::move(elseActions)};
        EXPECT_EQ(ifAction.execute(env), true);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where condition is true: then clause returns false
    try
    {
        // Create condition action that will return true
        std::unique_ptr<MockAction> conditionAction =
            std::make_unique<MockAction>();
        EXPECT_CALL(*conditionAction, execute).Times(1).WillOnce(Return(true));

        // Create vector of actions for then clause: last action returns false
        std::vector<std::unique_ptr<Action>> thenActions{};
        std::unique_ptr<MockAction> thenAction = std::make_unique<MockAction>();
        EXPECT_CALL(*thenAction, execute).Times(1).WillOnce(Return(false));
        thenActions.push_back(std::move(thenAction));

        // Create vector of actions for else clause: should not be executed
        std::vector<std::unique_ptr<Action>> elseActions{};
        std::unique_ptr<MockAction> elseAction = std::make_unique<MockAction>();
        EXPECT_CALL(*elseAction, execute).Times(0);
        elseActions.push_back(std::move(elseAction));

        IfAction ifAction{std::move(conditionAction), std::move(thenActions),
                          std::move(elseActions)};
        EXPECT_EQ(ifAction.execute(env), false);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where condition is false: else clause returns true
    try
    {
        // Create condition action that will return false
        std::unique_ptr<MockAction> conditionAction =
            std::make_unique<MockAction>();
        EXPECT_CALL(*conditionAction, execute).Times(1).WillOnce(Return(false));

        // Create vector of actions for then clause: should not be executed
        std::vector<std::unique_ptr<Action>> thenActions{};
        std::unique_ptr<MockAction> thenAction = std::make_unique<MockAction>();
        EXPECT_CALL(*thenAction, execute).Times(0);
        thenActions.push_back(std::move(thenAction));

        // Create vector of actions for else clause: last action returns true
        std::vector<std::unique_ptr<Action>> elseActions{};
        std::unique_ptr<MockAction> elseAction = std::make_unique<MockAction>();
        EXPECT_CALL(*elseAction, execute).Times(1).WillOnce(Return(true));
        elseActions.push_back(std::move(elseAction));

        IfAction ifAction{std::move(conditionAction), std::move(thenActions),
                          std::move(elseActions)};
        EXPECT_EQ(ifAction.execute(env), true);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where condition is false: else clause returns false
    try
    {
        // Create condition action that will return false
        std::unique_ptr<MockAction> conditionAction =
            std::make_unique<MockAction>();
        EXPECT_CALL(*conditionAction, execute).Times(1).WillOnce(Return(false));

        // Create vector of actions for then clause: should not be executed
        std::vector<std::unique_ptr<Action>> thenActions{};
        std::unique_ptr<MockAction> thenAction = std::make_unique<MockAction>();
        EXPECT_CALL(*thenAction, execute).Times(0);
        thenActions.push_back(std::move(thenAction));

        // Create vector of actions for else clause: last action returns false
        std::vector<std::unique_ptr<Action>> elseActions{};
        std::unique_ptr<MockAction> elseAction = std::make_unique<MockAction>();
        EXPECT_CALL(*elseAction, execute).Times(1).WillOnce(Return(false));
        elseActions.push_back(std::move(elseAction));

        IfAction ifAction{std::move(conditionAction), std::move(thenActions),
                          std::move(elseActions)};
        EXPECT_EQ(ifAction.execute(env), false);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where condition is false: no else clause specified
    try
    {
        // Create condition action that will return false
        std::unique_ptr<MockAction> conditionAction =
            std::make_unique<MockAction>();
        EXPECT_CALL(*conditionAction, execute).Times(1).WillOnce(Return(false));

        // Create vector of actions for then clause: should not be executed
        std::vector<std::unique_ptr<Action>> thenActions{};
        std::unique_ptr<MockAction> thenAction = std::make_unique<MockAction>();
        EXPECT_CALL(*thenAction, execute).Times(0);
        thenActions.push_back(std::move(thenAction));

        IfAction ifAction{std::move(conditionAction), std::move(thenActions)};
        EXPECT_EQ(ifAction.execute(env), false);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(IfActionTests, GetConditionAction)
{
    MockAction* conditionAction = new MockAction{};

    std::vector<std::unique_ptr<Action>> thenActions{};

    IfAction ifAction{std::unique_ptr<Action>{conditionAction},
                      std::move(thenActions)};

    EXPECT_EQ(ifAction.getConditionAction().get(), conditionAction);
}

TEST(IfActionTests, GetThenActions)
{
    std::unique_ptr<Action> conditionAction = std::make_unique<MockAction>();

    std::vector<std::unique_ptr<Action>> thenActions{};

    MockAction* thenAction1 = new MockAction{};
    thenActions.push_back(std::unique_ptr<MockAction>{thenAction1});

    MockAction* thenAction2 = new MockAction{};
    thenActions.push_back(std::unique_ptr<MockAction>{thenAction2});

    IfAction ifAction{std::move(conditionAction), std::move(thenActions)};
    EXPECT_EQ(ifAction.getThenActions().size(), 2);
    EXPECT_EQ(ifAction.getThenActions()[0].get(), thenAction1);
    EXPECT_EQ(ifAction.getThenActions()[1].get(), thenAction2);
}

TEST(IfActionTests, GetElseActions)
{
    std::unique_ptr<Action> conditionAction = std::make_unique<MockAction>();

    std::vector<std::unique_ptr<Action>> thenActions{};

    std::vector<std::unique_ptr<Action>> elseActions{};

    MockAction* elseAction1 = new MockAction{};
    elseActions.push_back(std::unique_ptr<MockAction>{elseAction1});

    MockAction* elseAction2 = new MockAction{};
    elseActions.push_back(std::unique_ptr<MockAction>{elseAction2});

    IfAction ifAction{std::move(conditionAction), std::move(thenActions),
                      std::move(elseActions)};
    EXPECT_EQ(ifAction.getElseActions().size(), 2);
    EXPECT_EQ(ifAction.getElseActions()[0].get(), elseAction1);
    EXPECT_EQ(ifAction.getElseActions()[1].get(), elseAction2);
}
