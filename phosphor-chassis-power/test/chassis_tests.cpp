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
 * @param prevValue Previous GPIO value (optional; if not set, getPreviousValue
 *                  expectation is skipped)
 */
void expectPresenceGpio(Chassis& chassis, int value,
                        std::optional<int> prevValue = std::nullopt)
{
    auto& gpio = getMockGpio(chassis, 0);
    EXPECT_CALL(gpio, foundLine()).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, requestRead()).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, getValue()).WillOnce(testing::Return(value));
    if (prevValue.has_value())
    {
        EXPECT_CALL(gpio, getPreviousValue())
            .WillOnce(testing::Return(*prevValue));
    }
    EXPECT_CALL(gpio, release()).Times(1);
}

/**
 * Helper to set repeated GPIO read expectations for presence detection.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=present, 0=absent)
 * @param prevValue Previous GPIO value (optional; if not set, getPreviousValue
 *                  expectation is skipped)
 */
void expectPresenceGpioRepeated(Chassis& chassis, int value,
                                std::optional<int> prevValue = std::nullopt)
{
    auto& gpio = getMockGpio(chassis, 0);
    EXPECT_CALL(gpio, foundLine()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(gpio, requestRead()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(gpio, getValue()).WillRepeatedly(testing::Return(value));
    if (prevValue.has_value())
    {
        EXPECT_CALL(gpio, getPreviousValue())
            .WillRepeatedly(testing::Return(*prevValue));
    }
    EXPECT_CALL(gpio, release()).WillRepeatedly(testing::Return());
}

/**
 * Helper to set GPIO read expectations for power-chs1-sb-fault-unlatched.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=enabled, 0=disabled)
 */
void expectSbFaultUnLatched(Chassis& chassis, int value)
{
    auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
    EXPECT_CALL(faultUnlatchedGpio, foundLine())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(faultUnlatchedGpio, requestRead())
        .WillOnce(testing::Return(true));
    EXPECT_CALL(faultUnlatchedGpio, getValue())
        .WillOnce(testing::Return(value)); // set return
    EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);
}

/**
 * Helper to not expect GPIO read of power-chs1-sb-fault-unlatched.
 *
 * @param chassis Chassis containing the GPIO
 */
void unExpectSbFaultUnLatched(Chassis& chassis)
{
    auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
    EXPECT_CALL(faultUnlatchedGpio, foundLine()).Times(0);
    EXPECT_CALL(faultUnlatchedGpio, requestRead()).Times(0);
    EXPECT_CALL(faultUnlatchedGpio, getValue()).Times(0);
    EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);
}

/**
 * Helper to set GPIO read expectations for power-chs1-sb-fault-unlatched.
 *
 * @param chassis Chassis containing the GPIO
 * @param valueRead1 first read GPIO value to return (1=enabled, 0=disabled)
 * @param valueRead2 second read GPIO value to return (1=enabled, 0=disabled)
 */
void expectSbFaultUnLatched_read2(Chassis& chassis, int valueRead1,
                                  int valueRead2)
{
    auto& faultUnlatchedGpio = getMockGpio(chassis, 1);
    EXPECT_CALL(faultUnlatchedGpio, foundLine())
        // pre-check avoid calling findLine() unnecessarily,
        .WillOnce(testing::Return(true))
        // safety re-check confirm the line available before using it
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(faultUnlatchedGpio, requestRead())
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(faultUnlatchedGpio, getValue())
        .WillOnce(testing::Return(valueRead1))
        .WillOnce(testing::Return(valueRead2)); // set return
    EXPECT_CALL(faultUnlatchedGpio, release()).Times(0);
}

/**
 * Helper to set GPIO read expectations for power-chs1-sb-fault-latched.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=enabled, 0=disabled)
 */
