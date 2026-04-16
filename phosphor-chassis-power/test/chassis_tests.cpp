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
#include "chassis_power_system_interface.hpp"
#include "gpio.hpp"

#include <sdbusplus/bus.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::chassis;
using PowerSystemInputs = sdbusplus::server::xyz::openbmc_project::state::
    decorator::PowerSystemInputs;

class ChassisTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the D-Bus bus object needed for some Chassis methods.
     */
    ChassisTests() : bus{sdbusplus::bus::new_default()} {}

  protected:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t bus;
};

TEST_F(ChassisTests, Constructor)
{
    // Test where works: Only required parameters are specified
    {
        Chassis chassis{2};
        EXPECT_EQ(chassis.getNumber(), 2);
        EXPECT_EQ(chassis.getGpios().size(), 0);
        EXPECT_FALSE(chassis.getPresencePath().has_value());
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
    // Test where works: Only PresencePath specified with Absolute path
    {
        // Create Chassis
        Chassis chassis{1, "/dev/i2c-359"};
        EXPECT_EQ(chassis.getNumber(), 1);
        EXPECT_EQ(chassis.getPresencePath(), "/dev/i2c-359");
    }
}

TEST_F(ChassisTests, GetNumber)
{
    // Test where only required parameter (number as int) is specified
    {
        Chassis chassis{1};
        EXPECT_EQ(chassis.getNumber(), 1);
    }

    // Test where only required parameter (number as hex) is specified
    {
        Chassis chassis{0xa};
        EXPECT_EQ(chassis.getNumber(), 10);
    }
}

TEST_F(ChassisTests, getGpios)
{
    // Test where no GPIOs were specified in constructor
    {
        Chassis chassis{2};
        EXPECT_EQ(chassis.getGpios().size(), 0);
    }

    // Test where GPIOs were specified in constructor (without default values)
    {
        // Create vector of Gpio objects
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(std::make_unique<Gpio>(
            "GpioName_1", GpioDirection::Input, GpioPolarity::High));

        gpios.emplace_back(std::make_unique<Gpio>(
            "GpioName_2", GpioDirection::Input, GpioPolarity::Low));

        gpios.emplace_back(std::make_unique<Gpio>(
            "GpioName_3", GpioDirection::Output, GpioPolarity::High));

        // Create Chassis
        Chassis chassis{1, std::nullopt, std::move(gpios)};

        // Verify the number of gpios
        const auto& chassisGpios = chassis.getGpios();
        EXPECT_EQ(chassisGpios.size(), 3);

        // Verify each GPIO's properties (no default values)
        EXPECT_EQ(chassisGpios[0]->getName(), "GpioName_1");
        EXPECT_EQ(chassisGpios[0]->getDirection(), GpioDirection::Input);
        EXPECT_EQ(chassisGpios[0]->getPolarity(), GpioPolarity::High);
        EXPECT_FALSE(chassisGpios[0]->getDefaultValue().has_value());

        EXPECT_EQ(chassisGpios[1]->getName(), "GpioName_2");
        EXPECT_EQ(chassisGpios[1]->getDirection(), GpioDirection::Input);
        EXPECT_EQ(chassisGpios[1]->getPolarity(), GpioPolarity::Low);
        EXPECT_FALSE(chassisGpios[1]->getDefaultValue().has_value());

        EXPECT_EQ(chassisGpios[2]->getName(), "GpioName_3");
        EXPECT_EQ(chassisGpios[2]->getDirection(), GpioDirection::Output);
        EXPECT_EQ(chassisGpios[2]->getPolarity(), GpioPolarity::High);
        EXPECT_FALSE(chassisGpios[2]->getDefaultValue().has_value());
    }

    // Test where GPIOs with default values were specified in constructor
    {
        // Create vector of Gpio objects with default values
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(std::make_unique<Gpio>(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low, 0));

        gpios.emplace_back(
            std::make_unique<Gpio>("power-fault-unlatched",
                                   GpioDirection::Input, GpioPolarity::Low, 1));

        gpios.emplace_back(std::make_unique<Gpio>(
            "power-fault-reset", GpioDirection::Output, GpioPolarity::Low));

        // Create Chassis
        Chassis chassis{2, "/dev/i2c-259", std::move(gpios)};

        // Verify the number of gpios
        const auto& chassisGpios = chassis.getGpios();
        EXPECT_EQ(chassisGpios.size(), 3);

        // Verify GPIO with default Low
        EXPECT_EQ(chassisGpios[0]->getName(), "presence-chassis1");
        EXPECT_EQ(chassisGpios[0]->getDirection(), GpioDirection::Input);
        EXPECT_EQ(chassisGpios[0]->getPolarity(), GpioPolarity::Low);
        EXPECT_TRUE(chassisGpios[0]->getDefaultValue().has_value());
        EXPECT_EQ(chassisGpios[0]->getDefaultValue().value(), 0);

        // Verify GPIO with default High
        EXPECT_EQ(chassisGpios[1]->getName(), "power-fault-unlatched");
        EXPECT_EQ(chassisGpios[1]->getDirection(), GpioDirection::Input);
        EXPECT_EQ(chassisGpios[1]->getPolarity(), GpioPolarity::Low);
        EXPECT_TRUE(chassisGpios[1]->getDefaultValue().has_value());
        EXPECT_EQ(chassisGpios[1]->getDefaultValue().value(), 1);

        // Verify GPIO without default
        EXPECT_EQ(chassisGpios[2]->getName(), "power-fault-reset");
        EXPECT_EQ(chassisGpios[2]->getDirection(), GpioDirection::Output);
        EXPECT_EQ(chassisGpios[2]->getPolarity(), GpioPolarity::Low);
        EXPECT_FALSE(chassisGpios[2]->getDefaultValue().has_value());
    }
}

TEST_F(ChassisTests, getPowerSystemInputsInterface)
{
    // Test where interface has not been set
    {
        Chassis chassis{1};
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);
    }

    // Test where interface has been set to Good
    {
        Chassis chassis{1};

        chassis.initializePowerSystemInputsInterface(bus);

        // Verify interface was set and has correct status
        EXPECT_NE(chassis.getPowerSystemInputsInterface(), nullptr);
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }
}

TEST_F(ChassisTests, initializePowerSystemInputsInterface)
{
    // Test setting interface successfully
    {
        Chassis chassis{1};

        // Verify initial state
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        // Set interface using bus
        bool result = chassis.initializePowerSystemInputsInterface(bus);

        // Verify interface was created successfully
        EXPECT_TRUE(result);
        EXPECT_NE(chassis.getPowerSystemInputsInterface(), nullptr);
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }
}
