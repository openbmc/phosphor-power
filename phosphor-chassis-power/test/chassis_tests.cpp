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
#include "mock_services.hpp"

#include <sdbusplus/bus.hpp>

#include <filesystem>
#include <fstream>
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

/**
 * Helper function to get a MockGpio reference from a Chassis GPIO vector.
 *
 * @param chassis Chassis object containing the GPIOs
 * @param i Index of the GPIO in the chassis vector
 * @return Reference to the MockGpio at the specified index
 */
MockGpio& getMockGpio(Chassis& chassis, size_t i)
{
    return static_cast<MockGpio&>(*(chassis.getGpios()[i]));
}

/**
 * Helper to set GPIO read expectations for presence detection.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=present, 0=absent)
 * @param prevValue Previous GPIO value
 */
void expectPresenceGpio(Chassis& chassis, int value, int prevValue)
{
    auto& gpio = getMockGpio(chassis, 0);
    EXPECT_CALL(gpio, foundLine()).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, requestRead()).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, getValue()).WillOnce(testing::Return(value));
    EXPECT_CALL(gpio, getPreviousValue()).WillOnce(testing::Return(prevValue));
    EXPECT_CALL(gpio, release()).Times(1);
}

/**
 * Helper to set repeated GPIO read expectations for presence detection.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=present, 0=absent)
 * @param prevValue Previous GPIO value
 */
void expectPresenceGpioRepeated(Chassis& chassis, int value, int prevValue)
{
    auto& gpio = getMockGpio(chassis, 0);
    EXPECT_CALL(gpio, foundLine()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(gpio, requestRead()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(gpio, getValue()).WillRepeatedly(testing::Return(value));
    EXPECT_CALL(gpio, getPreviousValue())
        .WillRepeatedly(testing::Return(prevValue));
    EXPECT_CALL(gpio, release()).WillRepeatedly(testing::Return());
}

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
        MockServices services{};
        Chassis chassis{2, services};
        EXPECT_EQ(chassis.getNumber(), 2);
        EXPECT_EQ(chassis.getGpios().size(), 0);
        EXPECT_FALSE(chassis.getPresencePath().has_value());
    }

    // Test where fails: Invalid chassis number < 1
    try
    {
        MockServices services{};
        Chassis chassis{0, services};
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
        MockServices services{};
        Chassis chassis{1, services, "/dev/i2c-359"};
        EXPECT_EQ(chassis.getNumber(), 1);
        EXPECT_EQ(chassis.getPresencePath(), "/dev/i2c-359");
    }
}

TEST_F(ChassisTests, GetNumber)
{
    // Test where only required parameter (number as int) is specified
    {
        MockServices services{};
        Chassis chassis{1, services};
        EXPECT_EQ(chassis.getNumber(), 1);
    }

    // Test where only required parameter (number as hex) is specified
    {
        MockServices services{};
        Chassis chassis{0xa, services};
        EXPECT_EQ(chassis.getNumber(), 10);
    }
}