void expectSbFaultLatched(Chassis& chassis, int value)
{
    auto& faultlatchedGpio = getMockGpio(chassis, 2);
    EXPECT_CALL(faultlatchedGpio, foundLine())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(faultlatchedGpio, requestRead())
        .WillOnce(testing::Return(true));
    EXPECT_CALL(faultlatchedGpio, getValue())
        .WillOnce(testing::Return(value)); // set return
    EXPECT_CALL(faultlatchedGpio, release()).Times(0);
}

/**
 * Helper to not expect GPIO read of power-chs1-sb-fault-latched.
 *
 * @param chassis Chassis containing the GPIO
 */
void unExpectSbFaultLatched(Chassis& chassis)
{
    auto& faultlatchedGpio = getMockGpio(chassis, 2);
    // EXPECT_CALL(faultlatchedGpio, foundLine()).Times(0);
    EXPECT_CALL(faultlatchedGpio, requestRead()).Times(0);
    EXPECT_CALL(faultlatchedGpio, getValue()).Times(0);
    EXPECT_CALL(faultlatchedGpio, release()).Times(0);
}

// SHELDON:TODO: make 2nd optional, and combine with expectSbFaultLatched!!
/**
 * Helper to set GPIO read expectations for power-chs1-sb-fault-latched.
 *
 * @param chassis Chassis containing the GPIO
 * @param valueRead1 first read GPIO value to return (1=enabled, 0=disabled)
 * @param valueRead2 second read GPIO value to return (1=enabled, 0=disabled)
 */
void expectSbFaultLatched_read2(Chassis& chassis, int valueRead1,
                                int valueRead2)
{
    auto& faultlatchedGpio = getMockGpio(chassis, 2);
    EXPECT_CALL(faultlatchedGpio, foundLine())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(faultlatchedGpio, requestRead())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(faultlatchedGpio, getValue())
        .WillOnce(testing::Return(valueRead1))
        .WillOnce(testing::Return(valueRead2)); // set return
    EXPECT_CALL(faultlatchedGpio, release()).Times(0);
}

/**
 * Helper to set GPIO read expectations for reset-enable-chs1-sb-power.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=enabled, 0=disabled)
 */
void expectResetEnable(Chassis& chassis, int value)
{
    auto& resetEnableGpio = getMockGpio(chassis, 3);
    EXPECT_CALL(resetEnableGpio, foundLine())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(resetEnableGpio, requestWrite(value)) // set return
        .WillOnce(testing::Return(true));
    EXPECT_CALL(resetEnableGpio, setValue(value))     // set return
        .Times(1);
    EXPECT_CALL(resetEnableGpio, release()).Times(0);
}

/**
 * Helper to set GPIO read expectations for reset-enable-chs1-sb-power.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=enabled, 0=disabled)
 */
void expectResetEnableRepeated(Chassis& chassis, int value)
{
    auto& resetEnableGpio = getMockGpio(chassis, 3);
    EXPECT_CALL(resetEnableGpio, foundLine())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(resetEnableGpio, requestWrite(value))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(resetEnableGpio, setValue(value));
    EXPECT_CALL(resetEnableGpio, release()).Times(0);
}

/**
 * Helper to set GPIO read expectations for power-chs1-sb-fault-reset.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=enabled, 0=disabled)
 */
void expectFaultReset(Chassis& chassis, int value)
{
    auto& faultResetGpio = getMockGpio(chassis, 4);
    EXPECT_CALL(faultResetGpio, foundLine())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(faultResetGpio, requestWrite(value)) // set return
        .WillOnce(testing::Return(true));
    EXPECT_CALL(faultResetGpio, setValue(value))     // set return
        .Times(1);
    EXPECT_CALL(faultResetGpio, release()).Times(0);
}

/**
 * Helper to set GPIO read expectations for power-chs1-sb-fault-reset.
 *
 * @param chassis Chassis containing the GPIO
 * @param value GPIO value to return (1=enabled, 0=disabled)
 */
