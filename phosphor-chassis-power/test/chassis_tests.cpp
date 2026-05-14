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
#include <sdeventplus/event.hpp>

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

/**
 * Helper to build a Chassis pre-wired with the five GPIOs
 *
 * GPIO layout:
 *   0 – presence-chassis1              (Input,  Low  – not-present)
 *   1 – power-chs1-sb-fault-unlatched  (Input,  Low  – no-fault)
 *   2 – power-chs1-sb-fault-latched    (Input,  Low  – no-fault)
 *   3 – reset-enable-chs1-sb-power     (Output, High)
 *   4 – power-chs1-sb-fault-reset      (Output, Low)
 *
 * @param services MockServices instance (must outlive the returned Chassis)
 * @param event    sdeventplus::Event instance
 * @return Chassis configured for missing-sled testing
 */
Chassis buildSledChassis(MockServices& services, sdeventplus::Event& event)
{
    std::vector<std::unique_ptr<Gpio>> gpios{};

    // Index 0: presence-chassis1 disabled (not-present)
    gpios.emplace_back(services.createGPIO(
        "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

    // Index 1: power-chs1-sb-fault-unlatched disabled (no-fault)
    gpios.emplace_back(
        services.createGPIO("power-chs1-sb-fault-unlatched",
                            GpioDirection::Input, GpioPolarity::Low));

    // Index 2: power-chs1-sb-fault-latched disabled (no-fault)
    gpios.emplace_back(
        services.createGPIO("power-chs1-sb-fault-latched", GpioDirection::Input,
                            GpioPolarity::Low));

    // Index 3: reset-enable-chs1-sb-power
    gpios.emplace_back(
        services.createGPIO("reset-enable-chs1-sb-power", GpioDirection::Output,
                            GpioPolarity::High));

    // Index 4: power-chs1-sb-fault-reset
    gpios.emplace_back(services.createGPIO(
        "power-chs1-sb-fault-reset", GpioDirection::Output, GpioPolarity::Low));

    // No presence path → isPresent() returns false
    return Chassis{1, services, event, std::nullopt, std::move(gpios)};
}

class ChassisTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the D-Bus bus object and event loop needed for some Chassis
     * methods.
     */
    ChassisTests() :
        bus{sdbusplus::bus::new_default()},
        event{sdeventplus::Event::get_default()}
    {}

  protected:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t bus;

    /**
     * Event loop object.
     */
    sdeventplus::Event event;
};

TEST_F(ChassisTests, Constructor)
{
    // Test where works: Only required parameters are specified
    {
        MockServices services{};
        Chassis chassis{2, services, event};
        EXPECT_EQ(chassis.getNumber(), 2);
        EXPECT_EQ(chassis.getGpios().size(), 0);
        EXPECT_FALSE(chassis.getPresencePath().has_value());
    }

    // Test where fails: Invalid chassis number < 1
    try
    {
        MockServices services{};
        Chassis chassis{0, services, event};
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
        Chassis chassis{1, services, event, "/dev/i2c-359"};
        EXPECT_EQ(chassis.getNumber(), 1);
        EXPECT_EQ(chassis.getPresencePath(), "/dev/i2c-359");
    }
}

TEST_F(ChassisTests, GetNumber)
{
    // Test where only required parameter (number as int) is specified
    {
        MockServices services{};
        Chassis chassis{1, services, event};
        EXPECT_EQ(chassis.getNumber(), 1);
    }

    // Test where only required parameter (number as hex) is specified
    {
        MockServices services{};
        Chassis chassis{0xa, services, event};
        EXPECT_EQ(chassis.getNumber(), 10);
    }
}

TEST_F(ChassisTests, getGpios)
{
    // Test where no GPIOs were specified in constructor
    {
        MockServices services{};
        Chassis chassis{2, services, event};
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
        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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
        Chassis chassis{2, services, event, "/dev/i2c-259", std::move(gpios)};

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

TEST_F(ChassisTests, GetPowerSystemInputsInterface)
{
    // Test where interface has not been set
    {
        MockServices services{};
        Chassis chassis{1, services, event};
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);
    }

    // Test where interface has been set to Good
    {
        MockServices services{};
        Chassis chassis{1, services, event};

        chassis.initializePowerSystemInputsInterface(
            PowerSystemInputs::Status::Good);

        // Verify interface was set and has correct status
        EXPECT_NE(chassis.getPowerSystemInputsInterface(), nullptr);
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }
}

TEST_F(ChassisTests, InitializePowerSystemInputsInterface)
{
    // Test setting interface successfully with Good status
    {
        MockServices services{};
        Chassis chassis{1, services, event};

        bool result = chassis.initializePowerSystemInputsInterface(
            PowerSystemInputs::Status::Good);

        EXPECT_TRUE(result);
        EXPECT_NE(chassis.getPowerSystemInputsInterface(), nullptr);
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }

    // Test setting interface successfully with Fault status
    {
        MockServices services{};
        Chassis chassis{1, services, event};

        bool result = chassis.initializePowerSystemInputsInterface(
            PowerSystemInputs::Status::Fault);

        EXPECT_TRUE(result);
        EXPECT_NE(chassis.getPowerSystemInputsInterface(), nullptr);
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Fault);
    }
}

TEST_F(ChassisTests, SetPowerSystemInputsStatus)
{
    // Test where interface not created, initialized to Fault.
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-unlatched",
                                GpioDirection::Input, GpioPolarity::Low));
        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Throw(std::runtime_error("No previous value")));

        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        chassis.monitor();

        EXPECT_NE(chassis.getPowerSystemInputsInterface(), nullptr);
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Fault);
        EXPECT_EQ(chassis.getFaultUnlatchedValue(), 1);
    }

    // Test where interface not created, initialized to Good.
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-unlatched",
                                GpioDirection::Input, GpioPolarity::Low));
        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(0));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Throw(std::runtime_error("No previous value")));

        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        chassis.monitor();

        EXPECT_NE(chassis.getPowerSystemInputsInterface(), nullptr);
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
        EXPECT_EQ(chassis.getFaultUnlatchedValue(), 0);
    }

    // Test where interface is already initialized, and status updated
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-unlatched",
                                GpioDirection::Input, GpioPolarity::Low));
        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        chassis.initializePowerSystemInputsInterface(
            PowerSystemInputs::Status::Good);
        const auto* firstIface = chassis.getPowerSystemInputsInterface().get();

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Throw(std::runtime_error("No previous value")));

        chassis.monitor();

        // Same interface object, status updated to Fault
        EXPECT_EQ(chassis.getPowerSystemInputsInterface().get(), firstIface);
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Fault);
    }
}

