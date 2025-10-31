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
#include "mock_services.hpp"
#include "power_sequencer_device.hpp"
#include "rail.hpp"
#include "services.hpp"
#include "ucd90160_device.hpp"

#include <stddef.h> // for size_t

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

/**
 * Creates a PowerSequencerDevice instance.
 *
 * PowerSequencerDevice is an abstract base class. The actual object type
 * created is a UCD90160Device.
 *
 * @param bus I2C bus for the device
 * @param address I2C address for the device
 * @param services System services like hardware presence and the journal
 * @return PowerSequencerDevice instance
 */
std::unique_ptr<PowerSequencerDevice> createPowerSequencer(
    uint8_t bus, uint16_t address, Services& services)
{
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails;
    return std::make_unique<UCD90160Device>(
        bus, address, powerControlGPIOName, powerGoodGPIOName, std::move(rails),
        services);
}

TEST(ChassisTests, Constructor)
{
    MockServices services;

    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    powerSequencers.emplace_back(createPowerSequencer(3, 0x70, services));
    Chassis chassis{number, inventoryPath, std::move(powerSequencers)};

    EXPECT_EQ(chassis.getNumber(), number);
    EXPECT_EQ(chassis.getInventoryPath(), inventoryPath);
    EXPECT_EQ(chassis.getPowerSequencers().size(), 1);
    EXPECT_EQ(chassis.getPowerSequencers()[0]->getBus(), 3);
    EXPECT_EQ(chassis.getPowerSequencers()[0]->getAddress(), 0x70);
}

TEST(ChassisTests, GetNumber)
{
    MockServices services;

    size_t number{2};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis2"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers)};

    EXPECT_EQ(chassis.getNumber(), number);
}

TEST(ChassisTests, GetInventoryPath)
{
    MockServices services;

    size_t number{3};
    std::string inventoryPath{
        "/xyz/openbmc_project/inventory/system/chassis_3"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers)};

    EXPECT_EQ(chassis.getInventoryPath(), inventoryPath);
}

TEST(ChassisTests, GetPowerSequencers)
{
    MockServices services;

    size_t number{2};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis2"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    powerSequencers.emplace_back(createPowerSequencer(3, 0x70, services));
    powerSequencers.emplace_back(createPowerSequencer(4, 0x32, services));
    powerSequencers.emplace_back(createPowerSequencer(10, 0x16, services));
    Chassis chassis{number, inventoryPath, std::move(powerSequencers)};

    EXPECT_EQ(chassis.getPowerSequencers().size(), 3);
    EXPECT_EQ(chassis.getPowerSequencers()[0]->getBus(), 3);
    EXPECT_EQ(chassis.getPowerSequencers()[0]->getAddress(), 0x70);
    EXPECT_EQ(chassis.getPowerSequencers()[1]->getBus(), 4);
    EXPECT_EQ(chassis.getPowerSequencers()[1]->getAddress(), 0x32);
    EXPECT_EQ(chassis.getPowerSequencers()[2]->getBus(), 10);
    EXPECT_EQ(chassis.getPowerSequencers()[2]->getAddress(), 0x16);
}
