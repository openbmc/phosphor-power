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
#include "log_phase_fault_action.hpp"
#include "mock_action.hpp"
#include "mock_error_logging.hpp"
#include "mock_journal.hpp"
#include "mock_sensors.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "phase_fault.hpp"
#include "phase_fault_detection.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "sensor_monitoring.hpp"
#include "sensors.hpp"
#include "services.hpp"
#include "system.hpp"
#include "test_sdbus_error.hpp"
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
using ::testing::Throw;
using ::testing::TypedEq;

static const std::string chassisInvPath{
    "/xyz/openbmc_project/inventory/system/chassis"};

TEST(SystemTests, Constructor)
{
    // Create Rules
    std::vector<std::unique_ptr<Rule>> rules{};
    rules.emplace_back(createRule("set_voltage_rule"));

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(createDevice("reg1", {"rail1"}));
    chassis.emplace_back(
        std::make_unique<Chassis>(1, chassisInvPath, std::move(devices)));

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

TEST(SystemTests, ClearCache)
{
    // Create PresenceDetection
    std::vector<std::unique_ptr<Action>> actions{};
    auto presenceDetection =
        std::make_unique<PresenceDetection>(std::move(actions));
    PresenceDetection* presenceDetectionPtr = presenceDetection.get();

    // Create Device that contains PresenceDetection
    auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
    auto device = std::make_unique<Device>(
        "reg1", true,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
        std::move(i2cInterface), std::move(presenceDetection));
    Device* devicePtr = device.get();

    // Create Chassis that contains Device
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(std::move(device));
    auto chassis =
        std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
    Chassis* chassisPtr = chassis.get();

    // Create System that contains Chassis
    std::vector<std::unique_ptr<Rule>> rules{};
    std::vector<std::unique_ptr<Chassis>> chassisVec{};
    chassisVec.emplace_back(std::move(chassis));
    System system{std::move(rules), std::move(chassisVec)};

    // Cache presence value in PresenceDetection
    MockServices services{};
    presenceDetectionPtr->execute(services, system, *chassisPtr, *devicePtr);
    EXPECT_TRUE(presenceDetectionPtr->getCachedPresence().has_value());

    // Clear cached data in System
    system.clearCache();

    // Verify presence value no longer cached in PresenceDetection
    EXPECT_FALSE(presenceDetectionPtr->getCachedPresence().has_value());
}

TEST(SystemTests, ClearErrorHistory)
{
    // Create SensorMonitoring.  Will fail with a DBus exception.
    auto action = std::make_unique<MockAction>();
    EXPECT_CALL(*action, execute)
        .WillRepeatedly(Throw(TestSDBusError{"Unable to set sensor value"}));
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::move(action));
    auto sensorMonitoring =
        std::make_unique<SensorMonitoring>(std::move(actions));

    // Create Rail
    std::unique_ptr<Configuration> configuration{};
    auto rail = std::make_unique<Rail>("vddr1", std::move(configuration),
                                       std::move(sensorMonitoring));

    // Create Device that contains Rail
    auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
    std::unique_ptr<PresenceDetection> presenceDetection{};
    std::unique_ptr<Configuration> deviceConfiguration{};
    std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
    std::vector<std::unique_ptr<Rail>> rails{};
    rails.emplace_back(std::move(rail));
    auto device = std::make_unique<Device>(
        "reg1", true,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
        std::move(i2cInterface), std::move(presenceDetection),
        std::move(deviceConfiguration), std::move(phaseFaultDetection),
        std::move(rails));

    // Create Chassis that contains Device
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(std::move(device));
    auto chassis =
        std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));

    // Create System that contains Chassis
    std::vector<std::unique_ptr<Rule>> rules{};
    std::vector<std::unique_ptr<Chassis>> chassisVec{};
    chassisVec.emplace_back(std::move(chassis));
    System system{std::move(rules), std::move(chassisVec)};

    // Create lambda that sets MockServices expectations.  The lambda allows
    // us to set expectations multiple times without duplicate code.
    auto setExpectations = [](MockServices& services) {
        // Expect Sensors service to be called 10 times
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail).Times(10);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail).Times(10);

        // Expect Journal service to be called 6 times to log error messages
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::vector<std::string>&>()))
            .Times(6);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(6);

        // Expect ErrorLogging service to be called once to log a DBus error
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logDBusError).Times(1);
    };

    // Monitor sensors 10 times.  Verify errors logged.
    {
        // Create mock services.  Set expectations via lambda.
        MockServices services{};
        setExpectations(services);

        for (int i = 1; i <= 10; ++i)
        {
            system.monitorSensors(services);
        }
    }

    // Clear error history
    system.clearErrorHistory();

    // Monitor sensors 10 more times.  Verify errors logged again.
    {
        // Create mock services.  Set expectations via lambda.
        MockServices services{};
        setExpectations(services);

        for (int i = 1; i <= 10; ++i)
        {
            system.monitorSensors(services);
        }
    }
}