TEST_F(ChassisTests, Monitor)
{
    // Test where no GPIOs configured
    {
        MockServices services{};
        Chassis chassis{1, services, event};
        chassis.monitor();
    }

    // Test where GPIO line not found
    {
        MockServices services{};
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, tempPath.string(),
                        std::move(gpios)};

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

        Chassis chassis{1, services, event, tempPath.string(),
                        std::move(gpios)};

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

        Chassis chassis{1, services, event, tempPath.string(),
                        std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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
        Chassis chassis{1, services, event, tempPath.string(),
                        std::move(gpios)};

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

        Chassis chassis{1, services, event, tempPath.string(),
                        std::move(gpios)};

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

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

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

TEST_F(ChassisTests, HandleBMCReset_MissingSled)
{
    {
        // (1) Test For missing sleds (isPresent() returns false):
        // + chassis power                      not-checked
        // + presence-chassis1                  disabled
        // + power-chs1-sb-fault-unlatched      not-checked
        // + power-chs1-sb-fault-latched        not-checked
        //
        // - reset-enable-chs1-sb-power         expect disabled
        // - power-chs1-sb-fault-reset          expect enabled
        // - ChassisState                       expect Missing
        // - PowerSystemInputs status           expect ????
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        // ---- chassis power ----
        auto monitorOwner =
            std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
        auto* mockMonitor = monitorOwner.get();
        chassis.setChassisStatusMonitor(std::move(monitorOwner));
        EXPECT_CALL(*mockMonitor, isPoweredOn()).Times(0); // not-checked

        // ---- presence-chassis1 ----
        auto& presenceGpio = getMockGpio(chassis, 0);
        EXPECT_CALL(presenceGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, getValue())
            .WillOnce(testing::Return(0)); // set return disabled
        EXPECT_CALL(presenceGpio, release()).Times(1);

        // ---- power-chs1-sb-fault-unlatched ----
        auto& faultUnlatchedGpio = getMockGpio(chassis, 1); // not-checked
        EXPECT_CALL(faultUnlatchedGpio, foundLine()).Times(0);
        EXPECT_CALL(faultUnlatchedGpio, requestRead()).Times(0);
        EXPECT_CALL(faultUnlatchedGpio, getValue()).Times(0);
        EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-latched ----
        auto& faultLatchedGpio = getMockGpio(chassis, 2); // not-checked
        EXPECT_CALL(faultUnlatchedGpio, foundLine()).Times(0);
        EXPECT_CALL(faultUnlatchedGpio, requestRead()).Times(0);
        EXPECT_CALL(faultUnlatchedGpio, getValue()).Times(0);
        EXPECT_CALL(faultLatchedGpio, release()).Times(0);

        // ---- reset-enable-chs1-sb-power ----
        auto& resetEnableGpio = getMockGpio(chassis, 3);
        EXPECT_CALL(resetEnableGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, requestWrite(0)) // set return disabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, setValue(0))     // set return disabled
            .Times(1);
        EXPECT_CALL(resetEnableGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-reset ----
        auto& faultResetGpio = getMockGpio(chassis, 4);
        EXPECT_CALL(faultResetGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, requestWrite(1)) // set return enabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, setValue(1))     // set return enabled
            .Times(1);
        EXPECT_CALL(faultResetGpio, release()).Times(0);

        chassis.handleBMCReset();

        // ---- ChassisState ----
        EXPECT_EQ(chassis.getState(), ChassisState::Missing); // expect Missing

        // ---- PowerSystemInputs status ----
        // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
        //           PowerSystemInputs::Status::????); // expect ????
    }
}

TEST_F(ChassisTests, HandleBMCReset_PresentSledChassisOff)
{
    {
        // (2.1) Test Present sled, no faults, D-Bus reports chassis OFF:
        // + chassis power                      Off
        // + presence-chassis1                  enabled
        // + power-chs1-sb-fault-unlatched      disabled
        // + power-chs1-sb-fault-latched        not-checked
        // SHELDON:QUESTION: latched not checked seems like would not reset!
        //
        // - reset-enable-chs1-sb-power         expect disabled
        // - power-chs1-sb-fault-reset          expect enabled
        // - ChassisState                       expect Off
        // - PowerSystemInputs status           expect ????
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        // ---- chassis power ----
        auto monitorOwner =
            std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
        auto* mockMonitor = monitorOwner.get();
        chassis.setChassisStatusMonitor(std::move(monitorOwner));
        // ---- Status Monitor Power ----
        EXPECT_CALL(*mockMonitor, isPoweredOn())
            .WillOnce(testing::Return(false)); // set return OFF

        // ---- presence-chassis1 ----
        auto& presenceGpio = getMockGpio(chassis, 0);
        EXPECT_CALL(presenceGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, getValue())
            .WillOnce(testing::Return(1)); // set return present
        EXPECT_CALL(presenceGpio, release()).Times(1);

        // ---- power-chs1-sb-fault-unlatched ----
        auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
        // EXPECT_CALL(faultUnlatchedGpio, foundLine())
        //     // pre-check avoid calling findLine() unnecessarily,
        //     .WillOnce(testing::Return(true))
        //     // safety re-check confirm the line available before using it
        //     .WillOnce(testing::Return(true));
        // EXPECT_CALL(faultUnlatchedGpio, requestRead())
        //     .WillOnce(testing::Return(true));
        // EXPECT_CALL(faultUnlatchedGpio, getValue())
        //     .WillOnce(testing::Return(0)); // set return disabled
        // EXPECT_CALL(faultUnlatchedGpio, release()).Times(1);
        EXPECT_CALL(faultUnlatchedGpio, foundLine()).Times(0);
        EXPECT_CALL(faultUnlatchedGpio, requestRead()).Times(0);
        EXPECT_CALL(faultUnlatchedGpio, getValue()).Times(0);
        EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-latched ----
        auto& faultLatchedGpio = getMockGpio(chassis, 2);
        EXPECT_CALL(faultUnlatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, getValue())
            .WillOnce(testing::Return(0)); // set return disabled
        EXPECT_CALL(faultLatchedGpio, release()).Times(0);

        // ---- reset-enable-chs1-sb-power ----
        auto& resetEnableGpio = getMockGpio(chassis, 3);
        EXPECT_CALL(resetEnableGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, requestWrite(0)) // set return disabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, setValue(0))     // set return disabled
            .Times(1);                                // set return disabled
        EXPECT_CALL(resetEnableGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-reset ----
        auto& faultResetGpio = getMockGpio(chassis, 4);
        EXPECT_CALL(faultResetGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, requestWrite(1)) // set return enabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, setValue(1))     // set return enabled
            .Times(1);
        EXPECT_CALL(faultResetGpio, release()).Times(0);

        chassis.handleBMCReset();

        // ---- ChassisState ----
        EXPECT_EQ(chassis.getState(), ChassisState::Off); // expect Off

        // ---- PowerSystemInputs status ----
        // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
        //           PowerSystemInputs::Status::????); // expect ????
    }

    {
        // (2.2) Test Present sled, power-chs1-sb-fault-latched fault,
        //                          D-Bus reports chassis OFF:
        // + chassis power                      Off
        // + presence-chassis1                  enabled
        // + power-chs1-sb-fault-unlatched      disabled
        // + power-chs1-sb-fault-latched        enabled
        //
        // - reset-enable-chs1-sb-power         expect disabled
        // - power-chs1-sb-fault-reset          expect enabled
        // - ChassisState                       expect Off
        // - PowerSystemInputs status           expect ????

        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        // ---- chassis power ----
        auto monitorOwner =
            std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
        auto* mockMonitor = monitorOwner.get();
        chassis.setChassisStatusMonitor(std::move(monitorOwner));
        // ---- Status Monitor Power ----
        EXPECT_CALL(*mockMonitor, isPoweredOn())
            .WillOnce(testing::Return(false)); // set return OFF

        // ---- presence-chassis1 ----
        auto& presenceGpio = getMockGpio(chassis, 0);
        EXPECT_CALL(presenceGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, getValue())
            .WillOnce(testing::Return(1)); // set return present
        EXPECT_CALL(presenceGpio, release()).Times(1);

        // ---- power-chs1-sb-fault-unlatched ----
        auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
        EXPECT_CALL(faultUnlatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, getValue())
            .WillOnce(testing::Return(0)); // set return disabled
        EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-latched ----
        auto& faultLatchedGpio = getMockGpio(chassis, 2);
        EXPECT_CALL(faultLatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultLatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultLatchedGpio, getValue())
            .WillOnce(testing::Return(1)); // set return enabled
        EXPECT_CALL(faultLatchedGpio, release()).Times(0);

        // ---- reset-enable-chs1-sb-power ----
        auto& resetEnableGpio = getMockGpio(chassis, 3);
        EXPECT_CALL(resetEnableGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, requestWrite(0)) // set return disabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, setValue(0))     // set return disabled
            .Times(1);
        EXPECT_CALL(resetEnableGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-reset ----
        auto& faultResetGpio = getMockGpio(chassis, 4);
        EXPECT_CALL(faultResetGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, requestWrite(1)) // set return enabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, setValue(1))     // set return enabled
            .Times(1);
        EXPECT_CALL(faultResetGpio, release()).Times(0);

        chassis.handleBMCReset();

        // ---- ChassisState ----
        EXPECT_EQ(chassis.getState(), ChassisState::Off); // expect Off

        // ---- PowerSystemInputs status ----
        // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
        //           PowerSystemInputs::Status::????); // expect ????
    }

    {
        // (2.3) Test Present sled, power-chsX-sb-fault-unlatched fault,
        //                          D-Bus reports chassis OFF:
        // + chassis power                      not-checked
        // + presence-chassis1                  enabled
        // + power-chs1-sb-fault-unlatched      enabled
        // + power-chs1-sb-fault-latched        not-checked
        //
        // - reset-enable-chs1-sb-power         expect disabled
        // - power-chs1-sb-fault-reset          expect enabled
        // - ChassisState                       expect Faulted
        // - PowerSystemInputs status           expect Fault
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        // ---- chassis power ----
        auto monitorOwner =
            std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
        auto* mockMonitor = monitorOwner.get();
        chassis.setChassisStatusMonitor(std::move(monitorOwner));
        EXPECT_CALL(*mockMonitor, isPoweredOn()).Times(0); // not-checked

        // ---- presence-chassis1 ----
        auto& presenceGpio = getMockGpio(chassis, 0);
        EXPECT_CALL(presenceGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, getValue())
            .WillOnce(testing::Return(1)); // set return present
        EXPECT_CALL(presenceGpio, release()).Times(1);

        // ---- power-chs1-sb-fault-unlatched ----
        auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
        EXPECT_CALL(faultUnlatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, getValue())
            .WillOnce(testing::Return(1)); // set return enabled
        EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-latched ----
        auto& faultLatchedGpio = getMockGpio(chassis, 2);
        EXPECT_CALL(faultLatchedGpio, release()).Times(0); // not-checked

        // ---- reset-enable-chs1-sb-power ----
        auto& resetEnableGpio = getMockGpio(chassis, 3);
        EXPECT_CALL(resetEnableGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, requestWrite(0)) // set return disabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, setValue(0))     // set return disabled
            .Times(1);                                // expect disabled(0)
        EXPECT_CALL(resetEnableGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-reset ----
        auto& faultResetGpio = getMockGpio(chassis, 4);
        EXPECT_CALL(faultResetGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, requestWrite(1)) // set return enabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, setValue(1))     // set return enabled
            .Times(1);
        EXPECT_CALL(faultResetGpio, release()).Times(0);

        chassis.handleBMCReset();

        // ---- ChassisState ----
        EXPECT_EQ(chassis.getState(), ChassisState::Faulted); // expect Faulted

        // ---- PowerSystemInputs status ----
        // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
        //           PowerSystemInputs::Status::Fault); // expect Fault
    }
}

TEST_F(ChassisTests, HandleBMCReset_PresentSledChassisOn)
{
    {
        // (3.1) Test Present sled, no faults, D-Bus reports chassis ON:
        // + chassis power                      On
        // + presence-chassis1                  enabled
        // + power-chs1-sb-fault-unlatched      disabled
        // + power-chs1-sb-fault-latched        disabled
        //
        // - Status Monitor Power               expect ON
        // - reset-enable-chs1-sb-power         expect enabled
        // - power-chs1-sb-fault-reset          expect disabled
        // - power-chs1-sb-fault-unlatched      expect no-fault
        // - ChassisState                       expect On
        // - PowerSystemInputs status           expect Good
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        // Initialize PowerSystemInputs interface (needed by
        // updatePowerSystemInputsStatus)
        chassis.initializePowerSystemInputsInterface(
            PowerSystemInputs::Status::Good);

        // ---- chassis power ----
        auto monitorOwner =
            std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
        auto* mockMonitor = monitorOwner.get();
        chassis.setChassisStatusMonitor(std::move(monitorOwner));
        EXPECT_CALL(*mockMonitor, isPoweredOn())
            .WillOnce(testing::Return(true)); // set return On

        // ---- presence-chassis1 ----
        auto& presenceGpio = getMockGpio(chassis, 0);
        EXPECT_CALL(presenceGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, getValue())
            .WillOnce(testing::Return(1)); // set return present
        EXPECT_CALL(presenceGpio, release()).Times(1);

        // ---- power-chs1-sb-fault-unlatched ----
        auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
        EXPECT_CALL(faultUnlatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, getValue())
            .WillOnce(testing::Return(0)); // set return disabled
        EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-latched ----
        auto& faultLatchedGpio = getMockGpio(chassis, 2);
        EXPECT_CALL(faultLatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultLatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultLatchedGpio, getValue())
            .WillOnce(testing::Return(0)); // set return disabled
        EXPECT_CALL(faultLatchedGpio, release()).Times(0);

        // ---- reset-enable-chs1-sb-power ----
        auto& resetEnableGpio = getMockGpio(chassis, 3);
        EXPECT_CALL(resetEnableGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, requestWrite(1)) // set return enabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, setValue(1))     // set return enabled
            .Times(1);
        EXPECT_CALL(resetEnableGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-reset ----
        auto& faultResetGpio = getMockGpio(chassis, 4);
        EXPECT_CALL(faultResetGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, requestWrite(0)) // set return disable
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, setValue(0))     // set return disable
            .Times(1);
        EXPECT_CALL(faultResetGpio, release()).Times(0);

        chassis.handleBMCReset();

        // ---- ChassisState ----
        EXPECT_EQ(chassis.getState(), ChassisState::On); // expect On

        // ---- PowerSystemInputs status ----
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good); // expect Good
    }

    {
        // (3.2) Test Present sled, power-chsX-sb-fault-unlatched faulted
        //                        D-Bus reports chassis ON:
        // + chassis power                      Not-checked
        // + presence-chassis1                  enabled
        // + power-chs1-sb-fault-unlatched      enabled *****************
        // + power-chs1-sb-fault-latched        disabled
        //
        // - Status Monitor Power               expect NOT called
        // - reset-enable-chs1-sb-power         expect enabled
        // - power-chs1-sb-fault-reset          expect disabled
        // - currentState                       expect Faulted
        // - PowerSystemInputs status           expect Fault
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        // Initialize PowerSystemInputs interface (needed by
        // updatePowerSystemInputsStatus)
        chassis.initializePowerSystemInputsInterface(
            PowerSystemInputs::Status::Good);

        // ---- chassis power ----
        auto monitorOwner =
            std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
        auto* mockMonitor = monitorOwner.get();
        chassis.setChassisStatusMonitor(std::move(monitorOwner));
        // ---- Status Monitor Power ----
        EXPECT_CALL(*mockMonitor, isPoweredOn()).Times(0); // not-checked

        // ---- presence-chassis1 ----
        auto& presenceGpio = getMockGpio(chassis, 0);
        EXPECT_CALL(presenceGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, getValue())
            .WillOnce(testing::Return(1)); // set return present
        EXPECT_CALL(presenceGpio, release()).Times(1);

        // ---- power-chs1-sb-fault-unlatched ----
        auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
        EXPECT_CALL(faultUnlatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, getValue())
            .WillOnce(testing::Return(1)); // set return enabled
        EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-latched ----
        auto& faultLatchedGpio = getMockGpio(chassis, 2);
        EXPECT_CALL(faultLatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultLatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultLatchedGpio, getValue())
            .WillOnce(testing::Return(0)); // set return disabled
        EXPECT_CALL(faultLatchedGpio, release()).Times(0);

        // ---- reset-enable-chs1-sb-power ----
        auto& resetEnableGpio = getMockGpio(chassis, 3);
        EXPECT_CALL(resetEnableGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, requestWrite(0)) // set return disabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, setValue(0))     // set return disabled
            .Times(1);
        EXPECT_CALL(resetEnableGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-reset: disable ----
        auto& faultResetGpio = getMockGpio(chassis, 4);
        EXPECT_CALL(faultResetGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, requestWrite(1))       // set return enabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, setValue(1)).Times(1); // set return enabled
        EXPECT_CALL(faultResetGpio, release()).Times(0);

        chassis.handleBMCReset();

        // ---- ChassisState: Faulted ----
        EXPECT_EQ(chassis.getState(), ChassisState::Faulted); // expect Faulted

        // ---- PowerSystemInputs status ----
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Fault); // expect Fault
    }

    {
        // (3.3) Test Present sled, power-chs1-sb-fault-latched faulted,
        //                        D-Bus reports chassis ON:
        // + chassis power                      On
        // + presence-chassis1                  enabled
        // + power-chs1-sb-fault-unlatched      disabled
        // + power-chs1-sb-fault-latched        enabled
        //
        // - Status Monitor Power               expect On
        // - reset-enable-chs1-sb-power         expect enabled
        // - power-chs1-sb-fault-reset          expect disabled
        // - currentState                       expect On
        // - PowerSystemInputs status           expect Good
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        // Initialize PowerSystemInputs interface (needed by
        // updatePowerSystemInputsStatus)
        chassis.initializePowerSystemInputsInterface(
            PowerSystemInputs::Status::Good);

        // ---- chassis power ----
        auto monitorOwner =
            std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
        auto* mockMonitor = monitorOwner.get();
        chassis.setChassisStatusMonitor(std::move(monitorOwner));
        // ---- Status Monitor Power ----
        EXPECT_CALL(*mockMonitor, isPoweredOn())
            .WillOnce(testing::Return(true)); // set return on

        // ---- presence-chassis1 ----
        auto& presenceGpio = getMockGpio(chassis, 0);
        EXPECT_CALL(presenceGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(presenceGpio, getValue())
            .WillOnce(testing::Return(1)); // set return present
        EXPECT_CALL(presenceGpio, release()).Times(1);

        // ---- power-chs1-sb-fault-unlatched ----
        auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
        EXPECT_CALL(faultUnlatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, getValue())
            .WillOnce(testing::Return(0)); // set return disabled
        EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-latched ----
        auto& faultLatchedGpio = getMockGpio(chassis, 2);
        EXPECT_CALL(faultLatchedGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultLatchedGpio, requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultLatchedGpio, getValue())
            .WillOnce(testing::Return(1)); // set return enabled
        EXPECT_CALL(faultLatchedGpio, release()).Times(0);

        // ---- reset-enable-chs1-sb-power ----
        auto& resetEnableGpio = getMockGpio(chassis, 3);
        EXPECT_CALL(resetEnableGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, requestWrite(1)) // set return enabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, setValue(1))     // set return enabled
            .Times(1);
        EXPECT_CALL(resetEnableGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-reset: disable ----
        auto& faultResetGpio = getMockGpio(chassis, 4);
        EXPECT_CALL(faultResetGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, requestWrite(0)) // set return disabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, setValue(0))     // set return disabled
            .Times(1);
        EXPECT_CALL(faultResetGpio, release()).Times(0);

        chassis.handleBMCReset();

        // ---- ChassisState ----
        EXPECT_EQ(chassis.getState(), ChassisState::On); // expect On

        // ---- PowerSystemInputs status ----
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good); // expect Good
    }
}

TEST_F(ChassisTests, HandleBMCReset_60SecTimer_NulloptPowerStatus)
{
    {
        // (4.1) Test Present sled, no faults, isChassisPoweredOn() returns
        //       nullopt(status monitor throws), first attempt — 60-second timer
        //       is started:
        //
        // + presence-chassis1                  enabled (present)
        // + power-chs1-sb-fault-unlatched      disabled (no fault)
        // + power-chs1-sb-fault-latched        disabled (no fault)
        // + isChassisPoweredOn()               returns std::nullopt (monitor
        // throws)
        // + bmcResetRetryTimerUsed             false (first attempt)
        //
        // chassis.handleBMCReset()
        // - Status Monitor Power               expect throw exception
        // - reset-enable-chs1-sb-power         expect NOT written
        // - power-chs1-sb-fault-reset          expect NOT written
        // - ChassisState                       expect Missing
        // - PowerSystemInputs status           ??????
        //
        // chassis.handleBMCResetTimerCallback()
        // - Status Monitor Power               expect ON
        // - power-chs1-sb-fault-unlatched      expect no-fault
        // - reset-enable-chs1-sb-power         expect enabled
        // - power-chs1-sb-fault-reset          expect disabled
        // - ChassisState                       expect On
        // - PowerSystemInputs status           ??????
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        // ---- ChassisState ----
        auto monitorOwner =
            std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
        auto* mockMonitor = monitorOwner.get();
        chassis.setChassisStatusMonitor(std::move(monitorOwner));
        EXPECT_CALL(*mockMonitor, isPoweredOn())
            // handleBMCReset() would expect Throw error
            .WillOnce(
                testing::Throw(std::runtime_error("D-Bus not yet available")))
            // handleBMCResetTimerCallback() would expect Power on
            .WillOnce(testing::Return(true));

        // ---- presence-chassis1 ----
        auto& presenceGpio = getMockGpio(chassis, 0);
        EXPECT_CALL(presenceGpio, foundLine())
            // Runs pre-check: and safety re-check: multiple times.
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(presenceGpio, requestRead())
            // Runs multiple times.
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(presenceGpio, getValue())
            // Runs multiple times.
            .WillRepeatedly(testing::Return(1)); // set return present
        EXPECT_CALL(presenceGpio, release()).Times(2);

        // ---- power-chs1-sb-fault-unlatched ----
        auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
        EXPECT_CALL(faultUnlatchedGpio, foundLine())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, requestRead())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(faultUnlatchedGpio, getValue())
            .WillRepeatedly(testing::Return(0)); // set return disabled
        EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);

        // ---- reset-enable-chs1-sb-power ----
        auto& resetEnableGpio = getMockGpio(chassis, 3);
        EXPECT_CALL(resetEnableGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, requestWrite(1)) // set return enabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(resetEnableGpio, setValue(1))     // set return enabled
            .Times(1);
        EXPECT_CALL(resetEnableGpio, release()).Times(0);

        // ---- power-chs1-sb-fault-reset ----
        auto& faultResetGpio = getMockGpio(chassis, 4);
        EXPECT_CALL(faultResetGpio, foundLine())
            // pre-check avoid calling findLine() unnecessarily,
            .WillOnce(testing::Return(true))
            // safety re-check confirm the line available before using it
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, requestWrite(0)) // set return disabled
            .WillOnce(testing::Return(true));
        EXPECT_CALL(faultResetGpio, setValue(0))     // set return disabled
            .Times(1);
        EXPECT_CALL(faultResetGpio, release()).Times(0);

        // #####################################################################
        chassis.handleBMCReset();

        // Could not read power state is set to Missing while waiting for timer
        EXPECT_EQ(chassis.getState(), ChassisState::Missing);

        // #####################################################################
        chassis.handleBMCResetTimerCallback();

        // Reads power on
        EXPECT_EQ(chassis.getState(), ChassisState::On);

        // ---- PowerSystemInputs status ----
        // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
        //           PowerSystemInputs::Status::Good); // expect Good
    }
}
