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

class DeviceTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the Chassis and System objects needed for calling some Device
     * methods.
     */
    DeviceTests() : ::testing::Test{}
    {
        // Create Chassis
        auto chassis = std::make_unique<Chassis>(1, chassisInvPath);
        this->chassis = chassis.get();

        // Create System
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        this->system =
            std::make_unique<System>(std::move(rules), std::move(chassisVec));
    }

  protected:
    const std::string deviceInvPath{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2"};
    const std::string chassisInvPath{
        "/xyz/openbmc_project/inventory/system/chassis"};

    // Note: This pointer does NOT need to be explicitly deleted.  The Chassis
    // object is owned by the System object and will be automatically deleted.
    Chassis* chassis{nullptr};

    std::unique_ptr<System> system{};
};

TEST_F(DeviceTests, Constructor)
{
    // Test where only required parameters are specified
    {
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();
        Device device{"vdd_reg", true, deviceInvPath, std::move(i2cInterface)};
        EXPECT_EQ(device.getID(), "vdd_reg");
        EXPECT_EQ(device.isRegulator(), true);
        EXPECT_EQ(device.getFRU(), deviceInvPath);
        EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
        EXPECT_EQ(device.getPresenceDetection(), nullptr);
        EXPECT_EQ(device.getConfiguration(), nullptr);
        EXPECT_EQ(device.getPhaseFaultDetection(), nullptr);
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
        auto presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));

        // Create Configuration
        std::optional<double> volts{};
        actions.clear();
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());
        auto configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create PhaseFaultDetection
        actions.clear();
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());
        auto phaseFaultDetection =
            std::make_unique<PhaseFaultDetection>(std::move(actions));

        // Create vector of Rail objects
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.push_back(std::make_unique<Rail>("vdd0"));
        rails.push_back(std::make_unique<Rail>("vdd1"));

        // Create Device
        Device device{"vdd_reg",
                      false,
                      deviceInvPath,
                      std::move(i2cInterface),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(phaseFaultDetection),
                      std::move(rails)};
        EXPECT_EQ(device.getID(), "vdd_reg");
        EXPECT_EQ(device.isRegulator(), false);
        EXPECT_EQ(device.getFRU(), deviceInvPath);
        EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
        EXPECT_NE(device.getPresenceDetection(), nullptr);
        EXPECT_EQ(device.getPresenceDetection()->getActions().size(), 1);
        EXPECT_NE(device.getConfiguration(), nullptr);
        EXPECT_EQ(device.getConfiguration()->getVolts().has_value(), false);
        EXPECT_EQ(device.getConfiguration()->getActions().size(), 2);
        EXPECT_NE(device.getPhaseFaultDetection(), nullptr);
        EXPECT_EQ(device.getPhaseFaultDetection()->getActions().size(), 3);
        EXPECT_EQ(device.getRails().size(), 2);
    }
}

TEST_F(DeviceTests, AddToIDMap)
{
    std::unique_ptr<PresenceDetection> presenceDetection{};
    std::unique_ptr<Configuration> configuration{};
    std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};

    // Create vector of Rail objects
    std::vector<std::unique_ptr<Rail>> rails{};
    rails.push_back(std::make_unique<Rail>("vdd0"));
    rails.push_back(std::make_unique<Rail>("vdd1"));

    // Create Device
    Device device{"vdd_reg",
                  false,
                  deviceInvPath,
                  createI2CInterface(),
                  std::move(presenceDetection),
                  std::move(configuration),
                  std::move(phaseFaultDetection),
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

TEST_F(DeviceTests, ClearCache)
{
    // Test where Device does not contain a PresenceDetection object
    try
    {
        Device device{"vdd_reg", false, deviceInvPath, createI2CInterface()};
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
        auto presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));
        PresenceDetection* presenceDetectionPtr = presenceDetection.get();

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        Device device{"reg2", true, deviceInvPath, std::move(i2cInterface),
                      std::move(presenceDetection)};

        // Cache presence value in PresenceDetection
        MockServices services{};
        presenceDetectionPtr->execute(services, *system, *chassis, device);
        EXPECT_TRUE(presenceDetectionPtr->getCachedPresence().has_value());

        // Clear cached data in Device
        device.clearCache();

        // Verify presence value no longer cached in PresenceDetection
        EXPECT_FALSE(presenceDetectionPtr->getCachedPresence().has_value());
    }
}

