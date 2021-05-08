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
#include "mock_sensors.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "pmbus_read_sensor_action.hpp"
#include "pmbus_utils.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "sensor_monitoring.hpp"
#include "sensors.hpp"
#include "system.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::pmbus_utils;

using ::testing::A;
using ::testing::Ref;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::Throw;
using ::testing::TypedEq;

/**
 * Creates the parent objects that normally contain a SensorMonitoring object.
 *
 * A SensorMonitoring object is normally contained within a hierarchy of System,
 * Chassis, Device, and Rail objects.  These objects are required in order to
 * call the execute() method.
 *
 * Creates the System, Chassis, Device, and Rail objects.  The SensorMonitoring
 * object is moved into the Rail object.
 *
 * @param monitoring SensorMonitoring object to move into object hierarchy
 * @return Tuple containing pointers the parent objects and the
 *         MockedI2CInterface object.  They are all contained within the System
 *         object and will be automatically destructed.
 */
std::tuple<std::unique_ptr<System>, Chassis*, Device*, i2c::MockedI2CInterface*,
           Rail*>
    createParentObjects(std::unique_ptr<SensorMonitoring> monitoring)
{
    // Create Rail that contains SensorMonitoring
    std::unique_ptr<Configuration> configuration{};
    std::unique_ptr<Rail> rail = std::make_unique<Rail>(
        "vdd", std::move(configuration), std::move(monitoring));
    Rail* railPtr = rail.get();

    // Create mock I2CInterface
    std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
        std::make_unique<i2c::MockedI2CInterface>();
    i2c::MockedI2CInterface* i2cInterfacePtr = i2cInterface.get();

    // Create Device that contains Rail
    std::unique_ptr<PresenceDetection> presenceDetection{};
    std::unique_ptr<Configuration> deviceConfiguration{};
    std::vector<std::unique_ptr<Rail>> rails{};
    rails.emplace_back(std::move(rail));
    std::unique_ptr<Device> device = std::make_unique<Device>(
        "vdd_reg", true,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
        std::move(i2cInterface), std::move(presenceDetection),
        std::move(deviceConfiguration), std::move(rails));
    Device* devicePtr = device.get();

    // Create Chassis that contains Device
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(std::move(device));
    std::unique_ptr<Chassis> chassis = std::make_unique<Chassis>(
        1, "/xyz/openbmc_project/inventory/system/chassis", std::move(devices));
    Chassis* chassisPtr = chassis.get();

    // Create System that contains Chassis
    std::vector<std::unique_ptr<Rule>> rules{};
    std::vector<std::unique_ptr<Chassis>> chassisVec{};
    chassisVec.emplace_back(std::move(chassis));
    std::unique_ptr<System> system =
        std::make_unique<System>(std::move(rules), std::move(chassisVec));

    return std::make_tuple(std::move(system), chassisPtr, devicePtr,
                           i2cInterfacePtr, railPtr);
}

TEST(SensorMonitoringTests, Constructor)
{
    std::vector<std::unique_ptr<Action>> actions{};
    actions.push_back(std::make_unique<MockAction>());

    SensorMonitoring sensorMonitoring(std::move(actions));
    EXPECT_EQ(sensorMonitoring.getActions().size(), 1);
}

TEST(SensorMonitoringTests, ClearErrorHistory)
{
    // Create PMBusReadSensorAction
    SensorType type{SensorType::iout};
    uint8_t command{0x8C};
    SensorDataFormat format{SensorDataFormat::linear_11};
    std::optional<int8_t> exponent{};
    std::unique_ptr<PMBusReadSensorAction> action =
        std::make_unique<PMBusReadSensorAction>(type, command, format,
                                                exponent);

    // Create SensorMonitoring
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::move(action));
    SensorMonitoring* monitoring = new SensorMonitoring(std::move(actions));

    // Create parent objects that contain SensorMonitoring
    auto [system, chassis, device, i2cInterface, rail] =
        createParentObjects(std::unique_ptr<SensorMonitoring>{monitoring});

    // Set I2CInterface expectations
    EXPECT_CALL(*i2cInterface, isOpen).WillRepeatedly(Return(true));
    EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x8C), A<uint16_t&>()))
        .WillRepeatedly(Throw(
            i2c::I2CException{"Failed to read word data", "/dev/i2c-1", 0x70}));

    // Perform sensor monitoring 10 times to set error history data members
    {
        // Create mock services
        MockServices services{};

        // Set Sensors service expectations.  SensorMonitoring will be executed
        // 10 times.
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail).Times(10);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail(true)).Times(10);

        // Set Journal service expectations.  SensorMonitoring should log error
        // messages 3 times and then stop.
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::vector<std::string>&>()))
            .Times(3);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(3);

        // Set ErrorLogging service expectations.  SensorMonitoring should log
        // an error only once.
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logI2CError).Times(1);

        // Execute SensorMonitoring 10 times
        for (int i = 1; i <= 10; ++i)
        {
            monitoring->execute(services, *system, *chassis, *device, *rail);
        }
    }

    // Clear error history
    monitoring->clearErrorHistory();

    // Perform sensor monitoring one more time.  Should log errors again.
    {
        // Create mock services
        MockServices services{};

        // Set Sensors service expectations.  SensorMonitoring will be executed
        // 1 time.
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail).Times(1);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail(true)).Times(1);

        // Set Journal service expectations.  SensorMonitoring should log error
        // messages 1 time.
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::vector<std::string>&>()))
            .Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(1);

        // Set ErrorLogging server expectations.  SensorMonitoring should log an
        // error.
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logI2CError).Times(1);

        // Execute SensorMonitoring
        monitoring->execute(services, *system, *chassis, *device, *rail);
    }
}