void expectFaultResetRepeated(Chassis& chassis, int value)
{
    auto& faultResetGpio = getMockGpio(chassis, 4);
    EXPECT_CALL(faultResetGpio, foundLine())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(faultResetGpio, requestWrite(value)) // set return
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(faultResetGpio, setValue(value));    // set return
    EXPECT_CALL(faultResetGpio, release()).Times(0);
}

/**
 * Helper to set Monitor isPoweredOn() as powered on/off
 *
 * @param chassis Chassis containing the GPIO
 * @param state bool to return (true=enabled, false=disabled)
 */
void expectPowerState(Chassis& chassis, bool state)
{
    auto monitorOwner =
        std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
    auto* mockMonitor = monitorOwner.get();
    chassis.setChassisStatusMonitor(std::move(monitorOwner));
    EXPECT_CALL(*mockMonitor, isPoweredOn()).WillOnce(testing::Return(state));
}

/**
 * Helper to Monitor isPoweredOn() is not called.
 *
 * @param chassis Chassis containing the monitor
 */
void expectNoPowerCheck(Chassis& chassis)
{
    auto monitorOwner =
        std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
    auto* mockMonitor = monitorOwner.get();
    chassis.setChassisStatusMonitor(std::move(monitorOwner));
    EXPECT_CALL(*mockMonitor, isPoweredOn()).Times(0);
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
 * @param services    MockServices instance (must outlive the returned Chassis)
 * @param event       sdeventplus::Event instance
 * @param tempPathOut Optional pointer to a path variable. When non-null, a
 *                    temporary presence file is created and its path is stored
 *                    in *tempPathOut so the caller can remove it afterwards.
 *                    When null (default), no presence file is created and the
 *                    Chassis is built without a presence path.
 * @return Chassis configured with or without a presence path
 */
Chassis buildSledChassis(MockServices& services, sdeventplus::Event& event,
                         std::filesystem::path* tempPathOut = nullptr)
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

    if (tempPathOut != nullptr)
    {
        return Chassis{1, services, event, tempPathOut->string(),
                       std::move(gpios)};
    }
    else
    {
        return Chassis{1, services, event, std::nullopt, std::move(gpios)};
    }
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
        MockServices services{};
        std::filesystem::path tempPath;
        Chassis chassis = buildSledChassis(services, event, &tempPath);

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        expectPresenceGpio(chassis, 1, 1);

        // #####################################################################
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
        Chassis chassis = buildSledChassis(services, event);

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
        Chassis chassis = buildSledChassis(services, event);

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
        auto tempPath =
            std::filesystem::temp_directory_path() / "test_presence";
        Chassis chassis = buildSledChassis(services, event, &tempPath);

        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        expectPresenceGpio(chassis, 1, 1);

        // Monitor to update GPIO value and handle presence change
        chassis.monitor();

        // Verify chassis is present (GPIO says present)
        EXPECT_TRUE(chassis.getPresenceValue());

        std::filesystem::remove(tempPath);
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

TEST_F(ChassisTests, CheckLatchedFault)
{
    // Test where fault-latched GPIO has not yet been read; fault is detected
    // on the next monitor tick
    {
        MockServices services;
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-latched",
                                GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(1));

        EXPECT_CALL(
            services,
            logError(
                "xyz.openbmc_project.Power.BMC.Reset.ChassisPreviouslyLostPower",
                Entry::Level::Error, testing::_))
            .Times(1);

        chassis.startLatchedFaultCheck();
        chassis.monitor();
    }

    // Test where fault-latched value is 0, no fault action taken
    {
        MockServices services;
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-latched",
                                GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        // Prime the cached value to 0 via monitor()
        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(0));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(0));

        EXPECT_CALL(services, logError).Times(0);

        chassis.monitor();
        ASSERT_EQ(chassis.getFaultLatchedValue(), 0);
        chassis.startLatchedFaultCheck();
    }

    // Test where fault-latched value is 1, calls handleLatchedFault
    {
        MockServices services;
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-latched",
                                GpioDirection::Input, GpioPolarity::Low));
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-reset",
                                GpioDirection::Output, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(1));
        chassis.monitor();
        ASSERT_EQ(chassis.getFaultLatchedValue(), 1);

        EXPECT_CALL(getMockGpio(chassis, 1), foundLine())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), requestWrite(1))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), setValue(1)).Times(1);
        EXPECT_CALL(getMockGpio(chassis, 1), requestWrite(0))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), setValue(0)).Times(1);
        EXPECT_CALL(getMockGpio(chassis, 1), release()).Times(1);
        EXPECT_CALL(
            services,
            logError(
                "xyz.openbmc_project.Power.BMC.Reset.ChassisPreviouslyLostPower",
                Entry::Level::Error, testing::_))
            .Times(1);

        chassis.startLatchedFaultCheck();
    }
}

