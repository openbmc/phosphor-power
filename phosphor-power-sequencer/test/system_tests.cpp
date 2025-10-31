/**
 * Copyright © 2025 IBM Corporation
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
#include "power_sequencer_device.hpp"
#include "system.hpp"

#include <stddef.h> // for size_t

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

/**
 * Creates a Chassis object.
 *
 * @param number Chassis number within the system. Must be >= 1.
 * @param inventoryPath D-Bus inventory path of the chassis
 * @return Chassis object
 */
std::unique_ptr<Chassis> createChassis(size_t number,
                                       const std::string& inventoryPath)
{
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    return std::make_unique<Chassis>(number, inventoryPath,
                                     std::move(powerSequencers));
}

TEST(SystemTests, Constructor)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis"));
    System system{std::move(chassis)};

    EXPECT_EQ(system.getChassis().size(), 1);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_EQ(system.getChassis()[0]->getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis");
}

TEST(SystemTests, GetChassis)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    chassis.emplace_back(
        createChassis(3, "/xyz/openbmc_project/inventory/system/chassis_3"));
    chassis.emplace_back(
        createChassis(7, "/xyz/openbmc_project/inventory/system/chassis7"));
    System system{std::move(chassis)};

    EXPECT_EQ(system.getChassis().size(), 3);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_EQ(system.getChassis()[0]->getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis1");
    EXPECT_EQ(system.getChassis()[1]->getNumber(), 3);
    EXPECT_EQ(system.getChassis()[1]->getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis_3");
    EXPECT_EQ(system.getChassis()[2]->getNumber(), 7);
    EXPECT_EQ(system.getChassis()[2]->getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis7");
}
