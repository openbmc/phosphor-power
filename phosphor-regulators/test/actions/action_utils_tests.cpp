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
#include "action_utils.hpp"
#include "id_map.hpp"
#include "stub_action.hpp"

#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(ActionUtilsTests, Execute)
{
    // Create ActionEnvironment
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};

    // Test where vector is empty
    std::vector<std::unique_ptr<Action>> actions{};
    try
    {
        EXPECT_EQ(action_utils::execute(actions, env), true);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Create two actions.  Retain raw pointers to them so we can call
    // StubAction-specific methods.
    StubAction* action1 = new StubAction(true);
    StubAction* action2 = new StubAction(true);

    // Add actions to vector.  Vector now owns action objects.
    actions.push_back(std::unique_ptr<StubAction>{action1});
    actions.push_back(std::unique_ptr<StubAction>{action2});

    // Test where first action throws an exception
    try
    {
        action1->setException(
            std::make_unique<std::logic_error>("Communication error"));
        action_utils::execute(actions, env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& error)
    {
        EXPECT_STREQ(error.what(), "Communication error");

        EXPECT_EQ(action1->getExecuteCount(), 1);
        EXPECT_EQ(action1->getExceptionCount(), 1);

        EXPECT_EQ(action2->getExecuteCount(), 0);
        EXPECT_EQ(action2->getExceptionCount(), 0);
    }

    // Test where last action returns false
    try
    {
        action1->clear();
        action1->setReturnValue(true);

        action2->clear();
        action2->setReturnValue(false);

        EXPECT_EQ(action_utils::execute(actions, env), false);
        EXPECT_EQ(action1->getExecuteCount(), 1);
        EXPECT_EQ(action2->getExecuteCount(), 1);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where last action returns true
    try
    {
        action1->clear();
        action1->setReturnValue(false);

        action2->clear();
        action2->setReturnValue(true);

        EXPECT_EQ(action_utils::execute(actions, env), true);
        EXPECT_EQ(action1->getExecuteCount(), 1);
        EXPECT_EQ(action2->getExecuteCount(), 1);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}