TEST_F(ChassisTests, HandleLatchedFault)
{
    // Test where fault-reset GPIO is not configured — logs PEL, returns true
    {
        MockServices services;
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-latched",
                                GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(1));
        chassis.monitor();
        ASSERT_EQ(chassis.getFaultLatchedValue(), 1);

        EXPECT_CALL(
            services,
            logError(
                "xyz.openbmc_project.Power.BMC.Reset.ChassisPreviouslyLostPower",
                Entry::Level::Error, testing::_))
            .Times(1);

        chassis.startLatchedFaultCheck();
    }

    // Test where reset GPIO write fails, returns false, retries on next
    // monitor() tick, PEL logged only once across both attempts
    {
        MockServices services;
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-latched",
                                GpioDirection::Input, GpioPolarity::Low));
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-reset",
                                GpioDirection::Output, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1))
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(1))
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 1), foundLine())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), requestWrite(1))
            .WillOnce(testing::Return(false))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), setValue(1)).Times(1);
        EXPECT_CALL(getMockGpio(chassis, 1), requestWrite(0))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), setValue(0)).Times(1);
        EXPECT_CALL(getMockGpio(chassis, 1), release()).Times(1);
        EXPECT_CALL(
            services,
            logError(
                "xyz.openbmc_project.Power.BMC.Reset.ChassisPreviouslyLostPower",
                Entry::Level::Error, testing::_))
            .Times(1);
        chassis.startLatchedFaultCheck();
        chassis.monitor();
        chassis.monitor();
    }

    // Test where reset GPIO write succeeds, returns true, PEL logged
    {
        MockServices services;
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-latched",
                                GpioDirection::Input, GpioPolarity::Low));
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-reset",
                                GpioDirection::Output, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_CALL(getMockGpio(chassis, 0), foundLine())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), requestRead())
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 0), getValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 0), getPreviousValue())
            .WillOnce(testing::Return(1));
        EXPECT_CALL(getMockGpio(chassis, 1), foundLine())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), requestWrite(1))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), setValue(1)).Times(1);
        EXPECT_CALL(getMockGpio(chassis, 1), requestWrite(0))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(getMockGpio(chassis, 1), setValue(0)).Times(1);
        EXPECT_CALL(getMockGpio(chassis, 1), release()).Times(1);
        EXPECT_CALL(
            services,
            logError(
                "xyz.openbmc_project.Power.BMC.Reset.ChassisPreviouslyLostPower",
                Entry::Level::Error, testing::_))
            .Times(1);

        chassis.startLatchedFaultCheck();
        chassis.monitor();
    }
}

