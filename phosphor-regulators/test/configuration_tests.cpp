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
#include "action.hpp"
#include "chassis.hpp"
#include "configuration.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "i2c_write_byte_action.hpp"
#include "mock_action.hpp"
#include "mock_error_logging.hpp"
#include "mock_journal.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "pmbus_utils.hpp"
#include "pmbus_write_vout_command_action.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "system.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::pmbus_utils;

using ::testing::A;
using ::testing::Ref;
using ::testing::Return;
using ::testing::Throw;
using ::testing::TypedEq;

static const std::string chassisInvPath{
    "/xyz/openbmc_project/inventory/system/chassis"};

TEST(ConfigurationTests, Constructor)
{
    // Test where volts value specified
    {
        std::optional<double> volts{1.3};

        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());

        Configuration configuration(volts, std::move(actions));
        EXPECT_EQ(configuration.getVolts().has_value(), true);
        EXPECT_EQ(configuration.getVolts().value(), 1.3);
        EXPECT_EQ(configuration.getActions().size(), 2);
    }

    // Test where volts value not specified
    {
        std::optional<double> volts{};

        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        Configuration configuration(volts, std::move(actions));
        EXPECT_EQ(configuration.getVolts().has_value(), false);
        EXPECT_EQ(configuration.getActions().size(), 1);
    }
}

// Test for execute(Services&, System&, Chassis&, Device&)
TEST(ConfigurationTests, ExecuteForDevice)
{
    // Test where works: Volts value not specified
    {
        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Configuring vdd_reg")).Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create I2CWriteByteAction with register 0x7C and value 0x0A
        std::unique_ptr<I2CWriteByteAction> action =
            std::make_unique<I2CWriteByteAction>(0x7C, 0x0A);

        // Create mock I2CInterface.  Expect action to write 0x0A to 0x7C.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x7C), TypedEq<uint8_t>(0x0A)))
            .Times(1);

        // Create Configuration with no volts value specified
        std::optional<double> volts{};
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));
        Configuration* configurationPtr = configuration.get();

        // Create Device that contains Configuration
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute Configuration
        configurationPtr->execute(services, system, *chassisPtr, *devicePtr);
    }

    // Test where works: Volts value specified
    {
        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Configuring vdd_reg: volts=1.300000"))
            .Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create PMBusWriteVoutCommandAction.  Do not specify a volts value
        // because it will get a value of 1.3V from the
        // ActionEnvironment/Configuration.  Specify a -8 exponent.
        // Linear format volts value = (1.3 / 2^(-8)) = 332.8 = 333 = 0x014D.
        std::optional<double> volts{};
        std::unique_ptr<PMBusWriteVoutCommandAction> action =
            std::make_unique<PMBusWriteVoutCommandAction>(
                volts, pmbus_utils::VoutDataFormat::linear, -8, false);

        // Create mock I2CInterface.  Expect action to write 0x014D to
        // VOUT_COMMAND (command/register 0x21).
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x21), TypedEq<uint16_t>(0x014D)))
            .Times(1);

        // Create Configuration with volts value 1.3V
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(1.3, std::move(actions));
        Configuration* configurationPtr = configuration.get();

        // Create Device that contains Configuration
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute Configuration
        configurationPtr->execute(services, system, *chassisPtr, *devicePtr);
    }

    // Test where fails
    {
        // Create mock services.  Expect logDebug(), logError(), and
        // logI2CError() to be called.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        std::vector<std::string> expectedErrMessagesException{
            "I2CException: Failed to write byte: bus /dev/i2c-1, addr 0x70",
            "ActionError: i2c_write_byte: { register: 0x7C, value: 0xA, mask: "
            "0xFF }"};
        EXPECT_CALL(journal, logDebug("Configuring vdd_reg")).Times(1);
        EXPECT_CALL(journal, logError(expectedErrMessagesException)).Times(1);
        EXPECT_CALL(journal, logError("Unable to configure vdd_reg")).Times(1);
        EXPECT_CALL(errorLogging,
                    logI2CError(Entry::Level::Warning, Ref(journal),
                                "/dev/i2c-1", 0x70, 0))
            .Times(1);

        // Create I2CWriteByteAction with register 0x7C and value 0x0A
        std::unique_ptr<I2CWriteByteAction> action =
            std::make_unique<I2CWriteByteAction>(0x7C, 0x0A);

        // Create mock I2CInterface.  write() throws an I2CException.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x7C), TypedEq<uint8_t>(0x0A)))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to write byte", "/dev/i2c-1", 0x70}));

        // Create Configuration with no volts value specified
        std::optional<double> volts{};
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));
        Configuration* configurationPtr = configuration.get();

        // Create Device that contains Configuration
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute Configuration
        configurationPtr->execute(services, system, *chassisPtr, *devicePtr);
    }
}