TEST_F(ChassisTests, getGpios)
{
    // Test where no GPIOs were specified in constructor
    {
        MockServices services{};
        Chassis chassis{2, services};
        EXPECT_EQ(chassis.getGpios().size(), 0);
    }

    // Test where GPIOs were specified in constructor (without default values)
    {
        // Create vector of Gpio objects
        std::vector<std::unique_ptr<Gpio>> gpios{};
        MockServices services{};

        gpios.emplace_back(services.createGPIO(
            "GpioName_1", GpioDirection::Input, GpioPolarity::High));

        gpios.emplace_back(services.createGPIO(
            "GpioName_2", GpioDirection::Input, GpioPolarity::Low));

        gpios.emplace_back(services.createGPIO(
            "GpioName_3", GpioDirection::Output, GpioPolarity::High));

        // Create Chassis
        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

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
        MockServices services{};

        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low, 0));

        gpios.emplace_back(
            services.createGPIO("power-fault-unlatched", GpioDirection::Input,
                                GpioPolarity::Low, 1));

        gpios.emplace_back(services.createGPIO(
            "power-fault-reset", GpioDirection::Output, GpioPolarity::Low));

        // Create Chassis
        Chassis chassis{2, services, "/dev/i2c-259", std::move(gpios)};

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
        MockServices services{};
        Chassis chassis{1, services};
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);
    }

    // Test where interface has been set to Good
    {
        MockServices services{};
        Chassis chassis{1, services};

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
        MockServices services{};
        Chassis chassis{1, services};

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

TEST_F(ChassisTests, Monitor)
{
    // Test where no GPIOs configured
    {
        MockServices services{};
        Chassis chassis{1, services};
        chassis.monitor();
    }

    // Test where GPIO line not found
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(false));
        EXPECT_CALL(getMockGpio(chassis, 0), findLine())
            .WillOnce(testing::Return(true));

        chassis.monitor();
    }

    // Test where requestRead fails
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(false));

        chassis.monitor();

        EXPECT_FALSE(chassis.getPresenceGPIOValue().has_value());
    }

    // Test monitoring all three GPIO types in one pass
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};

        // Add presence GPIO
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        // Add fault-latched GPIO
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-latched",
                                GpioDirection::Input, GpioPolarity::Low));

        // Add fault-unlatched GPIO
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-unlatched",
                                GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        // Setup expectations for presence GPIO
        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), release()).Times(1);

        // Setup expectations for fault-latched GPIO
        EXPECT_CALL(getMockGpio(chassis, 1), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), getValue())
            .WillOnce(testing::Return(0));
        EXPECT_CALL(getMockGpio(chassis, 1), getPreviousValue())
            .WillOnce(testing::Return(0));

        // Setup expectations for fault-unlatched GPIO
        EXPECT_CALL(getMockGpio(chassis, 2), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 2), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 2), getValue())
            .WillOnce(testing::Return(0));
        EXPECT_CALL(getMockGpio(chassis, 2), getPreviousValue())
            .WillOnce(testing::Return(0));

        chassis.monitor();

        EXPECT_EQ(chassis.getPresenceGPIOValue(), 1);
        EXPECT_EQ(chassis.getFaultLatchedValue(), 0);
        EXPECT_EQ(chassis.getFaultUnlatchedValue(), 0);
    }
}

