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
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::test_utils;

using ::testing::A;
using ::testing::Ref;
using ::testing::Return;
using ::testing::Throw;
using ::testing::TypedEq;

static const std::string chassisInvPath{
    "/xyz/openbmc_project/inventory/system/chassis"};

TEST(DeviceTests, Constructor)
{
    // Test where only required parameters are specified
    {
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();
        Device device{
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface)};
        EXPECT_EQ(device.getID(), "vdd_reg");
        EXPECT_EQ(device.isRegulator(), true);
        EXPECT_EQ(
            device.getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
        EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
        EXPECT_EQ(device.getPresenceDetection(), nullptr);
        EXPECT_EQ(device.getConfiguration(), nullptr);
        EXPECT_EQ(device.getRails().size(), 0);
    }

    // Test where all parameters are specified
    {
        // Create I2CInterface
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();

        // Create PresenceDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));

        // Create Configuration
        std::optional<double> volts{};
        actions.clear();
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create vector of Rail objects
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.push_back(std::make_unique<Rail>("vdd0"));
        rails.push_back(std::make_unique<Rail>("vdd1"));

        // Create Device
        Device device{
            "vdd_reg",
            false,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface),
            std::move(presenceDetection),
            std::move(configuration),
            std::move(rails)};
        EXPECT_EQ(device.getID(), "vdd_reg");
        EXPECT_EQ(device.isRegulator(), false);
        EXPECT_EQ(
            device.getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1");
        EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
        EXPECT_NE(device.getPresenceDetection(), nullptr);
        EXPECT_EQ(device.getPresenceDetection()->getActions().size(), 1);
        EXPECT_NE(device.getConfiguration(), nullptr);
        EXPECT_EQ(device.getConfiguration()->getVolts().has_value(), false);
        EXPECT_EQ(device.getConfiguration()->getActions().size(), 2);
        EXPECT_EQ(device.getRails().size(), 2);
    }
}

TEST(DeviceTests, AddToIDMap)
{
    std::unique_ptr<PresenceDetection> presenceDetection{};
    std::unique_ptr<Configuration> configuration{};

    // Create vector of Rail objects
    std::vector<std::unique_ptr<Rail>> rails{};
    rails.push_back(std::make_unique<Rail>("vdd0"));
    rails.push_back(std::make_unique<Rail>("vdd1"));

    // Create Device
    Device device{
        "vdd_reg",
        false,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
        std::move(createI2CInterface()),
        std::move(presenceDetection),
        std::move(configuration),
        std::move(rails)};

    // Add Device and Rail objects to an IDMap
    IDMap idMap{};
    device.addToIDMap(idMap);

    // Verify Device is in the IDMap
    EXPECT_NO_THROW(idMap.getDevice("vdd_reg"));
    EXPECT_THROW(idMap.getDevice("vio_reg"), std::invalid_argument);

    // Verify all Rails are in the IDMap
    EXPECT_NO_THROW(idMap.getRail("vdd0"));
    EXPECT_NO_THROW(idMap.getRail("vdd1"));
    EXPECT_THROW(idMap.getRail("vdd2"), std::invalid_argument);
}

TEST(DeviceTests, ClearCache)
{
    // Test where Device does not contain a PresenceDetection object
    try
    {
        Device device{
            "vdd_reg", false,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(createI2CInterface())};
        device.clearCache();
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where Device contains a PresenceDetection object
    {
        // Create PresenceDetection
        std::vector<std::unique_ptr<Action>> actions{};
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));
        PresenceDetection* presenceDetectionPtr = presenceDetection.get();

        // Create Device
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
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Cache presence value in PresenceDetection
        MockServices services{};
        presenceDetectionPtr->execute(services, system, *chassisPtr,
                                      *devicePtr);
        EXPECT_TRUE(presenceDetectionPtr->getCachedPresence().has_value());

        // Clear cached data in Device
        devicePtr->clearCache();

        // Verify presence value no longer cached in PresenceDetection
        EXPECT_FALSE(presenceDetectionPtr->getCachedPresence().has_value());
    }
}