TEST_F(DeviceTests, ClearErrorHistory)
{
    // Create SensorMonitoring.  Will fail with a DBus exception.
    std::unique_ptr<SensorMonitoring> sensorMonitoring{};
    {
        auto action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute)
            .WillRepeatedly(Throw(TestSDBusError{"DBus Error"}));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        sensorMonitoring =
            std::make_unique<SensorMonitoring>(std::move(actions));
    }

    // Create Rail
    std::unique_ptr<Configuration> configuration{};
    auto rail = std::make_unique<Rail>("vdd", std::move(configuration),
                                       std::move(sensorMonitoring));

    // Create PhaseFaultDetection.  Will log an N phase fault.
    std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
    {
        auto action = std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        phaseFaultDetection =
            std::make_unique<PhaseFaultDetection>(std::move(actions));
    }

    // Create Device
    auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
    std::unique_ptr<PresenceDetection> presenceDetection{};
    std::unique_ptr<Configuration> deviceConfiguration{};
    std::vector<std::unique_ptr<Rail>> rails{};
    rails.emplace_back(std::move(rail));
    Device device{"reg2",
                  true,
                  deviceInvPath,
                  std::move(i2cInterface),
                  std::move(presenceDetection),
                  std::move(deviceConfiguration),
                  std::move(phaseFaultDetection),
                  std::move(rails)};

    // Create lambda that sets MockServices expectations.  The lambda allows
    // us to set expectations multiple times without duplicate code.
    auto setExpectations = [](MockServices& services) {
        // Set Journal service expectations:
        // - 6 error messages for D-Bus errors
        // - 6 error messages for inability to monitor sensors
        // - 2 error messages for the N phase fault
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(std::vector<std::string>{"DBus Error"}))
            .Times(6);
        EXPECT_CALL(journal, logError("Unable to monitor sensors for rail vdd"))
            .Times(6);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator reg2: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator reg2: count=2"))
            .Times(1);

        // Set ErrorLogging service expectations:
        // - D-Bus error should be logged once for the D-Bus exceptions
        // - N phase fault error should be logged once
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logDBusError).Times(1);
        EXPECT_CALL(errorLogging, logPhaseFault).Times(1);

        // Set Sensors service expections:
        // - startRail() and endRail() called 10 times
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail).Times(10);
        EXPECT_CALL(sensors, endRail).Times(10);
    };

    // Monitor sensors and detect phase faults 10 times.  Verify errors logged.
    {
        // Create mock services.  Set expectations via lambda.
        MockServices services{};
        setExpectations(services);

        for (int i = 1; i <= 10; ++i)
        {
            device.monitorSensors(services, *system, *chassis);
            device.detectPhaseFaults(services, *system, *chassis);
        }
    }

    // Clear error history
    device.clearErrorHistory();

    // Monitor sensors and detect phase faults 10 more times.  Verify errors
    // logged again.
    {
        // Create mock services.  Set expectations via lambda.
        MockServices services{};
        setExpectations(services);

        for (int i = 1; i <= 10; ++i)
        {
            device.monitorSensors(services, *system, *chassis);
            device.detectPhaseFaults(services, *system, *chassis);
        }
    }
}

TEST_F(DeviceTests, Close)
{
    // Test where works: I2C interface is not open
    {
        // Create mock I2CInterface
        auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*i2cInterface, close).Times(0);

        // Create mock services.  No logError should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::vector<std::string>&>()))
            .Times(0);

        // Create Device
        Device device{"vdd_reg", true, deviceInvPath, std::move(i2cInterface)};

        // Close Device
        device.close(services);
    }

    // Test where works: I2C interface is open
    {
        // Create mock I2CInterface
        auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, close).Times(1);

        // Create mock services.  No logError should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::vector<std::string>&>()))
            .Times(0);

        // Create Device
        Device device{"vdd_reg", true, deviceInvPath, std::move(i2cInterface)};

        // Close Device
        device.close(services);
    }

    // Test where fails: closing I2C interface fails
    {
        // Create mock I2CInterface
        auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
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
        Device device{"vdd_reg", true, deviceInvPath, std::move(i2cInterface)};

        // Close Device
        device.close(services);
    }
}