TEST_F(ChassisTests, gpioValueChanged)
{
    // Test where previous value fails
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        // Get reference to mock GPIO
        MockGpio& mockGpio = getMockGpio(chassis, 0);

        // Setup expectations for GPIO operations
        EXPECT_CALL(mockGpio, foundLine()).WillOnce(testing::Return(true));
        EXPECT_CALL(mockGpio, requestRead()).WillOnce(testing::Return(true));
        EXPECT_CALL(mockGpio, getValue()).WillOnce(testing::Return(1));
        EXPECT_CALL(mockGpio, getPreviousValue())
            .WillOnce(testing::Throw(std::runtime_error("No previous value")));
        EXPECT_CALL(mockGpio, release()).Times(1);

        chassis.monitor();
        EXPECT_EQ(chassis.getPresenceGPIOValue(), 1);
    }

    // Test where getValue fails on first read
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        // Get reference to mock GPIO
        MockGpio& mockGpio = getMockGpio(chassis, 0);

        // Setup expectations for GPIO operations
        EXPECT_CALL(mockGpio, foundLine()).WillOnce(testing::Return(true));
        EXPECT_CALL(mockGpio, requestRead()).WillOnce(testing::Return(true));
        EXPECT_CALL(mockGpio, getValue())
            .WillOnce(
                testing::Throw(std::runtime_error("Failed to read value")));
        EXPECT_CALL(mockGpio, release()).Times(1);
        chassis.monitor();
        EXPECT_EQ(chassis.getPresenceGPIOValue(), std::nullopt);
    }

    // Test where getValue fails, after successful reads
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        // Get reference to mock GPIO
        MockGpio& mockGpio = getMockGpio(chassis, 0);

        // Setup expectations for 2 monitor() calls
        EXPECT_CALL(mockGpio, foundLine())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(mockGpio, requestRead())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(mockGpio, getValue())
            .WillOnce(testing::Return(0)) // First call succeeds
            .WillOnce(testing::Throw(
                std::runtime_error("Failed to read value"))); // Second fails
        EXPECT_CALL(mockGpio, getPreviousValue()).WillOnce(testing::Return(0));
        EXPECT_CALL(mockGpio, release()).Times(2);

        chassis.monitor();
        chassis.monitor();
        EXPECT_EQ(chassis.getPresenceGPIOValue(), 0);
    }

    // Test where value matches previousValue
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), release()).Times(1);

        chassis.monitor();
        EXPECT_EQ(chassis.getPresenceGPIOValue(), 1);
    }

    // Test where value != previousValue
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        // Setup expectations for 2 monitor() calls
        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(0))  // First call
            .WillOnce(testing::Return(1)); // Second call
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .Times(2)
            .WillRepeatedly(testing::Return(0));
        EXPECT_CALL(getMockGpio(chassis, 0), release()).Times(2);

        chassis.monitor();
        chassis.monitor();
        EXPECT_EQ(chassis.getPresenceGPIOValue(), 0);
    }

    // Test where value changes on second read and is accepted on the third read
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};

        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        // Setup expectations for 3 monitor() calls
        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(0))        // First call
            .WillRepeatedly(testing::Return(1)); // Second and third calls
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(0))        // First call
            .WillOnce(testing::Return(0))        // Second call
            .WillOnce(testing::Return(1));       // Third call
        EXPECT_CALL(getMockGpio(chassis, 0), release()).Times(3);

        chassis.monitor();

        EXPECT_EQ(chassis.getPresenceGPIOValue(), 0);

        chassis.monitor();

        EXPECT_EQ(chassis.getPresenceGPIOValue(), 0);

        chassis.monitor();

        EXPECT_EQ(chassis.getPresenceGPIOValue(), 1);
    }
}

