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
#include "i2c_interface.hpp"
#include "test_utils.hpp"

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::test_utils;

TEST(ChassisTests, Constructor)
{
    // Test where works: Only required parameters are specified
    {
        Chassis chassis{2};
        EXPECT_EQ(chassis.getNumber(), 2);
        EXPECT_EQ(chassis.getDevices().size(), 0);
    }

    // Test where works: All parameters are specified
    {
        // Create vector of Device objects
        std::vector<std::unique_ptr<Device>> devices{};
        devices.push_back(std::make_unique<Device>(
            "vdd_reg1", true, "/system/chassis/motherboard/reg1",
            std::move(createI2CInterface())));
        devices.push_back(std::make_unique<Device>(
            "vdd_reg2", true, "/system/chassis/motherboard/reg2",
            std::move(createI2CInterface())));

        // Create Chassis
        Chassis chassis{1, std::move(devices)};
        EXPECT_EQ(chassis.getNumber(), 1);
        EXPECT_EQ(chassis.getDevices().size(), 2);
    }

    // Test where fails: Invalid chassis number < 1
    try
    {
        Chassis chassis{0};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid chassis number: 0");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(ChassisTests, GetDevices)
{
    // Test where no devices were specified in constructor
    {
        Chassis chassis{2};
        EXPECT_EQ(chassis.getDevices().size(), 0);
    }

    // Test where devices were specified in constructor
    {
        // Create vector of Device objects
        std::vector<std::unique_ptr<Device>> devices{};
        devices.push_back(std::make_unique<Device>(
            "vdd_reg1", true, "/system/chassis/motherboard/reg1",
            std::move(createI2CInterface())));
        devices.push_back(std::make_unique<Device>(
            "vdd_reg2", true, "/system/chassis/motherboard/reg2",
            std::move(createI2CInterface())));

        // Create Chassis
        Chassis chassis{1, std::move(devices)};
        EXPECT_EQ(chassis.getDevices().size(), 2);
        EXPECT_EQ(chassis.getDevices()[0]->getID(), "vdd_reg1");
        EXPECT_EQ(chassis.getDevices()[1]->getID(), "vdd_reg2");
    }
}

TEST(ChassisTests, GetNumber)
{
    Chassis chassis{3};
    EXPECT_EQ(chassis.getNumber(), 3);
}