TEST(SystemTests, CloseDevices)
{
    // Specify an empty rules vector
    std::vector<std::unique_ptr<Rule>> rules{};

    // Create mock services.  Expect logDebug() to be called.
    MockServices services{};
    MockJournal& journal = services.getMockJournal();
    EXPECT_CALL(journal, logDebug("Closing devices in chassis 1")).Times(1);
    EXPECT_CALL(journal, logDebug("Closing devices in chassis 3")).Times(1);
    EXPECT_CALL(journal, logInfo(A<const std::string&>())).Times(0);
    EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1, chassisInvPath + '1'));
    chassis.emplace_back(std::make_unique<Chassis>(3, chassisInvPath + '3'));

    // Create System
    System system{std::move(rules), std::move(chassis)};

    // Call closeDevices()
    system.closeDevices(services);
}

TEST(SystemTests, Configure)
{
    // Create mock services.  Expect logInfo() to be called.
    MockServices services{};
    MockJournal& journal = services.getMockJournal();
    EXPECT_CALL(journal, logInfo("Configuring chassis 1")).Times(1);
    EXPECT_CALL(journal, logInfo("Configuring chassis 3")).Times(1);
    EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
    EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

    // Specify an empty rules vector
    std::vector<std::unique_ptr<Rule>> rules{};

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1, chassisInvPath + '1'));
    chassis.emplace_back(std::make_unique<Chassis>(3, chassisInvPath + '3'));

    // Create System
    System system{std::move(rules), std::move(chassis)};

    // Call configure()
    system.configure(services);
}

TEST(SystemTests, DetectPhaseFaults)
{
    // Create mock services with the following expectations:
    // - 2 error messages in journal for N phase fault detected in reg0
    // - 2 error messages in journal for N phase fault detected in reg1
    // - 1 N phase fault error logged for reg0
    // - 1 N phase fault error logged for reg1
    MockServices services{};
    MockJournal& journal = services.getMockJournal();
    EXPECT_CALL(journal,
                logError("n phase fault detected in regulator reg0: count=1"))
        .Times(1);
    EXPECT_CALL(journal,
                logError("n phase fault detected in regulator reg0: count=2"))
        .Times(1);
    EXPECT_CALL(journal,
                logError("n phase fault detected in regulator reg1: count=1"))
        .Times(1);
    EXPECT_CALL(journal,
                logError("n phase fault detected in regulator reg1: count=2"))
        .Times(1);
    MockErrorLogging& errorLogging = services.getMockErrorLogging();
    EXPECT_CALL(errorLogging, logPhaseFault).Times(2);

    std::vector<std::unique_ptr<Chassis>> chassisVec{};

    // Create Chassis 1 with regulator reg0
    {
        // Create PhaseFaultDetection
        auto action = std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        auto phaseFaultDetection =
            std::make_unique<PhaseFaultDetection>(std::move(actions));

        // Create Device
        auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> configuration{};
        auto device = std::make_unique<Device>(
            "reg0", true,
            "/xyz/openbmc_project/inventory/system/chassis1/motherboard/"
            "reg0",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration), std::move(phaseFaultDetection));

        // Create Chassis
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        auto chassis = std::make_unique<Chassis>(1, chassisInvPath + '1',
                                                 std::move(devices));
        chassisVec.emplace_back(std::move(chassis));
    }

    // Create Chassis 2 with regulator reg1
    {
        // Create PhaseFaultDetection
        auto action = std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        auto phaseFaultDetection =
            std::make_unique<PhaseFaultDetection>(std::move(actions));

        // Create Device
        auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> configuration{};
        auto device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis2/motherboard/"
            "reg1",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration), std::move(phaseFaultDetection));

        // Create Chassis
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        auto chassis = std::make_unique<Chassis>(2, chassisInvPath + '2',
                                                 std::move(devices));
        chassisVec.emplace_back(std::move(chassis));
    }

    // Create System that contains Chassis
    std::vector<std::unique_ptr<Rule>> rules{};
    System system{std::move(rules), std::move(chassisVec)};

    // Call detectPhaseFaults() 5 times
    for (int i = 1; i <= 5; ++i)
    {
        system.detectPhaseFaults(services);
    }
}