TEST(DeviceTests, ClearErrorHistory)
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
        devicePtr->monitorSensors(services, system, *chassisPtr);
    }

    // Clear error history
    devicePtr->clearErrorHistory();

    // Monitor sensors 5 times again.  Should fail every time, write to journal
    // 3 times, and log one error.
    for (int i = 1; i <= 5; ++i)
    {
        devicePtr->monitorSensors(services, system, *chassisPtr);
    }
}

TEST(DeviceTests, Close)
{
    // Test where works: I2C interface is not open
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*i2cInterface, close).Times(0);

        // Create mock services.  No logError should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::vector<std::string>&>()))
            .Times(0);

        // Create Device
        Device device{
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface)};

        // Close Device
        device.close(services);
    }

    // Test where works: I2C interface is open
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, close).Times(1);

        // Create mock services.  No logError should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::vector<std::string>&>()))
            .Times(0);

        // Create Device
        Device device{
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface)};

        // Close Device
        device.close(services);
    }

    // Test where fails: closing I2C interface fails
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, close)
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to close", "/dev/i2c-1", 0x70}));

        // Create mock services.  Expect logError() and logI2CError() to be
        // called.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        std::vector<std::string> expectedErrMessagesException{
            "I2CException: Failed to close: bus /dev/i2c-1, addr 0x70"};
        EXPECT_CALL(journal, logError("Unable to close device vdd_reg"))
            .Times(1);
        EXPECT_CALL(journal, logError(expectedErrMessagesException)).Times(1);
        EXPECT_CALL(errorLogging,
                    logI2CError(Entry::Level::Notice, Ref(journal),
                                "/dev/i2c-1", 0x70, 0))
            .Times(1);

        // Create Device
        Device device{
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface)};

        // Close Device
        device.close(services);
    }
}

TEST(DeviceTests, Configure)
{
    // Test where device is not present
    {
        // Create mock services.  No logging should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create PresenceDetection.  Indicates device is not present.
        std::unique_ptr<MockAction> presAction = std::make_unique<MockAction>();
        EXPECT_CALL(*presAction, execute).Times(1).WillOnce(Return(false));
        std::vector<std::unique_ptr<Action>> presActions{};
        presActions.emplace_back(std::move(presAction));
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(presActions));

        // Create Configuration.  Action inside it should not be executed.
        std::optional<double> volts{};
        std::unique_ptr<MockAction> confAction = std::make_unique<MockAction>();
        EXPECT_CALL(*confAction, execute).Times(0);
        std::vector<std::unique_ptr<Action>> confActions{};
        confActions.emplace_back(std::move(confAction));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(confActions));

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
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

        // Call configure().  Should do nothing.
        devicePtr->configure(services, system, *chassisPtr);
    }

    // Test where Configuration and Rails were not specified in constructor
    {
        // Create mock services.  No logging should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface));
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

        // Call configure().
        devicePtr->configure(services, system, *chassisPtr);
    }

    // Test where Configuration and Rails were specified in constructor
    {
        std::vector<std::unique_ptr<Rail>> rails{};

        // Create mock services.  Expect logDebug() to be called.
        // For the Device and both Rails, should execute the Configuration
        // and log a debug message.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Configuring reg1")).Times(1);
        EXPECT_CALL(journal, logDebug("Configuring vdd0: volts=1.300000"))
            .Times(1);
        EXPECT_CALL(journal, logDebug("Configuring vio0: volts=3.200000"))
            .Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Rail vdd0
        {
            // Create Configuration for Rail
            std::optional<double> volts{1.3};
            std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            std::unique_ptr<Configuration> configuration =
                std::make_unique<Configuration>(volts, std::move(actions));

            // Create Rail
            std::unique_ptr<Rail> rail =
                std::make_unique<Rail>("vdd0", std::move(configuration));
            rails.emplace_back(std::move(rail));
        }

        // Create Rail vio0
        {
            // Create Configuration for Rail
            std::optional<double> volts{3.2};
            std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            std::unique_ptr<Configuration> configuration =
                std::make_unique<Configuration>(volts, std::move(actions));

            // Create Rail
            std::unique_ptr<Rail> rail =
                std::make_unique<Rail>("vio0", std::move(configuration));
            rails.emplace_back(std::move(rail));
        }

        // Create Configuration for Device
        std::optional<double> volts{};
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration), std::move(rails));
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

        // Call configure().
        devicePtr->configure(services, system, *chassisPtr);
    }
}

