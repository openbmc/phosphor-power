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
#include "mock_services.hpp"
#include "not_action.hpp"

#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::Return;
using ::testing::Throw;

TEST(NotActionTests, Constructor)
{
    NotAction notAction{std::make_unique<MockAction>()};
    EXPECT_NE(notAction.getAction().get(), nullptr);
}

TEST(NotActionTests, Execute)
{
    // Create ActionEnvironment
    IDMap idMap{};
    MockServices services{};
    ActionEnvironment env{idMap, "", services};

    // Test where negated action throws an exception
    try
    {
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute)
            .Times(1)
            .WillOnce(Throw(std::logic_error{"Communication error"}));

        NotAction notAction{std::move(action)};
        notAction.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& error)
    {
        EXPECT_STREQ(error.what(), "Communication error");
    }

    // Test where negated action returns true
    try
    {
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));

        NotAction notAction{std::move(action)};
        EXPECT_EQ(notAction.execute(env), false);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where negated action returns false
    try
    {
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));

        NotAction notAction{std::move(action)};
        EXPECT_EQ(notAction.execute(env), true);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(NotActionTests, GetAction)
{
    MockAction* action = new MockAction{};
    NotAction notAction{std::unique_ptr<Action>{action}};
    EXPECT_EQ(notAction.getAction().get(), action);
}

TEST(NotActionTests, ToString)
{
    NotAction notAction{std::make_unique<MockAction>()};
    EXPECT_EQ(notAction.toString(), "not: { ... }");
}
