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
#include "chassis.hpp"
#include "device.hpp"
#include "id_map.hpp"
#include "journal.hpp"
#include "mock_journal.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "system.hpp"
#include "test_utils.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::test_utils;

TEST(SystemTests, Constructor)
{
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    rules.emplace_back(createRule("set_voltage_rule"));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(createDevice("reg1", {"rail1"}));
    chassis.emplace_back(std::make_unique<Chassis>(1, std::move(devices)));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getChassis().size(), 1);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_NO_THROW(system.getIDMap().getRule("set_voltage_rule"));
    EXPECT_NO_THROW(system.getIDMap().getDevice("reg1"));
    EXPECT_NO_THROW(system.getIDMap().getRail("rail1"));
    EXPECT_THROW(system.getIDMap().getRail("rail2"), std::invalid_argument);
    EXPECT_EQ(system.getRules().size(), 1);
    EXPECT_EQ(system.getRules()[0]->getID(), "set_voltage_rule");
}

TEST(SystemTests, Configure)
{
    // Specify an empty rules vector
    std::vector<std::unique_ptr<Rule>> rules{};

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));
    chassis.emplace_back(std::make_unique<Chassis>(3));

    // Create System
    System system{std::move(rules), std::move(chassis)};

    // Call configure()
    journal::clear();
    system.configure();
    EXPECT_EQ(journal::getDebugMessages().size(), 0);
    EXPECT_EQ(journal::getErrMessages().size(), 0);
    std::vector<std::string> expectedInfoMessages{"Configuring chassis 1",
                                                  "Configuring chassis 3"};
    EXPECT_EQ(journal::getInfoMessages(), expectedInfoMessages);
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
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    rules.emplace_back(createRule("set_voltage_rule"));
    rules.emplace_back(createRule("read_sensors_rule"));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    {
        // Chassis 1
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("reg1", {"rail1"}));
        devices.emplace_back(createDevice("reg2", {"rail2a", "rail2b"}));
        chassis.emplace_back(std::make_unique<Chassis>(1, std::move(devices)));
    }
    {
        // Chassis 2
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("reg3", {"rail3a", "rail3b"}));
        devices.emplace_back(createDevice("reg4"));
        chassis.emplace_back(std::make_unique<Chassis>(2, std::move(devices)));
    }

    // Create System
    System system{std::move(rules), std::move(chassis)};
    const IDMap& idMap = system.getIDMap();

    // Verify all Rules are in the IDMap
    EXPECT_NO_THROW(idMap.getRule("set_voltage_rule"));
    EXPECT_NO_THROW(idMap.getRule("read_sensors_rule"));
    EXPECT_THROW(idMap.getRule("set_voltage_rule2"), std::invalid_argument);

    // Verify all Devices are in the IDMap
    EXPECT_NO_THROW(idMap.getDevice("reg1"));
    EXPECT_NO_THROW(idMap.getDevice("reg2"));
    EXPECT_NO_THROW(idMap.getDevice("reg3"));
    EXPECT_NO_THROW(idMap.getDevice("reg4"));
    EXPECT_THROW(idMap.getDevice("reg5"), std::invalid_argument);

    // Verify all Rails are in the IDMap
    EXPECT_NO_THROW(idMap.getRail("rail1"));
    EXPECT_NO_THROW(idMap.getRail("rail2a"));
    EXPECT_NO_THROW(idMap.getRail("rail2b"));
    EXPECT_NO_THROW(idMap.getRail("rail3a"));
    EXPECT_NO_THROW(idMap.getRail("rail3b"));
    EXPECT_THROW(idMap.getRail("rail4"), std::invalid_argument);
}

TEST(SystemTests, GetRules)
{
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    rules.emplace_back(createRule("set_voltage_rule"));
    rules.emplace_back(createRule("read_sensors_rule"));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getRules().size(), 2);
    EXPECT_EQ(system.getRules()[0]->getID(), "set_voltage_rule");
    EXPECT_EQ(system.getRules()[1]->getID(), "read_sensors_rule");
}
