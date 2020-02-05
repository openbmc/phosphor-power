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
#include "and_action.hpp"
#include "id_map.hpp"
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

TEST(AndActionTests, Constructor)
{
    std::vector<std::unique_ptr<Action>> actions{};
    actions.push_back(std::make_unique<MockAction>());
    actions.push_back(std::make_unique<MockAction>());

    AndAction andAction{std::move(actions)};
    EXPECT_EQ(andAction.getActions().size(), 2);
}

TEST(AndActionTests, Execute)
{
    // Create ActionEnvironment
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};

    // Test where empty vector of actions is specified
    try
    {
        std::vector<std::unique_ptr<Action>> actions{};
        AndAction andAction{std::move(actions)};
        EXPECT_EQ(andAction.execute(env), true);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where action throws an exception
    try
    {
        std::vector<std::unique_ptr<Action>> actions{};
        std::unique_ptr<MockAction> action;

        // First action will throw an exception
        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute)
            .Times(1)
            .WillOnce(Throw(std::logic_error{"Communication error"}));
        actions.push_back(std::move(action));

        // Second action should not get executed
        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(0);
        actions.push_back(std::move(action));

        AndAction andAction{std::move(actions)};
        andAction.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& error)
    {
        EXPECT_STREQ(error.what(), "Communication error");
    }

    // Test where middle action returns false
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

        // Third action will return true
        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        actions.push_back(std::move(action));

        AndAction andAction{std::move(actions)};
        EXPECT_EQ(andAction.execute(env), false);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where all actions return true
    try
    {
        std::vector<std::unique_ptr<Action>> actions{};
        std::unique_ptr<MockAction> action;

        // First action will return true
        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        actions.push_back(std::move(action));

        // Second action will return true
        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        actions.push_back(std::move(action));

        // Third action will return true
        action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        actions.push_back(std::move(action));

        AndAction andAction{std::move(actions)};
        EXPECT_EQ(andAction.execute(env), true);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(AndActionTests, GetActions)
{
    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action2});

    AndAction andAction{std::move(actions)};
    EXPECT_EQ(andAction.getActions().size(), 2);
    EXPECT_EQ(andAction.getActions()[0].get(), action1);
    EXPECT_EQ(andAction.getActions()[1].get(), action2);
}

TEST(AndActionTests, ToString)
{
    std::vector<std::unique_ptr<Action>> actions{};
    actions.push_back(std::make_unique<MockAction>());

    AndAction andAction{std::move(actions)};
    EXPECT_EQ(andAction.toString(), "and: [ ... ]");
}