TEST_F(DeviceTests, Configure)
{
    // Test where device is not present
    {
        // Create mock services.  No logging should occur.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create PresenceDetection.  Indicates device is not present.
        std::unique_ptr<PresenceDetection> presenceDetection{};
        {
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            presenceDetection =
                std::make_unique<PresenceDetection>(std::move(actions));
        }

        // Create Configuration.  Action inside it should not be executed.
        std::unique_ptr<Configuration> configuration{};
        {
            std::optional<double> volts{};
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(0);
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            configuration =
                std::make_unique<Configuration>(volts, std::move(actions));
        }

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        Device device{"reg2",
                      true,
                      deviceInvPath,
                      std::move(i2cInterface),
                      std::move(presenceDetection),
                      std::move(configuration)};

        // Call configure().  Should do nothing.
        device.configure(services, *system, *chassis);
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
        Device device{"reg2", true, deviceInvPath, std::move(i2cInterface)};

        // Call configure().
        device.configure(services, *system, *chassis);
    }

    // Test where Configuration and Rails were specified in constructor
    {
        std::vector<std::unique_ptr<Rail>> rails{};

        // Create mock services.  Expect logDebug() to be called.
        // For the Device and both Rails, should execute the Configuration
        // and log a debug message.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Configuring reg2")).Times(1);
        EXPECT_CALL(journal, logDebug("Configuring vdd0: volts=1.300000"))
            .Times(1);
        EXPECT_CALL(journal, logDebug("Configuring vio0: volts=3.200000"))
            .Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Rail vdd0
        {
            // Create Configuration for Rail
            std::optional<double> volts{1.3};
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            auto configuration =
                std::make_unique<Configuration>(volts, std::move(actions));

            // Create Rail
            auto rail =
                std::make_unique<Rail>("vdd0", std::move(configuration));
            rails.emplace_back(std::move(rail));
        }

        // Create Rail vio0
        {
            // Create Configuration for Rail
            std::optional<double> volts{3.2};
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            auto configuration =
                std::make_unique<Configuration>(volts, std::move(actions));

            // Create Rail
            auto rail =
                std::make_unique<Rail>("vio0", std::move(configuration));
            rails.emplace_back(std::move(rail));
        }

        // Create Configuration for Device
        std::optional<double> volts{};
        auto action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        auto configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
        Device device{"reg2",
                      true,
                      deviceInvPath,
                      std::move(i2cInterface),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(phaseFaultDetection),
                      std::move(rails)};

        // Call configure().
        device.configure(services, *system, *chassis);
    }
}

TEST_F(DeviceTests, DetectPhaseFaults)
{
    // Test where device is not present
    {
        // Create mock services.  No errors should be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        // Create PresenceDetection.  Indicates device is not present.
        std::unique_ptr<PresenceDetection> presenceDetection{};
        {
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            presenceDetection =
                std::make_unique<PresenceDetection>(std::move(actions));
        }

        // Create PhaseFaultDetection.  Action inside it should not be executed.
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
        {
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(0);
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            phaseFaultDetection =
                std::make_unique<PhaseFaultDetection>(std::move(actions));
        }

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<Configuration> configuration{};
        Device device{"reg2",
                      true,
                      deviceInvPath,
                      std::move(i2cInterface),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(phaseFaultDetection)};

        // Call detectPhaseFaults() 5 times.  Should do nothing.
        for (int i = 1; i <= 5; ++i)
        {
            device.detectPhaseFaults(services, *system, *chassis);
        }
    }

    // Test where PhaseFaultDetection was not specified in constructor
    {
        // Create mock services.  No errors should be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        Device device{"reg2", true, deviceInvPath, std::move(i2cInterface)};

        // Call detectPhaseFaults() 5 times.  Should do nothing.
        for (int i = 1; i <= 5; ++i)
        {
            device.detectPhaseFaults(services, *system, *chassis);
        }
    }

    // Test where PhaseFaultDetection was specified in constructor
    {
        // Create mock services with the following expectations:
        // - 2 error messages in journal for N phase fault detected
        // - 1 N phase fault error logged
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator reg2: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator reg2: count=2"))
            .Times(1);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(1);

        // Create PhaseFaultDetection
        auto action = std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        auto phaseFaultDetection =
            std::make_unique<PhaseFaultDetection>(std::move(actions));

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> configuration{};
        Device device{"reg2",
                      true,
                      deviceInvPath,
                      std::move(i2cInterface),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(phaseFaultDetection)};

        // Call detectPhaseFaults() 5 times
        for (int i = 1; i <= 5; ++i)
        {
            device.detectPhaseFaults(services, *system, *chassis);
        }
    }
}