TEST_F(ChassisTests, GetGpioByName)
{
    // Test where no GPIO name contains the substring — returns nullptr
    {
        MockServices services;
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(services.createGPIO(
            "presence-chassis1", GpioDirection::Input, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_EQ(chassis.getGpioByName("fault-reset"), nullptr);
    }

    // Test where GPIO name contains the substring — returns that GPIO
    {
        MockServices services;
        std::vector<std::unique_ptr<Gpio>> gpios{};
        gpios.emplace_back(
            services.createGPIO("power-chs1-sb-fault-reset",
                                GpioDirection::Output, GpioPolarity::Low));

        Chassis chassis{1, services, event, std::nullopt, std::move(gpios)};

        EXPECT_EQ(chassis.getGpioByName("fault-reset"),
                  chassis.getGpios()[0].get());
    }
}

TEST_F(ChassisTests, HandleBMCReset_MissingSled)
{
    // Test For missing sleds (isPresent() returns false):
    {
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        expectNoPowerCheck(chassis);

        expectPresenceGpioRepeated(chassis, 0, 0);

        // ##################################################################
        // COMMON
        // ##################################################################
        unExpectSbFaultLatched(chassis);

        // Validate reset-enable-chs1-sb-power  disabled
        expectResetEnableRepeated(chassis, 0);

        // Validate power-chs1-sb-fault-reset   enabled
        expectFaultResetRepeated(chassis, 1);

        // ##################################################################
        // BMC reset
        // ##################################################################
        unExpectSbFaultUnLatched(chassis);

        chassis.handleBMCReset();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Missing);

        // Validatge PowerSystemInputs D-Bus interface does not exist:
        //    Confirms it did not get set to Good or Fault.
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        // ##################################################################
        // 1 Second update timer
        // ##################################################################
        expectSbFaultUnLatched(chassis, 0);

        chassis.monitor();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Missing);

        // Validatge PowerSystemInputs D-Bus interface set:
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }
}

TEST_F(ChassisTests, HandleBMCReset_PresentSled_ChassisOff)
{
    // Test Present sled, no faults, D-Bus reports chassis OFF:
    {
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        expectPowerState(chassis, false);

        expectPresenceGpioRepeated(chassis, 1, 0);

        // ##################################################################
        // COMMON
        // ##################################################################
        expectSbFaultLatched(chassis, 0);

        // Validate reset-enable-chs1-sb-power  disabled
        expectResetEnableRepeated(chassis, 0);

        // Validate power-chs1-sb-fault-reset   enabled
        expectFaultResetRepeated(chassis, 1);

        // ##################################################################
        // BMC reset
        // ##################################################################
        unExpectSbFaultUnLatched(chassis);

        chassis.handleBMCReset();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Off);

        // Validatge PowerSystemInputs D-Bus interface does not exist:
        //    Confirms it did not get set to Good or Fault.
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        // ##################################################################
        // 1 Second update timer test.
        // ##################################################################
        expectSbFaultUnLatched(chassis, 0);

        chassis.monitor();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Off);

        // Validatge PowerSystemInputs D-Bus interface set:
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }

    // Test Present sled, power-chs1-sb-fault-latched fault,
    //                          D-Bus reports chassis OFF:
    {
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        expectPowerState(chassis, false);

        expectPresenceGpioRepeated(chassis, 1, 0);

        // ##################################################################
        //  COMMON
        // ##################################################################
        expectSbFaultLatched(chassis, 1);

        // Validate reset-enable-chs1-sb-power  disabled
        expectResetEnable(chassis, 0);

        // Validate power-chs1-sb-fault-reset   enabled
        expectFaultReset(chassis, 1);

        // ##################################################################
        //  BMC reset
        // ##################################################################
        unExpectSbFaultUnLatched(chassis);

        chassis.handleBMCReset();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Off);

        // Validatge PowerSystemInputs D-Bus interface does not exist:
        //    Confirms it did not get set to Good or Fault.
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        // ##################################################################
        //  1 Second update timer test.
        //  #################################################################
        expectSbFaultUnLatched(chassis, 0);

        chassis.monitor();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Off);

        // Validatge PowerSystemInputs D-Bus interface set:
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }

    // Test Present sled, power-chsX-sb-fault-unlatched fault,
    //                          D-Bus reports chassis OFF:
    {
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        expectPresenceGpioRepeated(chassis, 1, 0);

        expectPowerState(chassis, false);

        // ##################################################################
        //  COMMON
        // ##################################################################
        unExpectSbFaultLatched(chassis);

        // ##################################################################
        //  BMC reset
        // ##################################################################
        unExpectSbFaultUnLatched(chassis);

        // SHELDON:QUESTION: when powered off this gets called 2. ????
        // Validate reset-enable-chs1-sb-power  disabled
        expectResetEnableRepeated(chassis, 0);

        // Validate power-chs1-sb-fault-reset   enabled
        expectFaultResetRepeated(chassis, 1);

        chassis.handleBMCReset();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Off);

        // Validatge PowerSystemInputs D-Bus interface does not exist:
        //    Confirms it did not get set to Good or Fault.
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        // ##################################################################
        //  1 Second update timer test.
        //  #################################################################
        expectSbFaultUnLatched(chassis, 1);

        // Validate reset-enable-chs1-sb-power  disabled
        expectResetEnableRepeated(chassis, 0);

        // Validate power-chs1-sb-fault-reset   enabled
        expectFaultResetRepeated(chassis, 1);

        chassis.monitor();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Faulted);

        // Validatge PowerSystemInputs D-Bus interface set:
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Fault);
    }
}