TEST(SensorMonitoringTests, Execute)
{
    // Test where works
    {
        // Create PMBusReadSensorAction
        SensorType type{SensorType::iout};
        uint8_t command{0x8C};
        SensorDataFormat format{SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        std::unique_ptr<PMBusReadSensorAction> action =
            std::make_unique<PMBusReadSensorAction>(type, command, format,
                                                    exponent);

        // Create SensorMonitoring
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        SensorMonitoring* monitoring = new SensorMonitoring(std::move(actions));

        // Create parent objects that contain SensorMonitoring
        auto [system, chassis, device, i2cInterface, rail] =
            createParentObjects(std::unique_ptr<SensorMonitoring>{monitoring});

        // Set I2CInterface expectations
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x8C), A<uint16_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0xD2E0));

        // Create mock services.  Set Sensors service expectations.
        MockServices services{};
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors,
                    startRail("vdd",
                              "/xyz/openbmc_project/inventory/system/chassis/"
                              "motherboard/reg2",
                              "/xyz/openbmc_project/inventory/system/chassis"))
            .Times(1);
        EXPECT_CALL(sensors, setValue(SensorType::iout, 11.5)).Times(1);
        EXPECT_CALL(sensors, endRail(false)).Times(1);

        // Execute SensorMonitoring
        monitoring->execute(services, *system, *chassis, *device, *rail);
    }

    // Test where fails
    {
        // Create PMBusReadSensorAction
        SensorType type{SensorType::iout};
        uint8_t command{0x8C};
        SensorDataFormat format{SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        std::unique_ptr<PMBusReadSensorAction> action =
            std::make_unique<PMBusReadSensorAction>(type, command, format,
                                                    exponent);

        // Create SensorMonitoring
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        SensorMonitoring* monitoring = new SensorMonitoring(std::move(actions));

        // Create parent objects that contain SensorMonitoring
        auto [system, chassis, device, i2cInterface, rail] =
            createParentObjects(std::unique_ptr<SensorMonitoring>{monitoring});

        // Set I2CInterface expectations.  Should read register 0x8C 4 times.
        EXPECT_CALL(*i2cInterface, isOpen)
            .Times(4)
            .WillRepeatedly(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x8C), A<uint16_t&>()))
            .Times(4)
            .WillRepeatedly(Throw(i2c::I2CException{"Failed to read word data",
                                                    "/dev/i2c-1", 0x70}));

        // Create mock services
        MockServices services{};

        // Set Sensors service expectations.  SensorMonitoring will be executed
        // 4 times, and all should fail.
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors,
                    startRail("vdd",
                              "/xyz/openbmc_project/inventory/system/chassis/"
                              "motherboard/reg2",
                              "/xyz/openbmc_project/inventory/system/chassis"))
            .Times(4);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail(true)).Times(4);

        // Set Journal service expectations.  SensorMonitoring should log error
        // messages 3 times and then stop.
        MockJournal& journal = services.getMockJournal();
        std::vector<std::string> expectedErrMessagesException{
            "I2CException: Failed to read word data: bus /dev/i2c-1, addr 0x70",
            "ActionError: pmbus_read_sensor: { type: iout, command: 0x8C, "
            "format: linear_11 }"};
        EXPECT_CALL(journal, logError(expectedErrMessagesException)).Times(3);
        EXPECT_CALL(journal, logError("Unable to monitor sensors for rail vdd"))
            .Times(3);

        // Set ErrorLogging service expectations.  SensorMonitoring should log
        // an error only once.
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging,
                    logI2CError(Entry::Level::Warning, Ref(journal),
                                "/dev/i2c-1", 0x70, 0))
            .Times(1);

        // Execute SensorMonitoring 4 times
        for (int i = 1; i <= 4; ++i)
        {
            monitoring->execute(services, *system, *chassis, *device, *rail);
        }
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