TEST_F(DeviceTests, GetConfiguration)
{
    // Test where Configuration was not specified in constructor
    {
        Device device{"vdd_reg", true, deviceInvPath, createI2CInterface()};
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
        auto configuration =
            std::make_unique<Configuration>(volts, std::move(actions));

        // Create Device
        Device device{"vdd_reg",
                      true,
                      deviceInvPath,
                      createI2CInterface(),
                      std::move(presenceDetection),
                      std::move(configuration)};
        EXPECT_NE(device.getConfiguration(), nullptr);
        EXPECT_EQ(device.getConfiguration()->getVolts().has_value(), true);
        EXPECT_EQ(device.getConfiguration()->getVolts().value(), 3.2);
        EXPECT_EQ(device.getConfiguration()->getActions().size(), 2);
    }
}

TEST_F(DeviceTests, GetFRU)
{
    Device device{"vdd_reg", true, deviceInvPath, createI2CInterface()};
    EXPECT_EQ(device.getFRU(), deviceInvPath);
}

TEST_F(DeviceTests, GetI2CInterface)
{
    std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
    i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();
    Device device{"vdd_reg", true, deviceInvPath, std::move(i2cInterface)};
    EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
}

TEST_F(DeviceTests, GetID)
{
    Device device{"vdd_reg", false, deviceInvPath, createI2CInterface()};
    EXPECT_EQ(device.getID(), "vdd_reg");
}

TEST_F(DeviceTests, GetPhaseFaultDetection)
{
    // Test where PhaseFaultDetection was not specified in constructor
    {
        Device device{"vdd_reg", true, deviceInvPath, createI2CInterface()};
        EXPECT_EQ(device.getPhaseFaultDetection(), nullptr);
    }

    // Test where PhaseFaultDetection was specified in constructor
    {
        // Create PhaseFaultDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        auto phaseFaultDetection =
            std::make_unique<PhaseFaultDetection>(std::move(actions));

        // Create Device
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> configuration{};
        Device device{"vdd_reg",
                      false,
                      deviceInvPath,
                      createI2CInterface(),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(phaseFaultDetection)};
        EXPECT_NE(device.getPhaseFaultDetection(), nullptr);
        EXPECT_EQ(device.getPhaseFaultDetection()->getActions().size(), 1);
    }
}

TEST_F(DeviceTests, GetPresenceDetection)
{
    // Test where PresenceDetection was not specified in constructor
    {
        Device device{"vdd_reg", true, deviceInvPath, createI2CInterface()};
        EXPECT_EQ(device.getPresenceDetection(), nullptr);
    }

    // Test where PresenceDetection was specified in constructor
    {
        // Create PresenceDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        auto presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));

        // Create Device
        Device device{"vdd_reg", false, deviceInvPath, createI2CInterface(),
                      std::move(presenceDetection)};
        EXPECT_NE(device.getPresenceDetection(), nullptr);
        EXPECT_EQ(device.getPresenceDetection()->getActions().size(), 1);
    }
}

TEST_F(DeviceTests, GetRails)
{
    // Test where no rails were specified in constructor
    {
        Device device{"vdd_reg", true, deviceInvPath, createI2CInterface()};
        EXPECT_EQ(device.getRails().size(), 0);
    }

    // Test where rails were specified in constructor
    {
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};

        // Create vector of Rail objects
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.push_back(std::make_unique<Rail>("vdd0"));
        rails.push_back(std::make_unique<Rail>("vdd1"));

        // Create Device
        Device device{"vdd_reg",
                      false,
                      deviceInvPath,
                      createI2CInterface(),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(phaseFaultDetection),
                      std::move(rails)};
        EXPECT_EQ(device.getRails().size(), 2);
        EXPECT_EQ(device.getRails()[0]->getID(), "vdd0");
        EXPECT_EQ(device.getRails()[1]->getID(), "vdd1");
    }
}