TEST_F(ChassisTests, HandleBMCReset_PresentSled_ChassisOn)
{
    // Test Present sled, no faults, D-Bus reports chassis ON:
    {
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        expectPowerState(chassis, true);

        expectPresenceGpioRepeated(chassis, 1, 0);

        // ##################################################################
        //  COMMON
        // ##################################################################
        expectSbFaultUnLatched(chassis, 0);

        expectSbFaultLatched(chassis, 0);

        // Validate reset-enable-chs1-sb-power  enabled
        expectResetEnableRepeated(chassis, 1);

        // Validate power-chs1-sb-fault-reset   disabled
        expectFaultResetRepeated(chassis, 0);

        // ##################################################################
        //  BMC reset
        // ##################################################################
        chassis.handleBMCReset();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::On);

        // Validatge PowerSystemInputs D-Bus interface does not exist:
        //    Confirms it did not get set to Good or Fault.
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        // ##################################################################
        //  1 Second update timer test.
        //  #################################################################
        chassis.monitor();

        // // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::On);

        // // Validatge PowerSystemInputs D-Bus interface set:
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }

    //     // Test Present sled, power-chs1-sb-fault-latched faulted,
    //     //                        D-Bus reports chassis ON:
    {
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        expectPowerState(chassis, true);

        expectPresenceGpioRepeated(chassis, 1, 0);

        // ##################################################################
        //  COMMON
        // ##################################################################
        expectSbFaultUnLatched(chassis, 0);

        expectSbFaultLatched(chassis, 1);

        // Validate reset-enable-chs1-sb-power  enabled
        expectResetEnable(chassis, 1);

        // Validate power-chs1-sb-fault-reset   disabled
        expectFaultReset(chassis, 0);

        // ##################################################################
        //  BMC reset
        // ##################################################################
        chassis.handleBMCReset();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::On);

        // Validatge PowerSystemInputs D-Bus interface does not exist:
        //    Confirms it did not get set to Good or Fault.
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        // ##################################################################
        //  1 Second update timer test.
        //  #################################################################
        chassis.monitor();

        // // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::On);

        // // Validatge PowerSystemInputs D-Bus interface set:
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Good);
    }

    // Test Present sled, power-chsX-sb-fault-unlatched faulted
    //                        D-Bus reports chassis ON:
    {
        MockServices services{};
        Chassis chassis = buildSledChassis(services, event);

        expectPowerState(chassis, true);

        expectPresenceGpioRepeated(chassis, 1, 0);

        // SHELDON:TODO:TEST: remove this section to see if it works.
        auto monitor = services.createChassisStatusMonitor(
            0, "/xyz/openbmc_project/inventory/system/chassis",
            ChassisStatusMonitorOptions{});
        chassis.setSystemStatusMonitor(std::move(monitor));

        // ##################################################################
        //  COMMON
        // ##################################################################
        expectSbFaultUnLatched(chassis, 1);

        expectSbFaultLatched(chassis, 0);

        // ##################################################################
        //  BMC reset
        // ##################################################################
        chassis.handleBMCReset();

        // Validate local Chassis state:
        // This is not Faulted since we need the 1 Sec. timer to cache value.
        EXPECT_EQ(chassis.getState(), ChassisState::On);

        // Validatge PowerSystemInputs D-Bus interface does not exist:
        //    Confirms it did not get set to Good or Fault.
        EXPECT_EQ(chassis.getPowerSystemInputsInterface(), nullptr);

        // ##################################################################
        //  1 Second update timer test.
        //  #################################################################
        // Validate reset-enable-chs1-sb-power  disabled
        expectResetEnable(chassis, 0);

        // Validate power-chs1-sb-fault-reset   enabled
        expectFaultReset(chassis, 1);

        chassis.monitor();

        // Validate local Chassis state:
        EXPECT_EQ(chassis.getState(), ChassisState::Faulted);

        // Validatge PowerSystemInputs D-Bus interface set:
        EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
                  PowerSystemInputs::Status::Fault);
    }
}