TEST_F(ChassisTests, HandlePresenceChange)
{
    using ::testing::_;

    // GPIO ON and Presence path exists
    {
        auto tempPath = std::filesystem::temp_directory_path() / "test";
        std::ofstream(tempPath).close();

        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, tempPath.string(), std::move(gpios)};

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        expectPresenceGpio(chassis, 1, 1);

        // Monitor to update GPIO value and handle presence change
        chassis.monitor();

        // Verify chassis is present
        EXPECT_TRUE(chassis.getPresenceValue());

        std::filesystem::remove(tempPath);
    }

    // GPIO off and Presence path exists, system off
    {
        auto tempPath = std::filesystem::temp_directory_path() / "test";
        std::ofstream(tempPath).close();

        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, tempPath.string(), std::move(gpios)};

        chassis.initializePresence();

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        expectPresenceGpio(chassis, 0, 0);

        auto* mockMonitor = static_cast<MockChassisStatusMonitor*>(
            chassis.getSystemMonitor().get());
        EXPECT_CALL(*mockMonitor, getPowerGood()).WillOnce(testing::Return(0));

        EXPECT_CALL(services, logError).Times(0);

        // Monitor to update GPIO value and handle presence change
        chassis.monitor();

        // Verify chassis is present
        EXPECT_TRUE(chassis.getPresenceValue());

        std::filesystem::remove(tempPath);
    }

    // GPIO off, Presence path exists, system on
    {
        auto tempPath = std::filesystem::temp_directory_path() / "test_pel";
        std::ofstream(tempPath).close();

        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, tempPath.string(), std::move(gpios)};

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        chassis.initializePresence();

        expectPresenceGpioRepeated(chassis, 0, 0);

        auto* mockMonitor = static_cast<MockChassisStatusMonitor*>(
            chassis.getSystemMonitor().get());
        EXPECT_CALL(*mockMonitor, getPowerGood()).WillOnce(testing::Return(1));

        EXPECT_CALL(services, logError("xyz.openbmc_project.Power.Chassis."
                                       "PresentDetection.Incorrect",
                                       Entry::Level::Error, _))
            .Times(1);
        chassis.monitor();
        chassis.monitor();

        EXPECT_EQ(chassis.getPresenceGPIOValue(), 0);
        EXPECT_TRUE(chassis.getPresenceValue());

        std::filesystem::remove(tempPath);
    }

    // GPIO off and Presence Path not specified, system off
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        chassis.initializePresence();

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        expectPresenceGpio(chassis, 0, 0);

        auto* mockMonitor = static_cast<MockChassisStatusMonitor*>(
            chassis.getSystemMonitor().get());
        EXPECT_CALL(*mockMonitor, getPowerGood()).WillOnce(testing::Return(0));

        EXPECT_CALL(services, logError).Times(0);

        // Monitor to update GPIO value and handle presence change
        chassis.monitor();

        // Verify chassis is absent
        EXPECT_FALSE(chassis.getPresenceValue());
    }

    // GPIO off and Presence Path not specified, system on
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        chassis.initializePresence();

        expectPresenceGpioRepeated(chassis, 0, 0);
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(1))
            .WillOnce(testing::Return(0));

        auto* mockMonitor = static_cast<MockChassisStatusMonitor*>(
            chassis.getSystemMonitor().get());
        EXPECT_CALL(*mockMonitor, getPowerGood()).WillOnce(testing::Return(1));
        EXPECT_CALL(
            services,
            logError(
                "xyz.openbmc_project.Power.Chassis.Missing.ShouldBePresent",
                Entry::Level::Error, _))
            .Times(1);

        chassis.monitor();
        chassis.monitor();

        EXPECT_EQ(chassis.getPresenceGPIOValue(), 0);
        EXPECT_FALSE(chassis.getPresenceValue());
    }

    // GPIO On and Presence path does not exists
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        auto tempPath =
            std::filesystem::temp_directory_path() / "test_presence";
        Chassis chassis{1, services, tempPath.string(), std::move(gpios)};

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        expectPresenceGpio(chassis, 1, 1);

        // Monitor to update GPIO value and handle presence change
        chassis.monitor();

        // Verify chassis is present (GPIO says present)
        EXPECT_TRUE(chassis.getPresenceValue());
    }

    // GPIO read failure, Presence path exists, system off
    {
        auto tempPath = std::filesystem::temp_directory_path() / "test";
        std::ofstream(tempPath).close();

        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, tempPath.string(), std::move(gpios)};

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        chassis.initializePresence();

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillRepeatedly(
                testing::Throw(std::runtime_error("GPIO read failure")));
        EXPECT_CALL(getMockGpio(chassis, 0), release())
            .WillRepeatedly(testing::Return());

        auto* mockMonitor = static_cast<MockChassisStatusMonitor*>(
            chassis.getSystemMonitor().get());
        EXPECT_CALL(*mockMonitor, getPowerGood())
            .WillRepeatedly(testing::Return(0));

        EXPECT_CALL(services, logError).Times(0);

        chassis.monitor();

        EXPECT_TRUE(chassis.getPresenceValue());

        std::filesystem::remove(tempPath);
    }

    // GPIO read failure, Presence path not specified, system off
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, std::nullopt, std::move(gpios)};

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        chassis.initializePresence();

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Throw(std::runtime_error("GPIO read failure")));
        EXPECT_CALL(getMockGpio(chassis, 0), release())
            .WillOnce(testing::Return());

        auto* mockMonitor = static_cast<MockChassisStatusMonitor*>(
            chassis.getSystemMonitor().get());
        EXPECT_CALL(*mockMonitor, getPowerGood()).WillOnce(testing::Return(0));

        EXPECT_CALL(services, logError).Times(0);

        chassis.monitor();

        EXPECT_FALSE(chassis.getPresenceValue());
    }
}
