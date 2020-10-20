/**
 * Copyright © 2019 IBM Corporation
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
#include "mock_journal.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "pmbus_read_sensor_action.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "sensor_monitoring.hpp"
#include "system.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::A;
using ::testing::Return;
using ::testing::TypedEq;

TEST(RailTests, Constructor)
{
    // Test where only required parameters are specified
    {
        Rail rail{"vdd0"};
        EXPECT_EQ(rail.getID(), "vdd0");
        EXPECT_EQ(rail.getConfiguration(), nullptr);
        EXPECT_EQ(rail.getSensorMonitoring(), nullptr);
    }

    // Test where all parameters are specified
    {
        // Create Configuration
        std::optional<double> volts{1.3};
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create SensorMonitoring
        actions.clear();
        actions.push_back(std::make_unique<MockAction>());
        std::unique_ptr<SensorMonitoring> sensorMonitoring =
            std::make_unique<SensorMonitoring>(std::move(actions));

        // Create Rail
        Rail rail{"vddr1", std::move(configuration),
                  std::move(sensorMonitoring)};
        EXPECT_EQ(rail.getID(), "vddr1");
        EXPECT_NE(rail.getConfiguration(), nullptr);
        EXPECT_EQ(rail.getConfiguration()->getVolts().has_value(), true);
        EXPECT_EQ(rail.getConfiguration()->getVolts().value(), 1.3);
        EXPECT_EQ(rail.getConfiguration()->getActions().size(), 2);
        EXPECT_NE(rail.getSensorMonitoring(), nullptr);
        EXPECT_EQ(rail.getSensorMonitoring()->getActions().size(), 1);
    }
}

TEST(RailTests, Configure)
{
    // Test where Configuration was not specified in constructor
    {
        // Create mock services.  No logging should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Rail
        std::unique_ptr<Rail> rail = std::make_unique<Rail>("vdd0");
        Rail* railPtr = rail.get();

        // Create Device that contains Rail
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
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

        // Call configure().
        railPtr->configure(services, system, *chassisPtr, *devicePtr);
    }

    // Test where Configuration was specified in constructor
    {
        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Configuring vddr1: volts=1.300000"))
            .Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Configuration
        std::optional<double> volts{1.3};
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create Rail
        std::unique_ptr<Rail> rail =
            std::make_unique<Rail>("vddr1", std::move(configuration));
        Rail* railPtr = rail.get();

        // Create Device that contains Rail
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
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

        // Call configure().
        railPtr->configure(services, system, *chassisPtr, *devicePtr);
    }
}

TEST(RailTests, GetConfiguration)
{
    // Test where Configuration was not specified in constructor
    {
        Rail rail{"vdd0"};
        EXPECT_EQ(rail.getConfiguration(), nullptr);
    }

    // Test where Configuration was specified in constructor
    {
        // Create Configuration
        std::optional<double> volts{3.2};
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create Rail
        Rail rail{"vddr1", std::move(configuration)};
        EXPECT_NE(rail.getConfiguration(), nullptr);
        EXPECT_EQ(rail.getConfiguration()->getVolts().has_value(), true);
        EXPECT_EQ(rail.getConfiguration()->getVolts().value(), 3.2);
        EXPECT_EQ(rail.getConfiguration()->getActions().size(), 1);
    }
}

TEST(RailTests, GetID)
{
    Rail rail{"vio2"};
    EXPECT_EQ(rail.getID(), "vio2");
}

TEST(RailTests, MonitorSensors)
{
    // Test where SensorMonitoring was not specified in constructor
    {
        // Create mock services.  No logging should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create mock I2CInterface.  A two-byte read should NOT occur.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint16_t&>())).Times(0);

        // Create Rail
        std::unique_ptr<Rail> rail = std::make_unique<Rail>("vdd0");
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

        // Call monitorSensors().
        railPtr->monitorSensors(services, system, *chassisPtr, *devicePtr);
    }

    // Test where SensorMonitoring was specified in constructor
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

        // Create mock I2CInterface.  A two-byte read should occur.
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

        // Create Rail
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<Rail> rail = std::make_unique<Rail>(
            "vddr1", std::move(configuration), std::move(sensorMonitoring));
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

        // Call monitorSensors().
        railPtr->monitorSensors(services, system, *chassisPtr, *devicePtr);
    }
}

TEST(RailTests, GetSensorMonitoring)
{
    // Test where SensorMonitoring was not specified in constructor
    {
        Rail rail{"vdd0", nullptr, nullptr};
        EXPECT_EQ(rail.getSensorMonitoring(), nullptr);
    }

    // Test where SensorMonitoring was specified in constructor
    {
        std::unique_ptr<Configuration> configuration{};

        // Create SensorMonitoring
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());
        std::unique_ptr<SensorMonitoring> sensorMonitoring =
            std::make_unique<SensorMonitoring>(std::move(actions));

        // Create Rail
        Rail rail{"vddr1", std::move(configuration),
                  std::move(sensorMonitoring)};
        EXPECT_NE(rail.getSensorMonitoring(), nullptr);
        EXPECT_EQ(rail.getSensorMonitoring()->getActions().size(), 2);
    }
}