// TEST_F(ChassisTests, HandleBMCReset_60SecTimer_NulloptPowerStatus)
// {
//     // Test Present sled, no faults, isChassisPoweredOn() returns
//     //       nullopt(status monitor throws), first attempt — 60-second timer
//     //       is started:
//     { // SHELDON:TEST:7
//         MockServices services{};
//         Chassis chassis = buildSledChassis(services, event);

//         auto monitorOwner =
//             std::make_unique<testing::NiceMock<MockChassisStatusMonitor>>();
//         auto* mockMonitor = monitorOwner.get();
//         chassis.setChassisStatusMonitor(std::move(monitorOwner));
//         EXPECT_CALL(*mockMonitor, isPoweredOn())
//             // handleBMCReset() would expect Throw exception error
//             .WillOnce(
//                 testing::Throw(std::runtime_error("D-Bus not yet
//                 available")))
//             // handleBMCResetTimerCallback() would expect Power on
//             .WillOnce(testing::Return(true));

//         expectPresenceGpioRepeated(chassis, 1);

//         expectSbFaultUnLatched_read2(chassis, 0, 0); // set disabled,
//         disabled

//         expectSbFaultLatched_read2(chassis, 0, 0);   // set disabled,
//         disabled

//         expectResetEnable(chassis, 1);               // enabled

//         expectFaultReset(chassis, 0);                // disabled

//         chassis.handleBMCReset();
//         // - Status Monitor Power               expect throw exception
//         // - reset-enable-chs1-sb-power         expect NOT written
//         // - power-chs1-sb-fault-reset          expect NOT written

//         // Could not read power state is set to Missing while waiting for
//         timer EXPECT_EQ(chassis.getState(), ChassisState::Missing);

//         // Running 2nd test after a an exception thrown.

//         chassis.handleBMCResetTimerCallback();
//         // - power-chs1-sb-fault-unlatched      expect no-fault
//         // - reset-enable-chs1-sb-power         expect enabled
//         // - power-chs1-sb-fault-reset          expect disabled

//         // Reads power on
//         EXPECT_EQ(chassis.getState(), ChassisState::On);

//         // ---- PowerSystemInputs status ----
//         // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
//         //           PowerSystemInputs::Status::Good); // expect Good
//     }
// }

// TEST_F(ChassisTests, HandleBMC_BootErrors)
// {
//     // Test Present sled, FAULT after power on -
//     power-chs1-sb-fault-unlatched.
//     { // SHELDON:TEST:8
//         MockServices services{};
//         Chassis chassis = buildSledChassis(services, event);

