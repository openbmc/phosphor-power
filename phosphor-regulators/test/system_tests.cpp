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
#include "chassis.hpp"
#include "mock_action.hpp"
#include "rule.hpp"
#include "system.hpp"

#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(SystemTests, Constructor)
{
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::make_unique<MockAction>());
    rules.emplace_back(
        std::make_unique<Rule>("set_voltage_rule", std::move(actions)));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getChassis().size(), 1);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    // TODO: Add tests for IDMap once code to populate it is implemented
    EXPECT_EQ(system.getRules().size(), 1);
    EXPECT_EQ(system.getRules()[0]->getID(), "set_voltage_rule");
}

TEST(SystemTests, GetChassis)
{
    // Specify an empty rules vector
    std::vector<std::unique_ptr<Rule>> rules{};

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));
    chassis.emplace_back(std::make_unique<Chassis>(3));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getChassis().size(), 2);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_EQ(system.getChassis()[1]->getNumber(), 3);
}

TEST(SystemTests, GetIDMap)
{
    // TODO: Code to build IDMap is not implemented yet
}

TEST(SystemTests, GetRules)
{
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::make_unique<MockAction>());
    rules.emplace_back(
        std::make_unique<Rule>("set_voltage_rule", std::move(actions)));
    actions.emplace_back(std::make_unique<MockAction>());
    rules.emplace_back(
        std::make_unique<Rule>("read_sensors_rule", std::move(actions)));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getRules().size(), 2);
    EXPECT_EQ(system.getRules()[0]->getID(), "set_voltage_rule");
    EXPECT_EQ(system.getRules()[1]->getID(), "read_sensors_rule");
}