TEST(DeviceTests, GetConfiguration)
{
    // Test where Configuration was not specified in constructor
    {
        Device device{
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(createI2CInterface())};
        EXPECT_EQ(device.getConfiguration(), nullptr);
    }

    // Test where Configuration was specified in constructor
    {
        std::unique_ptr<PresenceDetection> presenceDetection{};

        // Create Configuration
        std::optional<double> volts{3.2};
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());
        std::unique_ptr<Configuration> configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create Device
        Device device{
            "vdd_reg",
            true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(createI2CInterface()),
            std::move(presenceDetection),
            std::move(configuration)};
        EXPECT_NE(device.getConfiguration(), nullptr);
        EXPECT_EQ(device.getConfiguration()->getVolts().has_value(), true);
        EXPECT_EQ(device.getConfiguration()->getVolts().value(), 3.2);
        EXPECT_EQ(device.getConfiguration()->getActions().size(), 2);
    }
}

TEST(DeviceTests, GetFRU)
{
    Device device{
        "vdd_reg", true,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
        std::move(createI2CInterface())};
    EXPECT_EQ(device.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
}

TEST(DeviceTests, GetI2CInterface)
{
    std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
    i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();
    Device device{
        "vdd_reg", true,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
        std::move(i2cInterface)};
    EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
}

TEST(DeviceTests, GetID)
{
    Device device{
        "vdd_reg", false,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
        std::move(createI2CInterface())};
    EXPECT_EQ(device.getID(), "vdd_reg");
}

TEST(DeviceTests, GetPresenceDetection)
{
    // Test where PresenceDetection was not specified in constructor
    {
        Device device{
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(createI2CInterface())};
        EXPECT_EQ(device.getPresenceDetection(), nullptr);
    }

    // Test where PresenceDetection was specified in constructor
    {
        // Create PresenceDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));

        // Create Device
        Device device{
            "vdd_reg", false,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(createI2CInterface()), std::move(presenceDetection)};
        EXPECT_NE(device.getPresenceDetection(), nullptr);
        EXPECT_EQ(device.getPresenceDetection()->getActions().size(), 1);
    }
}

TEST(DeviceTests, GetRails)
{
    // Test where no rails were specified in constructor
    {
        Device device{
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(createI2CInterface())};
        EXPECT_EQ(device.getRails().size(), 0);
    }

    // Test where rails were specified in constructor
    {
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> configuration{};

        // Create vector of Rail objects
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.push_back(std::make_unique<Rail>("vdd0"));
        rails.push_back(std::make_unique<Rail>("vdd1"));

        // Create Device
        Device device{
            "vdd_reg",
            false,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(createI2CInterface()),
            std::move(presenceDetection),
            std::move(configuration),
            std::move(rails)};
        EXPECT_EQ(device.getRails().size(), 2);
        EXPECT_EQ(device.getRails()[0]->getID(), "vdd0");
        EXPECT_EQ(device.getRails()[1]->getID(), "vdd1");
    }
}

TEST(DeviceTests, IsPresent)
{
    // Test where PresenceDetection not specified in constructor
    {
        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface));
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

        // Create MockServices
        MockServices services{};

        // Since no PresenceDetection defined, isPresent() should return true
        EXPECT_TRUE(devicePtr->isPresent(services, system, *chassisPtr));
    }

    // Test where PresenceDetection was specified in constructor: Is present
    {
        // Create PresenceDetection.  Indicates device is present.
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));

        // Create Device
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
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Create MockServices
        MockServices services{};

        // PresenceDetection::execute() and isPresent() should return true
        EXPECT_TRUE(devicePtr->isPresent(services, system, *chassisPtr));
    }

    // Test where PresenceDetection was specified in constructor: Is not present
    {
        // Create PresenceDetection.  Indicates device is not present.
        std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));

        // Create Device
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
            std::make_unique<Chassis>(1, chassisInvPath, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Create MockServices
        MockServices services{};

        // PresenceDetection::execute() and isPresent() should return false
        EXPECT_FALSE(devicePtr->isPresent(services, system, *chassisPtr));
    }
}

