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
#include "mock_action.hpp"
#include "mock_error_logging.hpp"
#include "mock_journal.hpp"
#include "mock_sensors.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "sensor_monitoring.hpp"
#include "sensors.hpp"
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

// Default chassis inventory path
static const std::string defaultInventoryPath{
    "/xyz/openbmc_project/inventory/system/chassis"};

TEST(ChassisTests, Constructor)
{
    // Test where works: Only required parameters are specified
    {
        Chassis chassis{2, defaultInventoryPath};
        EXPECT_EQ(chassis.getNumber(), 2);
        EXPECT_EQ(chassis.getInventoryPath(), defaultInventoryPath);
        EXPECT_EQ(chassis.getDevices().size(), 0);
    }

    // Test where works: All parameters are specified
    {
        // Create vector of Device objects
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("vdd_reg1"));
        devices.emplace_back(createDevice("vdd_reg2"));

        // Create Chassis
        Chassis chassis{1, defaultInventoryPath, std::move(devices)};
        EXPECT_EQ(chassis.getNumber(), 1);
        EXPECT_EQ(chassis.getInventoryPath(), defaultInventoryPath);
        EXPECT_EQ(chassis.getDevices().size(), 2);
    }

    // Test where fails: Invalid chassis number < 1
    try
    {
        Chassis chassis{0, defaultInventoryPath};
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
    Chassis chassis{1, defaultInventoryPath, std::move(devices)};

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

TEST(ChassisTests, ClearCache)
{
    // Create PresenceDetection
    std::vector<std::unique_ptr<Action>> actions{};
    std::unique_ptr<PresenceDetection> presenceDetection =
        std::make_unique<PresenceDetection>(std::move(actions));
    PresenceDetection* presenceDetectionPtr = presenceDetection.get();

    // Create Device that contains PresenceDetection
    std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
    std::unique_ptr<Device> device = std::make_unique<Device>(
        "reg1", true,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
        std::move(i2cInterface), std::move(presenceDetection));
    Device* devicePtr = device.get();

    // Create Chassis that contains Device
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(std::move(device));
    std::unique_ptr<Chassis> chassis =
        std::make_unique<Chassis>(1, defaultInventoryPath, std::move(devices));
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

    // Clear cached data in Chassis
    chassisPtr->clearCache();

    // Verify presence value no longer cached in PresenceDetection
    EXPECT_FALSE(presenceDetectionPtr->getCachedPresence().has_value());
}

TEST(ChassisTests, ClearErrorHistory)
{
    // Create SensorMonitoring.  Will fail with a DBus exception.
    std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
    EXPECT_CALL(*action, execute)
        .WillRepeatedly(Throw(TestSDBusError{"Unable to set sensor value"}));
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::move(action));
    std::unique_ptr<SensorMonitoring> sensorMonitoring =
        std::make_unique<SensorMonitoring>(std::move(actions));

    // Create Rail
    std::unique_ptr<Configuration> configuration{};
    std::unique_ptr<Rail> rail = std::make_unique<Rail>(
        "vddr1", std::move(configuration), std::move(sensorMonitoring));

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

    // Create Chassis that contains Device
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(std::move(device));
    std::unique_ptr<Chassis> chassis =
        std::make_unique<Chassis>(1, defaultInventoryPath, std::move(devices));
    Chassis* chassisPtr = chassis.get();

    // Create System that contains Chassis
    std::vector<std::unique_ptr<Rule>> rules{};
    std::vector<std::unique_ptr<Chassis>> chassisVec{};
    chassisVec.emplace_back(std::move(chassis));
    System system{std::move(rules), std::move(chassisVec)};

    // Create mock services
    MockServices services{};

    // Expect Sensors service to be called 5+5=10 times
    MockSensors& sensors = services.getMockSensors();
    EXPECT_CALL(sensors, startRail).Times(10);
    EXPECT_CALL(sensors, setValue).Times(0);
    EXPECT_CALL(sensors, endRail).Times(10);

    // Expect Journal service to be called 3+3=6 times to log error messages
    MockJournal& journal = services.getMockJournal();
    EXPECT_CALL(journal, logError(A<const std::vector<std::string>&>()))
        .Times(6);
    EXPECT_CALL(journal, logError(A<const std::string&>())).Times(6);

    // Expect ErrorLogging service to be called 1+1=2 times to log a DBus error
    MockErrorLogging& errorLogging = services.getMockErrorLogging();
    EXPECT_CALL(errorLogging, logDBusError).Times(2);

    // Monitor sensors 5 times.  Should fail every time, write to journal 3
    // times, and log one error.
    for (int i = 1; i <= 5; ++i)
    {
        chassisPtr->monitorSensors(services, system);
    }

    // Clear error history
    chassisPtr->clearErrorHistory();

    // Monitor sensors 5 times again.  Should fail every time, write to journal
    // 3 times, and log one error.
    for (int i = 1; i <= 5; ++i)
    {
        chassisPtr->monitorSensors(services, system);
    }
}

