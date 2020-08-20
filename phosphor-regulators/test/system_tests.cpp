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
#include "id_map.hpp"
#include "mock_journal.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "pmbus_read_sensor_action.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "services.hpp"
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

TEST(SystemTests, Constructor)
{
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    rules.emplace_back(createRule("set_voltage_rule"));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(createDevice("reg1", {"rail1"}));
    chassis.emplace_back(std::make_unique<Chassis>(1, std::move(devices)));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getChassis().size(), 1);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_NO_THROW(system.getIDMap().getRule("set_voltage_rule"));
    EXPECT_NO_THROW(system.getIDMap().getDevice("reg1"));
    EXPECT_NO_THROW(system.getIDMap().getRail("rail1"));
    EXPECT_THROW(system.getIDMap().getRail("rail2"), std::invalid_argument);
    EXPECT_EQ(system.getRules().size(), 1);
    EXPECT_EQ(system.getRules()[0]->getID(), "set_voltage_rule");
}

TEST(SystemTests, CloseDevices)
{
    // Specify an empty rules vector
    std::vector<std::unique_ptr<Rule>> rules{};

    // Create mock services.  LogDebug with expectedDebugMessages should be
    // called.
    MockServices services{};
    MockJournal& journal = services.getMockJournal();
    std::vector<std::string> expectedDebugMessages{
        "Closing devices in chassis 1", "Closing devices in chassis 3"};
    EXPECT_CALL(journal, logDebug(expectedDebugMessages[0])).Times(1);
    EXPECT_CALL(journal, logDebug(expectedDebugMessages[1])).Times(1);
    EXPECT_CALL(journal, logInfo(A<const std::string&>())).Times(0);
    EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));
    chassis.emplace_back(std::make_unique<Chassis>(3));

    // Create System
    System system{std::move(rules), std::move(chassis)};

    // Call closeDevices()
    system.closeDevices(services);
}

TEST(SystemTests, Configure)
{
    // Create mock services.  logInfo with expectedInfoMessages should be read.
    MockServices services{};
    MockJournal& journal = services.getMockJournal();
    std::vector<std::string> expectedInfoMessages{"Configuring chassis 1",
                                                  "Configuring chassis 3"};
    EXPECT_CALL(journal, logInfo(expectedInfoMessages[0])).Times(1);
    EXPECT_CALL(journal, logInfo(expectedInfoMessages[1])).Times(1);
    EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
    EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

    // Specify an empty rules vector
    std::vector<std::unique_ptr<Rule>> rules{};

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));
    chassis.emplace_back(std::make_unique<Chassis>(3));

    // Create System
    System system{std::move(rules), std::move(chassis)};

    // Call configure()
    system.configure(services);
}

TEST(SystemTests, GetChassis)
{
    // Specify an empty rules vector
    std::vector<std::unique_ptr<Rule>> rules{};

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));
    chassis.emplace_back(std::make_unique<Chassis>(3));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getChassis().size(), 2);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_EQ(system.getChassis()[1]->getNumber(), 3);
}

TEST(SystemTests, GetIDMap)
{
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    rules.emplace_back(createRule("set_voltage_rule"));
    rules.emplace_back(createRule("read_sensors_rule"));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    {
        // Chassis 1
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("reg1", {"rail1"}));
        devices.emplace_back(createDevice("reg2", {"rail2a", "rail2b"}));
        chassis.emplace_back(std::make_unique<Chassis>(1, std::move(devices)));
    }
    {
        // Chassis 2
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("reg3", {"rail3a", "rail3b"}));
        devices.emplace_back(createDevice("reg4"));
        chassis.emplace_back(std::make_unique<Chassis>(2, std::move(devices)));
    }

    // Create System
    System system{std::move(rules), std::move(chassis)};
    const IDMap& idMap = system.getIDMap();

    // Verify all Rules are in the IDMap
    EXPECT_NO_THROW(idMap.getRule("set_voltage_rule"));
    EXPECT_NO_THROW(idMap.getRule("read_sensors_rule"));
    EXPECT_THROW(idMap.getRule("set_voltage_rule2"), std::invalid_argument);

    // Verify all Devices are in the IDMap
    EXPECT_NO_THROW(idMap.getDevice("reg1"));
    EXPECT_NO_THROW(idMap.getDevice("reg2"));
    EXPECT_NO_THROW(idMap.getDevice("reg3"));
    EXPECT_NO_THROW(idMap.getDevice("reg4"));
    EXPECT_THROW(idMap.getDevice("reg5"), std::invalid_argument);

    // Verify all Rails are in the IDMap
    EXPECT_NO_THROW(idMap.getRail("rail1"));
    EXPECT_NO_THROW(idMap.getRail("rail2a"));
    EXPECT_NO_THROW(idMap.getRail("rail2b"));
    EXPECT_NO_THROW(idMap.getRail("rail3a"));
    EXPECT_NO_THROW(idMap.getRail("rail3b"));
    EXPECT_THROW(idMap.getRail("rail4"), std::invalid_argument);
}

TEST(SystemTests, GetRules)
{
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    rules.emplace_back(createRule("set_voltage_rule"));
    rules.emplace_back(createRule("read_sensors_rule"));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getRules().size(), 2);
    EXPECT_EQ(system.getRules()[0]->getID(), "set_voltage_rule");
    EXPECT_EQ(system.getRules()[1]->getID(), "read_sensors_rule");
}

TEST(SystemTests, MonitorSensors)
{
    // Create mock services.
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
    std::vector<std::unique_ptr<Rail>> rails{};
    std::unique_ptr<Configuration> configuration{};
    std::unique_ptr<Rail> rail = std::make_unique<Rail>(
        "vdd0", std::move(configuration), std::move(sensorMonitoring));
    rails.emplace_back(std::move(rail));

    // Create Device
    std::unique_ptr<PresenceDetection> presenceDetection{};
    std::unique_ptr<Configuration> deviceConfiguration{};
    std::unique_ptr<Device> device = std::make_unique<Device>(
        "reg1", true, "/system/chassis/motherboard/reg1",
        std::move(i2cInterface), std::move(presenceDetection),
        std::move(deviceConfiguration), std::move(rails));

    // Create Chassis
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(std::move(device));
    std::unique_ptr<Chassis> chassis =
        std::make_unique<Chassis>(1, std::move(devices));

    // Create System that contains Chassis
    std::vector<std::unique_ptr<Rule>> rules{};
    std::vector<std::unique_ptr<Chassis>> chassisVec{};
    chassisVec.emplace_back(std::move(chassis));
    System system{std::move(rules), std::move(chassisVec)};

    // Call monitorSensors()
    system.monitorSensors(services);
}
