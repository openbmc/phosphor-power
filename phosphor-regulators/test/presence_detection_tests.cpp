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
#include "action.hpp"
#include "mock_action.hpp"
#include "presence_detection.hpp"

#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(PresenceDetectionTests, Constructor)
{
    std::vector<std::unique_ptr<Action>> actions{};
    actions.push_back(std::make_unique<MockAction>());

    PresenceDetection presenceDetection(std::move(actions));
    EXPECT_EQ(presenceDetection.getActions().size(), 1);
}

TEST(PresenceDetectionTests, Execute)
{
    // TODO: Implement test when execute() function is done
}

TEST(PresenceDetectionTests, GetActions)
{
    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action2});

    PresenceDetection presenceDetection(std::move(actions));
    EXPECT_EQ(presenceDetection.getActions().size(), 2);
    EXPECT_EQ(presenceDetection.getActions()[0].get(), action1);
    EXPECT_EQ(presenceDetection.getActions()[1].get(), action2);
}
