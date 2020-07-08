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
#include "id_map.hpp"
#include "journal.hpp"
#include "mock_journal.hpp"
#include "mocked_i2c_interface.hpp"
#include "pmbus_read_sensor_action.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "system.hpp"
#include "test_utils.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::test_utils;

using ::testing::A;
using ::testing::Return;
using ::testing::TypedEq;

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
        devices.emplace_back(createDevice("vdd_reg1"));
        devices.emplace_back(createDevice("vdd_reg2"));

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

TEST(ChassisTests, AddToIDMap)
{
    // Create vector of Device objects
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(createDevice("reg1", {"rail1"}));
    devices.emplace_back(createDevice("reg2", {"rail2a", "rail2b"}));
    devices.emplace_back(createDevice("reg3"));

    // Create Chassis
    Chassis chassis{1, std::move(devices)};

    // Add Device and Rail objects within the Chassis to an IDMap
    IDMap idMap{};
    chassis.addToIDMap(idMap);

    // Verify all Devices are in the IDMap
    EXPECT_NO_THROW(idMap.getDevice("reg1"));
    EXPECT_NO_THROW(idMap.getDevice("reg2"));
    EXPECT_NO_THROW(idMap.getDevice("reg3"));
    EXPECT_THROW(idMap.getDevice("reg4"), std::invalid_argument);

    // Verify all Rails are in the IDMap
    EXPECT_NO_THROW(idMap.getRail("rail1"));
    EXPECT_NO_THROW(idMap.getRail("rail2a"));
    EXPECT_NO_THROW(idMap.getRail("rail2b"));
    EXPECT_THROW(idMap.getRail("rail3"), std::invalid_argument);
}

TEST(ChassisTests, CloseDevices)
{
    // Test where no devices were specified in constructor
    {
        // Create Chassis
        Chassis chassis{2};

        // Call closeDevices()
        journal::clear();
        chassis.closeDevices();
        EXPECT_EQ(journal::getErrMessages().size(), 0);
        EXPECT_EQ(journal::getInfoMessages().size(), 0);
        std::vector<std::string> expectedDebugMessages{
            "Closing devices in chassis 2"};
        EXPECT_EQ(journal::getDebugMessages(), expectedDebugMessages);
    }

    // Test where devices were specified in constructor
    {
        std::vector<std::unique_ptr<Device>> devices{};

        // Create Device vdd0_reg
        {
            // Create mock I2CInterface: isOpen() and close() should be called
            std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
                std::make_unique<i2c::MockedI2CInterface>();
            EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*i2cInterface, close).Times(1);

            // Create Device
            std::unique_ptr<Device> device = std::make_unique<Device>(
                "vdd0_reg", true, "/system/chassis/motherboard/vdd0_reg",
                std::move(i2cInterface));
            devices.emplace_back(std::move(device));
        }

        // Create Device vdd1_reg
        {
            // Create mock I2CInterface: isOpen() and close() should be called
            std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
                std::make_unique<i2c::MockedI2CInterface>();
            EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*i2cInterface, close).Times(1);

            // Create Device
            std::unique_ptr<Device> device = std::make_unique<Device>(
                "vdd1_reg", true, "/system/chassis/motherboard/vdd1_reg",
                std::move(i2cInterface));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{1, std::move(devices)};

        // Call closeDevices()
        journal::clear();
        chassis.closeDevices();
        EXPECT_EQ(journal::getErrMessages().size(), 0);
        EXPECT_EQ(journal::getInfoMessages().size(), 0);
        std::vector<std::string> expectedDebugMessages{
            "Closing devices in chassis 1"};
        EXPECT_EQ(journal::getDebugMessages(), expectedDebugMessages);
    }
}

TEST(ChassisTests, Configure)
{
    // Test where no devices were specified in constructor
    {
        // Create Chassis
        std::unique_ptr<Chassis> chassis = std::make_unique<Chassis>(1);
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call configure()
        journal::clear();
        chassisPtr->configure(system);
        EXPECT_EQ(journal::getDebugMessages().size(), 0);
        EXPECT_EQ(journal::getErrMessages().size(), 0);
        std::vector<std::string> expectedInfoMessages{"Configuring chassis 1"};
        EXPECT_EQ(journal::getInfoMessages(), expectedInfoMessages);
    }

    // Test where devices were specified in constructor
    {
        std::vector<std::unique_ptr<Device>> devices{};

        // Create Device vdd0_reg
        {
            // Create Configuration
            std::vector<std::unique_ptr<Action>> actions{};
            std::unique_ptr<Configuration> configuration =
                std::make_unique<Configuration>(1.3, std::move(actions));

            // Create Device
            std::unique_ptr<i2c::I2CInterface> i2cInterface =
                createI2CInterface();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            std::unique_ptr<Device> device = std::make_unique<Device>(
                "vdd0_reg", true, "/system/chassis/motherboard/vdd0_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(configuration));
            devices.emplace_back(std::move(device));
        }

        // Create Device vdd1_reg
        {
            // Create Configuration
            std::vector<std::unique_ptr<Action>> actions{};
            std::unique_ptr<Configuration> configuration =
                std::make_unique<Configuration>(1.2, std::move(actions));

            // Create Device
            std::unique_ptr<i2c::I2CInterface> i2cInterface =
                createI2CInterface();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            std::unique_ptr<Device> device = std::make_unique<Device>(
                "vdd1_reg", true, "/system/chassis/motherboard/vdd1_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(configuration));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(2, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call configure()
        journal::clear();
        chassisPtr->configure(system);
        std::vector<std::string> expectedDebugMessages{
            "Configuring vdd0_reg: volts=1.300000",
            "Configuring vdd1_reg: volts=1.200000"};
        EXPECT_EQ(journal::getDebugMessages(), expectedDebugMessages);
        EXPECT_EQ(journal::getErrMessages().size(), 0);
        std::vector<std::string> expectedInfoMessages{"Configuring chassis 2"};
        EXPECT_EQ(journal::getInfoMessages(), expectedInfoMessages);
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
        devices.emplace_back(createDevice("vdd_reg1"));
        devices.emplace_back(createDevice("vdd_reg2"));

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

TEST(ChassisTests, MonitorSensors)
{
    // Test where no devices were specified in constructor
    {
        // Create mock I2CInterface.  A two-byte read should NOT occur.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint16_t&>())).Times(0);

        // Create Device
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true, "/system/chassis/motherboard/reg1",
            std::move(i2cInterface));

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

        // Call monitorSensors()
        journal::clear();
        chassisPtr->monitorSensors(system);
        EXPECT_EQ(journal::getDebugMessages().size(), 0);
        EXPECT_EQ(journal::getErrMessages().size(), 0);
    }

    // Test where devices were specified in constructor
    {
        std::vector<std::unique_ptr<Device>> devices{};

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
        std::vector<std::unique_ptr<Rail>> rails{};
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<Rail> rail = std::make_unique<Rail>(
            "vdd0", std::move(configuration), std::move(sensorMonitoring));
        rails.emplace_back(std::move(rail));

        // Create Device
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true, "/system/chassis/motherboard/reg1",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration), std::move(rails));

        // Create Chassis
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call monitorSensors()
        journal::clear();
        chassisPtr->monitorSensors(system);
        EXPECT_EQ(journal::getDebugMessages().size(), 0);
        EXPECT_EQ(journal::getErrMessages().size(), 0);
    }
}