//         // Initialize PowerSystemInputs interface (needed by
//         // updatePowerSystemInputsStatus)
//         chassis.initializePowerSystemInputsInterface(
//             PowerSystemInputs::Status::Good);

//         expectPowerState(chassis, true);

//         expectPresenceGpioRepeated(chassis, 1);

//         expectSbFaultUnLatched(chassis, 0); // set return disabled

//         expectSbFaultLatched(chassis, 0);   // set disabled

//         chassis.handleBMCReset();
//         // - Status Monitor Power               expect On
//         // - reset-enable-chs1-sb-power         expect disabled
//         // - power-chs1-sb-fault-reset          expect enabled

//         EXPECT_EQ(chassis.getState(), ChassisState::On);

//         // Running 2nd test after a power on.

//         chassis.handlePowerStateChange(false);
//         // - Status Monitor Power               expect Faulted
//         // - reset-enable-chs1-sb-power         expect disabled
//         // - power-chs1-sb-fault-reset          expect enabled

//         EXPECT_EQ(chassis.getState(), ChassisState::Faulted);

//         // ---- PowerSystemInputs status ----
//         // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
//         //           PowerSystemInputs::Status::Good); // expect Good
//     }

//     // Test Present sled, FAULT - power-chs1-sb-fault-latched.
//     { // SHELDON:TEST:9
//         // SHELDON:HERE:
//         // + presence-chassis1                  enabled (present)
//         // + power-chs1-sb-fault-unlatched      disabled (no fault)
//         // + power-chs1-sb-fault-latched        enabled (fault)
//         //
//         // chassis.handleBMCResetTimerCallback()
//         // - Status Monitor Power               expect Fault
//         // - reset-enable-chs1-sb-power         expect disabled
//         // - power-chs1-sb-fault-reset          expect enabled
//         // - ChassisState                       expect On
//         // - PowerSystemInputs status           ??????
//         MockServices services{};
//         Chassis chassis = buildSledChassis(services, event);

//         // Initialize PowerSystemInputs interface (needed by
//         // updatePowerSystemInputsStatus)
//         chassis.initializePowerSystemInputsInterface(
//             PowerSystemInputs::Status::Good);

//         expectPowerState(chassis, true);

//         expectPresenceGpioRepeated(chassis, 1, 0);

//         expectSbFaultUnLatched(chassis, 0);

//         expectSbFaultLatched(chassis, 1);

//         chassis.handleBMCReset();

//         EXPECT_EQ(chassis.getState(), ChassisState::On);

//         //
//         ####################################################################
//         // chassis.handlePowerStateChange(false); // SHELDON:HERE:TEST:

//         // Reads power on
//         // EXPECT_EQ(chassis.getState(), ChassisState::Faulted); // expect
//         // Faulted

//         // ---- PowerSystemInputs status ----
//         // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
//         //           PowerSystemInputs::Status::Good); // expect Good
//     }
// }

// TEST_F(ChassisTests, handleBMCResetTimerCallback)
// {
//     // Test Present sled boot/RT, FAULT - due to Powered off.
//     { // SHELDON:TEST:10
//         // SHELDON:QUESTION: this test case seems to have missed the boot
//         Poff MockServices services{}; Chassis chassis =
//         buildSledChassis(services, event);

//         expectPowerState(chassis, false);

//         expectPresenceGpioRepeated(chassis, 1, 0);

//         expectSbFaultUnLatched(chassis, 0);

//         expectResetEnable(chassis, 0);

//         expectFaultReset(chassis, 1);

//         chassis.handleBMCResetTimerCallback();

//         EXPECT_EQ(chassis.getState(), ChassisState::Off); //
//         SHELDON:DEBUG:FAULT

//         // ---- PowerSystemInputs status ----
//         // EXPECT_EQ(chassis.getPowerSystemInputsInterface()->status(),
//         //           PowerSystemInputs::Status::Good); // expect Good
//     }
// }