TEST(SystemTests, GetChassis)
{
    // Specify an empty rules vector
    std::vector<std::unique_ptr<Rule>> rules{};

    // Create Chassis
    std::vector<std::unique_ptr<Chassis>> chassis{};
    chassis.emplace_back(std::make_unique<Chassis>(1, chassisInvPath + '1'));
    chassis.emplace_back(std::make_unique<Chassis>(3, chassisInvPath + '3'));

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
        chassis.emplace_back(std::make_unique<Chassis>(1, chassisInvPath + '1',
                                                       std::move(devices)));
    }
    {
        // Chassis 2
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("reg3", {"rail3a", "rail3b"}));
        devices.emplace_back(createDevice("reg4"));
        chassis.emplace_back(std::make_unique<Chassis>(2, chassisInvPath + '2',
                                                       std::move(devices)));
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
    chassis.emplace_back(std::make_unique<Chassis>(1, chassisInvPath));

    // Create System
    System system{std::move(rules), std::move(chassis)};
    EXPECT_EQ(system.getRules().size(), 2);
    EXPECT_EQ(system.getRules()[0]->getID(), "set_voltage_rule");
    EXPECT_EQ(system.getRules()[1]->getID(), "read_sensors_rule");
}

TEST(SystemTests, MonitorSensors)
{
    // Create mock services.  Set Sensors service expectations.
    MockServices services{};
    MockSensors& sensors = services.getMockSensors();
    EXPECT_CALL(sensors, startRail("c1_vdd0",
                                   "/xyz/openbmc_project/inventory/system/"
                                   "chassis1/motherboard/vdd0_reg",
                                   chassisInvPath + '1'))
        .Times(1);
    EXPECT_CALL(sensors, startRail("c2_vdd0",
                                   "/xyz/openbmc_project/inventory/system/"
                                   "chassis2/motherboard/vdd0_reg",
                                   chassisInvPath + '2'))
        .Times(1);
    EXPECT_CALL(sensors, setValue).Times(0);
    EXPECT_CALL(sensors, endRail(false)).Times(2);

    std::vector<std::unique_ptr<Chassis>> chassisVec{};

    // Create Chassis 1
    {
        // Create SensorMonitoring for Rail
        auto action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        auto sensorMonitoring =
            std::make_unique<SensorMonitoring>(std::move(actions));

        // Create Rail
        std::unique_ptr<Configuration> configuration{};
        auto rail = std::make_unique<Rail>("c1_vdd0", std::move(configuration),
                                           std::move(sensorMonitoring));

        // Create Device
        auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> deviceConfiguration{};
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(std::move(rail));
        auto device = std::make_unique<Device>(
            "c1_vdd0_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis1/motherboard/"
            "vdd0_reg",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(deviceConfiguration), std::move(phaseFaultDetection),
            std::move(rails));

        // Create Chassis
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        auto chassis = std::make_unique<Chassis>(1, chassisInvPath + '1',
                                                 std::move(devices));
        chassisVec.emplace_back(std::move(chassis));
    }

    // Create Chassis 2
    {
        // Create SensorMonitoring for Rail
        auto action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        auto sensorMonitoring =
            std::make_unique<SensorMonitoring>(std::move(actions));

        // Create Rail
        std::unique_ptr<Configuration> configuration{};
        auto rail = std::make_unique<Rail>("c2_vdd0", std::move(configuration),
                                           std::move(sensorMonitoring));

        // Create Device
        auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> deviceConfiguration{};
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(std::move(rail));
        auto device = std::make_unique<Device>(
            "c2_vdd0_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis2/motherboard/"
            "vdd0_reg",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(deviceConfiguration), std::move(phaseFaultDetection),
            std::move(rails));

        // Create Chassis
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        auto chassis = std::make_unique<Chassis>(2, chassisInvPath + '2',
                                                 std::move(devices));
        chassisVec.emplace_back(std::move(chassis));
    }

    // Create System that contains Chassis
    std::vector<std::unique_ptr<Rule>> rules{};
    System system{std::move(rules), std::move(chassisVec)};

    // Call monitorSensors()
    system.monitorSensors(services);
}