TEST(DeviceTests, IsRegulator)
{
    Device device{
        "vdd_reg", false,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
        std::move(createI2CInterface())};
    EXPECT_EQ(device.isRegulator(), false);
}

TEST(DeviceTests, MonitorSensors)
{
    // Test where device is not present
    {
        // Create mock services.  No Sensors methods should be called.
        MockServices services{};
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail).Times(0);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail).Times(0);

        // Create SensorMonitoring.  Action inside it should not be executed.
        std::unique_ptr<MockAction> sensAction = std::make_unique<MockAction>();
        EXPECT_CALL(*sensAction, execute).Times(0);
        std::vector<std::unique_ptr<Action>> sensActions{};
        sensActions.emplace_back(std::move(sensAction));
        std::unique_ptr<SensorMonitoring> sensorMonitoring =
            std::make_unique<SensorMonitoring>(std::move(sensActions));

        // Create Rail
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<Rail> rail = std::make_unique<Rail>(
            "vddr1", std::move(configuration), std::move(sensorMonitoring));

        // Create PresenceDetection.  Indicates device is not present.
        std::unique_ptr<MockAction> presAction = std::make_unique<MockAction>();
        EXPECT_CALL(*presAction, execute).Times(1).WillOnce(Return(false));
        std::vector<std::unique_ptr<Action>> presActions{};
        presActions.emplace_back(std::move(presAction));
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(presActions));

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
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

        // Call monitorSensors().  Should do nothing.
        devicePtr->monitorSensors(services, system, *chassisPtr);
    }

    // Test where Rails were not specified in constructor
    {
        // Create mock services.  No Sensors methods should be called.
        MockServices services{};
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail).Times(0);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail).Times(0);

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface));
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

        // Call monitorSensors().  Should do nothing.
        devicePtr->monitorSensors(services, system, *chassisPtr);
    }

    // Test where Rails were specified in constructor
    {
        // Create mock services.  Set Sensors service expectations.
        MockServices services{};
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail("vdd0",
                                       "/xyz/openbmc_project/inventory/system/"
                                       "chassis/motherboard/reg1",
                                       chassisInvPath))
            .Times(1);
        EXPECT_CALL(sensors, startRail("vio0",
                                       "/xyz/openbmc_project/inventory/system/"
                                       "chassis/motherboard/reg1",
                                       chassisInvPath))
            .Times(1);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail(false)).Times(2);

        std::vector<std::unique_ptr<Rail>> rails{};

        // Create Rail vdd0
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
            rails.emplace_back(std::move(rail));
        }

        // Create Rail vio0
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
                "vio0", std::move(configuration), std::move(sensorMonitoring));
            rails.emplace_back(std::move(rail));
        }

        // Create Device that contains Rails
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration), std::move(rails));
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

        // Call monitorSensors().  Should monitor sensors in both rails.
        devicePtr->monitorSensors(services, system, *chassisPtr);
    }
}