// Test for execute(Services&, System&, Chassis&, Device&, Rail&)
TEST(ConfigurationTests, ExecuteForRail)
{
    // Test where works: Volts value not specified
    {
        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Configuring vio2")).Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create I2CWriteByteAction with register 0x7C and value 0x0A
        std::unique_ptr<I2CWriteByteAction> action =
            std::make_unique<I2CWriteByteAction>(0x7C, 0x0A);

        // Create mock I2CInterface.  Expect action to write 0x0A to 0x7C.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x7C), TypedEq<uint8_t>(0x0A)))
            .Times(1);

        // Create Configuration with no volts value specified
        std::optional<double> volts{};
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));
        Configuration* configurationPtr = configuration.get();

        // Create Rail that contains Configuration
        std::unique_ptr<Rail> rail =
            std::make_unique<Rail>("vio2", std::move(configuration));
        Rail* railPtr = rail.get();

        // Create Device that contains Rail
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> deviceConfiguration{};
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(std::move(rail));
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(deviceConfiguration), std::move(rails));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute Configuration
        configurationPtr->execute(services, system, *chassisPtr, *devicePtr,
                                  *railPtr);
    }

    // Test where works: Volts value specified
    {
        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Configuring vio2: volts=1.300000"))
            .Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create PMBusWriteVoutCommandAction.  Do not specify a volts value
        // because it will get a value of 1.3V from the
        // ActionEnvironment/Configuration.  Specify a -8 exponent.
        // Linear format volts value = (1.3 / 2^(-8)) = 332.8 = 333 = 0x014D.
        std::optional<double> volts{};
        std::unique_ptr<PMBusWriteVoutCommandAction> action =
            std::make_unique<PMBusWriteVoutCommandAction>(
                volts, pmbus_utils::VoutDataFormat::linear, -8, false);

        // Create mock I2CInterface.  Expect action to write 0x014D to
        // VOUT_COMMAND (command/register 0x21).
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x21), TypedEq<uint16_t>(0x014D)))
            .Times(1);

        // Create Configuration with volts value 1.3V
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(1.3, std::move(actions));
        Configuration* configurationPtr = configuration.get();

        // Create Rail that contains Configuration
        std::unique_ptr<Rail> rail =
            std::make_unique<Rail>("vio2", std::move(configuration));
        Rail* railPtr = rail.get();

        // Create Device that contains Rail
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> deviceConfiguration{};
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(std::move(rail));
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(deviceConfiguration), std::move(rails));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute Configuration
        configurationPtr->execute(services, system, *chassisPtr, *devicePtr,
                                  *railPtr);
    }

    // Test where fails
    {
        // Create mock services.  Expect logDebug(), logError(), and logI2CError
        // to be called.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        std::vector<std::string> expectedErrMessagesException{
            "I2CException: Failed to write byte: bus /dev/i2c-1, addr 0x70",
            "ActionError: i2c_write_byte: { register: 0x7C, value: 0xA, mask: "
            "0xFF }"};
        EXPECT_CALL(journal, logDebug("Configuring vio2")).Times(1);
        EXPECT_CALL(journal, logError(expectedErrMessagesException)).Times(1);
        EXPECT_CALL(journal, logError("Unable to configure vio2")).Times(1);
        EXPECT_CALL(errorLogging,
                    logI2CError(Entry::Level::Warning, Ref(journal),
                                "/dev/i2c-1", 0x70, 0))
            .Times(1);

        // Create I2CWriteByteAction with register 0x7C and value 0x0A
        std::unique_ptr<I2CWriteByteAction> action =
            std::make_unique<I2CWriteByteAction>(0x7C, 0x0A);

        // Create mock I2CInterface.  write() throws an I2CException.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x7C), TypedEq<uint8_t>(0x0A)))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to write byte", "/dev/i2c-1", 0x70}));

        // Create Configuration with no volts value specified
        std::optional<double> volts{};
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));
        Configuration* configurationPtr = configuration.get();

        // Create Rail that contains Configuration
        std::unique_ptr<Rail> rail =
            std::make_unique<Rail>("vio2", std::move(configuration));
        Rail* railPtr = rail.get();

        // Create Device that contains Rail
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> deviceConfiguration{};
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(std::move(rail));
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(deviceConfiguration), std::move(rails));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute Configuration
        configurationPtr->execute(services, system, *chassisPtr, *devicePtr,
                                  *railPtr);
    }
}

TEST(ConfigurationTests, GetActions)
{
    std::optional<double> volts{1.3};

    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action2});

    Configuration configuration(volts, std::move(actions));
    EXPECT_EQ(configuration.getActions().size(), 2);
    EXPECT_EQ(configuration.getActions()[0].get(), action1);
    EXPECT_EQ(configuration.getActions()[1].get(), action2);
}

TEST(ConfigurationTests, GetVolts)
{
    // Test where volts value specified
    {
        std::optional<double> volts{3.2};

        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        Configuration configuration(volts, std::move(actions));
        EXPECT_EQ(configuration.getVolts().has_value(), true);
        EXPECT_EQ(configuration.getVolts().value(), 3.2);
    }

    // Test where volts value not specified
    {
        std::optional<double> volts{};

        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        Configuration configuration(volts, std::move(actions));
        EXPECT_EQ(configuration.getVolts().has_value(), false);
    }
}
