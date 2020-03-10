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
#include "configuration.hpp"
#include "mock_action.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(ConfigurationTests, Constructor)
{
    // Test where volts value specified
    {
        std::optional<double> volts{1.3};

        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());

        Configuration configuration(volts, std::move(actions));
        EXPECT_EQ(configuration.getVolts().has_value(), true);
        EXPECT_EQ(configuration.getVolts().value(), 1.3);
        EXPECT_EQ(configuration.getActions().size(), 2);
    }

    // Test where volts value not specified
    {
        std::optional<double> volts{};

        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        Configuration configuration(volts, std::move(actions));
        EXPECT_EQ(configuration.getVolts().has_value(), false);
        EXPECT_EQ(configuration.getActions().size(), 1);
    }
}

TEST(ConfigurationTests, Execute)
{
    // TODO: Implement test when execute() function is done
}

TEST(ConfigurationTests, GetActions)
{
    std::optional<double> volts{1.3};

    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action2});

    Configuration configuration(volts, std::move(actions));
    EXPECT_EQ(configuration.getActions().size(), 2);
    EXPECT_EQ(configuration.getActions()[0].get(), action1);
    EXPECT_EQ(configuration.getActions()[1].get(), action2);
}

TEST(ConfigurationTests, GetVolts)
{
    // Test where volts value specified
    {
        std::optional<double> volts{3.2};

        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        Configuration configuration(volts, std::move(actions));
        EXPECT_EQ(configuration.getVolts().has_value(), true);
        EXPECT_EQ(configuration.getVolts().value(), 3.2);
    }

    // Test where volts value not specified
    {
        std::optional<double> volts{};

        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        Configuration configuration(volts, std::move(actions));
        EXPECT_EQ(configuration.getVolts().has_value(), false);
    }
}
