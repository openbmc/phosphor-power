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
#include "mock_action.hpp"
#include "mock_error_logging.hpp"
#include "mock_journal.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "pmbus_read_sensor_action.hpp"
#include "pmbus_utils.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "sensor_monitoring.hpp"
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

TEST(SensorMonitoringTests, Constructor)
{
    std::vector<std::unique_ptr<Action>> actions{};
    actions.push_back(std::make_unique<MockAction>());

    SensorMonitoring sensorMonitoring(std::move(actions));
    EXPECT_EQ(sensorMonitoring.getActions().size(), 1);
}

TEST(SensorMonitoringTests, Execute)
{
    // Test where works
    {
        // Create mock services.  No logging should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create PMBusReadSensorAction
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        uint8_t command = 0x8C;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        std::unique_ptr<PMBusReadSensorAction> action =
            std::make_unique<PMBusReadSensorAction>(type, command, format,
                                                    exponent);

        // Create mock I2CInterface.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x8C), A<uint16_t&>()))
            .Times(1);

        // Create SensorMonitoring
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<SensorMonitoring> sensorMonitoring =
            std::make_unique<SensorMonitoring>(std::move(actions));
        SensorMonitoring* sensorMonitoringPtr = sensorMonitoring.get();

        // Create Rail that contains sensorMonitoring
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<Rail> rail = std::make_unique<Rail>(
            "vio2", std::move(configuration), std::move(sensorMonitoring));
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
            std::make_unique<Chassis>(1, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute sensorMonitoring
        sensorMonitoringPtr->execute(services, system, *chassisPtr, *devicePtr,
                                     *railPtr);
    }

    // Test where fails
    {
        // Create mock services.  Expect logError() and logI2CError() to be
        // called.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        std::vector<std::string> expectedErrMessagesException{
            "I2CException: Failed to write byte: bus /dev/i2c-1, addr 0x70",
            "ActionError: pmbus_read_sensor: { type: iout, command: 0x8C, "
            "format: linear_11 }"};
        EXPECT_CALL(journal, logError(expectedErrMessagesException)).Times(1);
        EXPECT_CALL(journal,
                    logError("Unable to monitor sensors for rail vio2"))
            .Times(1);
        EXPECT_CALL(errorLogging,
                    logI2CError(Entry::Level::Warning, Ref(journal),
                                "/dev/i2c-1", 0x70, 0))
            .Times(1);

        // Create PMBusReadSensorAction
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        uint8_t command = 0x8C;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        std::unique_ptr<PMBusReadSensorAction> action =
            std::make_unique<PMBusReadSensorAction>(type, command, format,
                                                    exponent);

        // Create mock I2CInterface.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x8C), A<uint16_t&>()))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to write byte", "/dev/i2c-1", 0x70}));

        // Create SensorMonitoring
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<SensorMonitoring> sensorMonitoring =
            std::make_unique<SensorMonitoring>(std::move(actions));
        SensorMonitoring* sensorMonitoringPtr = sensorMonitoring.get();

        // Create Rail that contains sensorMonitoring
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<Rail> rail = std::make_unique<Rail>(
            "vio2", std::move(configuration), std::move(sensorMonitoring));
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
            std::make_unique<Chassis>(1, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute sensorMonitoring
        sensorMonitoringPtr->execute(services, system, *chassisPtr, *devicePtr,
                                     *railPtr);
    }
}

TEST(SensorMonitoringTests, GetActions)
{
    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action2});

    SensorMonitoring sensorMonitoring(std::move(actions));
    EXPECT_EQ(sensorMonitoring.getActions().size(), 2);
    EXPECT_EQ(sensorMonitoring.getActions()[0].get(), action1);
    EXPECT_EQ(sensorMonitoring.getActions()[1].get(), action2);
}
