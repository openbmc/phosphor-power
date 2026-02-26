/**
 * Copyright © 2026 IBM Corporation
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
#include "gpio.hpp"

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
        EXPECT_EQ(chassis.getGpios().size(), 0);
    }

    // Test where works: required and Presence Path.
    {
        // Create Chassis
        Chassis chassis{1, "/dev/i2c-359"};
        EXPECT_EQ(chassis.getNumber(), 1);
        EXPECT_EQ(chassis.getPresencePath(), "/dev/i2c-359");
        EXPECT_EQ(chassis.getGpios().size(), 0);
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

TEST_F(ChassisTests, GetPresencePath)
{
    std::vector<std::unique_ptr<Gpio>> gpios{};

    // Create Chassis
    Chassis chassis{1, "/dev/i2c-359"};
    EXPECT_EQ(chassis.getNumber(), 1);
    EXPECT_EQ(chassis.getPresencePath(), "/dev/i2c-359");
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

TEST_F(ChassisTests, getGpios)
{
    // Test where no GPIOs were specified in constructor
    {
        Chassis chassis{2};
        EXPECT_EQ(chassis.getGpios().size(), 0);
    }

    // Test where GPIOs were specified in constructor
    {
        // Create vector of Gpio objects
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(std::make_unique<Gpio>(
            "GpioName_1", parseDirection("Input"), parsePolarity("High")));

        gpios.emplace_back(std::make_unique<Gpio>(
            "GpioName_2", parseDirection("Input"), parsePolarity("Low")));

        gpios.emplace_back(std::make_unique<Gpio>(
            "GpioName_3", parseDirection("Output"), parsePolarity("High")));

        // Create Chassis
        Chassis chassis{1, std::nullopt, std::move(gpios)};

        // Verify the number of gpios
        const auto& chassisGpios = chassis.getGpios();
        EXPECT_EQ(chassisGpios.size(), 3);

        // Verify each GPIO's properties
        EXPECT_EQ(chassisGpios[0]->getName(), "GpioName_1");
        EXPECT_EQ(chassisGpios[0]->getDirection(), gpioDirection::Input);
        EXPECT_EQ(chassisGpios[0]->getPolarity(), gpioPolarity::High);

        EXPECT_EQ(chassisGpios[1]->getName(), "GpioName_2");
        EXPECT_EQ(chassisGpios[1]->getDirection(), gpioDirection::Input);
        EXPECT_EQ(chassisGpios[1]->getPolarity(), gpioPolarity::Low);

        EXPECT_EQ(chassisGpios[2]->getName(), "GpioName_3");
        EXPECT_EQ(chassisGpios[2]->getDirection(), gpioDirection::Output);
        EXPECT_EQ(chassisGpios[2]->getPolarity(), gpioPolarity::High);
    }
}