TEST(ChassisTests, CloseDevices)
{
    // Test where no devices were specified in constructor
    {
        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Closing devices in chassis 2")).Times(1);

        // Create Chassis
        Chassis chassis{2, defaultInventoryPath};

        // Call closeDevices()
        chassis.closeDevices(services);
    }

    // Test where devices were specified in constructor
    {
        std::vector<std::unique_ptr<Device>> devices{};

        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Closing devices in chassis 1")).Times(1);

        // Create Device vdd0_reg
        {
            // Create mock I2CInterface: isOpen() and close() should be called
            std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
                std::make_unique<i2c::MockedI2CInterface>();
            EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*i2cInterface, close).Times(1);

            // Create Device
            std::unique_ptr<Device> device =
                std::make_unique<Device>("vdd0_reg", true,
                                         "/xyz/openbmc_project/inventory/"
                                         "system/chassis/motherboard/vdd0_reg",
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
            std::unique_ptr<Device> device =
                std::make_unique<Device>("vdd1_reg", true,
                                         "/xyz/openbmc_project/inventory/"
                                         "system/chassis/motherboard/vdd1_reg",
                                         std::move(i2cInterface));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{1, defaultInventoryPath, std::move(devices)};

        // Call closeDevices()
        chassis.closeDevices(services);
    }
}

TEST(ChassisTests, Configure)
{
    // Test where no devices were specified in constructor
    {
        // Create mock services.  Expect logInfo() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logInfo("Configuring chassis 1")).Times(1);
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Chassis
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, defaultInventoryPath);
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call configure()
        chassisPtr->configure(services, system);
    }

    // Test where devices were specified in constructor
    {
        std::vector<std::unique_ptr<Device>> devices{};

        // Create mock services.  Expect logInfo() and logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logInfo("Configuring chassis 2")).Times(1);
        EXPECT_CALL(journal, logDebug("Configuring vdd0_reg: volts=1.300000"))
            .Times(1);
        EXPECT_CALL(journal, logDebug("Configuring vdd1_reg: volts=1.200000"))
            .Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

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
                "vdd0_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd0_reg",
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
                "vdd1_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd1_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(configuration));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        std::unique_ptr<Chassis> chassis = std::make_unique<Chassis>(
            2, defaultInventoryPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call configure()
        chassisPtr->configure(services, system);
    }
}

TEST(ChassisTests, GetDevices)
{
    // Test where no devices were specified in constructor
    {
        Chassis chassis{2, defaultInventoryPath};
        EXPECT_EQ(chassis.getDevices().size(), 0);
    }

    // Test where devices were specified in constructor
    {
        // Create vector of Device objects
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("vdd_reg1"));
        devices.emplace_back(createDevice("vdd_reg2"));

        // Create Chassis
        Chassis chassis{1, defaultInventoryPath, std::move(devices)};
        EXPECT_EQ(chassis.getDevices().size(), 2);
        EXPECT_EQ(chassis.getDevices()[0]->getID(), "vdd_reg1");
        EXPECT_EQ(chassis.getDevices()[1]->getID(), "vdd_reg2");
    }
}

TEST(ChassisTests, GetInventoryPath)
{
    Chassis chassis{3, defaultInventoryPath};
    EXPECT_EQ(chassis.getInventoryPath(), defaultInventoryPath);
}

TEST(ChassisTests, GetNumber)
{
    Chassis chassis{3, defaultInventoryPath};
    EXPECT_EQ(chassis.getNumber(), 3);
}

TEST(ChassisTests, MonitorSensors)
{
    // Test where no devices were specified in constructor
    {
        // Create mock services.  No Sensors methods should be called.
        MockServices services{};
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail).Times(0);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail).Times(0);

        // Create Chassis
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, defaultInventoryPath);
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call monitorSensors().  Should do nothing.
        chassisPtr->monitorSensors(services, system);
    }

    // Test where devices were specified in constructor
    {
        // Create mock services.  Set Sensors service expectations.
        MockServices services{};
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail("vdd0",
                                       "/xyz/openbmc_project/inventory/system/"
                                       "chassis/motherboard/vdd0_reg",
                                       defaultInventoryPath))
            .Times(1);
        EXPECT_CALL(sensors, startRail("vdd1",
                                       "/xyz/openbmc_project/inventory/system/"
                                       "chassis/motherboard/vdd1_reg",
                                       defaultInventoryPath))
            .Times(1);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail(false)).Times(2);

        std::vector<std::unique_ptr<Device>> devices{};

        // Create Device vdd0_reg
        {
            // Create SensorMonitoring for Rail
            std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            std::unique_ptr<SensorMonitoring> sensorMonitoring =
                std::make_unique<SensorMonitoring>(std::move(actions));

            // Create Rail
            std::unique_ptr<Configuration> configuration{};
            std::unique_ptr<Rail> rail = std::make_unique<Rail>(
                "vdd0", std::move(configuration), std::move(sensorMonitoring));

            // Create Device
            std::unique_ptr<i2c::I2CInterface> i2cInterface =
                createI2CInterface();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            std::unique_ptr<Configuration> deviceConfiguration{};
            std::vector<std::unique_ptr<Rail>> rails{};
            rails.emplace_back(std::move(rail));
            std::unique_ptr<Device> device = std::make_unique<Device>(
                "vdd0_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd0_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(deviceConfiguration), std::move(rails));
            devices.emplace_back(std::move(device));
        }

        // Create Device vdd1_reg
        {
            // Create SensorMonitoring for Rail
            std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            std::unique_ptr<SensorMonitoring> sensorMonitoring =
                std::make_unique<SensorMonitoring>(std::move(actions));

            // Create Rail
            std::unique_ptr<Configuration> configuration{};
            std::unique_ptr<Rail> rail = std::make_unique<Rail>(
                "vdd1", std::move(configuration), std::move(sensorMonitoring));

            // Create Device
            std::unique_ptr<i2c::I2CInterface> i2cInterface =
                createI2CInterface();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            std::unique_ptr<Configuration> deviceConfiguration{};
            std::vector<std::unique_ptr<Rail>> rails{};
            rails.emplace_back(std::move(rail));
            std::unique_ptr<Device> device = std::make_unique<Device>(
                "vdd1_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd1_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(deviceConfiguration), std::move(rails));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis that contains Devices
        std::unique_ptr<Chassis> chassis = std::make_unique<Chassis>(
            2, defaultInventoryPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call monitorSensors()
        chassisPtr->monitorSensors(services, system);
    }
}