TEST_F(DeviceTests, IsPresent)
{
    // Test where PresenceDetection not specified in constructor
    {
        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        Device device{"reg2", true, deviceInvPath, std::move(i2cInterface)};

        // Create MockServices
        MockServices services{};

        // Since no PresenceDetection defined, isPresent() should return true
        EXPECT_TRUE(device.isPresent(services, *system, *chassis));
    }

    // Test where PresenceDetection was specified in constructor: Is present
    {
        // Create PresenceDetection.  Indicates device is present.
        auto action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        auto presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        Device device{"reg2", true, deviceInvPath, std::move(i2cInterface),
                      std::move(presenceDetection)};

        // Create MockServices
        MockServices services{};

        // PresenceDetection::execute() and isPresent() should return true
        EXPECT_TRUE(device.isPresent(services, *system, *chassis));
    }

    // Test where PresenceDetection was specified in constructor: Is not present
    {
        // Create PresenceDetection.  Indicates device is not present.
        auto action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        auto presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        Device device{"reg2", true, deviceInvPath, std::move(i2cInterface),
                      std::move(presenceDetection)};

        // Create MockServices
        MockServices services{};

        // PresenceDetection::execute() and isPresent() should return false
        EXPECT_FALSE(device.isPresent(services, *system, *chassis));
    }
}

TEST_F(DeviceTests, IsRegulator)
{
    Device device{"vdd_reg", false, deviceInvPath, createI2CInterface()};
    EXPECT_EQ(device.isRegulator(), false);
}

TEST_F(DeviceTests, MonitorSensors)
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
        std::unique_ptr<SensorMonitoring> sensorMonitoring{};
        {
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(0);
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            sensorMonitoring =
                std::make_unique<SensorMonitoring>(std::move(actions));
        }

        // Create Rail
        std::unique_ptr<Configuration> configuration{};
        auto rail = std::make_unique<Rail>("vddr1", std::move(configuration),
                                           std::move(sensorMonitoring));

        // Create PresenceDetection.  Indicates device is not present.
        std::unique_ptr<PresenceDetection> presenceDetection{};
        {
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            presenceDetection =
                std::make_unique<PresenceDetection>(std::move(actions));
        }

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<Configuration> deviceConfiguration{};
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(std::move(rail));
        Device device{"reg2",
                      true,
                      deviceInvPath,
                      std::move(i2cInterface),
                      std::move(presenceDetection),
                      std::move(deviceConfiguration),
                      std::move(phaseFaultDetection),
                      std::move(rails)};

        // Call monitorSensors().  Should do nothing.
        device.monitorSensors(services, *system, *chassis);
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
        Device device{"reg2", true, deviceInvPath, std::move(i2cInterface)};

        // Call monitorSensors().  Should do nothing.
        device.monitorSensors(services, *system, *chassis);
    }

    // Test where Rails were specified in constructor
    {
        // Create mock services.  Set Sensors service expectations.
        MockServices services{};
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail("vdd0", deviceInvPath, chassisInvPath))
            .Times(1);
        EXPECT_CALL(sensors, startRail("vio0", deviceInvPath, chassisInvPath))
            .Times(1);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail(false)).Times(2);

        std::vector<std::unique_ptr<Rail>> rails{};

        // Create Rail vdd0
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
            auto rail = std::make_unique<Rail>("vdd0", std::move(configuration),
                                               std::move(sensorMonitoring));
            rails.emplace_back(std::move(rail));
        }

        // Create Rail vio0
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
            auto rail = std::make_unique<Rail>("vio0", std::move(configuration),
                                               std::move(sensorMonitoring));
            rails.emplace_back(std::move(rail));
        }

        // Create Device that contains Rails
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<PresenceDetection> presenceDetection{};
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
        Device device{"reg2",
                      true,
                      deviceInvPath,
                      std::move(i2cInterface),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(phaseFaultDetection),
                      std::move(rails)};

        // Call monitorSensors().  Should monitor sensors in both rails.
        device.monitorSensors(services, *system, *chassis);
    }
}
