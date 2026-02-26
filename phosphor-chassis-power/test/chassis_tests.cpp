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

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::chassis;
/*using namespace phosphor::power::chassis::test_utils;*/

using ::testing::A;
using ::testing::Return;
using ::testing::Throw;
using ::testing::TypedEq;

class ChassisTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the System object needed for calling some Chassis methods.
     */
    ChassisTests() : ::testing::Test{} {}

  protected:
};

TEST_F(ChassisTests, Constructor)
{
    // Test where works: Only required parameters are specified
    {
        Chassis chassis{2};
        EXPECT_EQ(chassis.getNumber(), 2);
    }

    // Test where works: All parameters are specified
    {
        // Create vector of Device objects
        std::vector<std::unique_ptr<Device>> devices{};

        devices.emplace_back(std::make_unique<Device>(
            "DeviceName_1", "DeviceDirection_1", "DevicePolarity_1"));

        devices.emplace_back(std::make_unique<Device>(
            "DeviceName_2", "DeviceDirection_2", "DevicePolarity_2"));

        // Create Chassis
        Chassis chassis{1, "/dev/i2c-359", std::move(devices)};
        EXPECT_EQ(chassis.getNumber(), 1);
        EXPECT_EQ(chassis.getPresencePath(), "/dev/i2c-359");
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

TEST_F(ChassisTests, GetNumber)
{
    // Test where only required parameter (number) is specified
    {
        Chassis chassis{1};
        EXPECT_EQ(chassis.getNumber(), 1);
    }

    // Test with a different chassis number
    {
        Chassis chassis{3};
        EXPECT_EQ(chassis.getNumber(), 3);
    }
}

TEST_F(ChassisTests, GetDevices)
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

        devices.emplace_back(std::make_unique<Device>(
            "DeviceName_1", "DeviceDirection_1", "DevicePolarity_1"));

        devices.emplace_back(std::make_unique<Device>(
            "DeviceName_2", "DeviceDirection_2", "DevicePolarity_2"));

        devices.emplace_back(std::make_unique<Device>(
            "DeviceName_3", "DeviceDirection_3", "DevicePolarity_3"));

        // Create Chassis
        Chassis chassis{1, std::nullopt, std::move(devices)};

        // Verify the number of devices
        const auto& chassisDevices = chassis.getDevices();
        EXPECT_EQ(chassisDevices.size(), 3);

        // Verify each device's properties
        EXPECT_EQ(chassisDevices[0]->getName(), "DeviceName_1");
        EXPECT_EQ(chassisDevices[0]->getDirection(), "DeviceDirection_1");
        EXPECT_EQ(chassisDevices[0]->getPolarity(), "DevicePolarity_1");

        EXPECT_EQ(chassisDevices[1]->getName(), "DeviceName_2");
        EXPECT_EQ(chassisDevices[1]->getDirection(), "DeviceDirection_2");
        EXPECT_EQ(chassisDevices[1]->getPolarity(), "DevicePolarity_2");

        EXPECT_EQ(chassisDevices[2]->getName(), "DeviceName_3");
        EXPECT_EQ(chassisDevices[2]->getDirection(), "DeviceDirection_3");
        EXPECT_EQ(chassisDevices[2]->getPolarity(), "DevicePolarity_3");
    }
}
