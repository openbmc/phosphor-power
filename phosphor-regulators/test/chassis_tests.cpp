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

class ChassisTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the System object needed for calling some Chassis methods.
     */
    ChassisTests() : ::testing::Test{}
    {
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassis{};
        system = std::make_unique<System>(std::move(rules), std::move(chassis));
    }

  protected:
    const std::string defaultInventoryPath{
        "/xyz/openbmc_project/inventory/system/chassis"};

    std::unique_ptr<System> system{};
};

TEST_F(ChassisTests, Constructor)
{
    // Test where works: Only required parameters are specified
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};
        EXPECT_EQ(chassis.getNumber(), 2);
        EXPECT_EQ(chassis.getInventoryPath(), defaultInventoryPath);
        EXPECT_EQ(chassis.getDevices().size(), 0);
        EXPECT_FALSE(chassis.getMonitorOptions().isPresentMonitored);
        EXPECT_FALSE(chassis.getMonitorOptions().isAvailableMonitored);
        EXPECT_FALSE(chassis.getMonitorOptions().isEnabledMonitored);
        EXPECT_TRUE(chassis.getMonitorOptions().isPowerStateMonitored);
        EXPECT_TRUE(chassis.getMonitorOptions().isPowerGoodMonitored);
    }

    // Test where works: All parameters are specified
    {
        // Create vector of Device objects
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("vdd_reg1"));
        devices.emplace_back(createDevice("vdd_reg2"));

        // Create ChassisStatusMonitorOptions
        ChassisStatusMonitorOptions options{};
        options.isPresentMonitored = true;
        options.isAvailableMonitored = true;
        options.isEnabledMonitored = false;

        // Create Chassis
        Chassis chassis{1, defaultInventoryPath, options, std::move(devices)};
        EXPECT_EQ(chassis.getNumber(), 1);
        EXPECT_EQ(chassis.getInventoryPath(), defaultInventoryPath);
        EXPECT_EQ(chassis.getDevices().size(), 2);
        EXPECT_TRUE(chassis.getMonitorOptions().isPresentMonitored);
        EXPECT_TRUE(chassis.getMonitorOptions().isAvailableMonitored);
        EXPECT_FALSE(chassis.getMonitorOptions().isEnabledMonitored);
        EXPECT_TRUE(chassis.getMonitorOptions().isPowerStateMonitored);
        EXPECT_TRUE(chassis.getMonitorOptions().isPowerGoodMonitored);
    }

    // Test where fails: Invalid chassis number < 1
    try
    {
        Chassis chassis{0, defaultInventoryPath, ChassisStatusMonitorOptions{}};
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

TEST_F(ChassisTests, AddToIDMap)
{
    // Create vector of Device objects
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(createDevice("reg1", {"rail1"}));
    devices.emplace_back(createDevice("reg2", {"rail2a", "rail2b"}));
    devices.emplace_back(createDevice("reg3"));

    // Create Chassis
    Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{},
                    std::move(devices)};

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

TEST_F(ChassisTests, CanCloseDevices)
{
    // Test where all statuses are good
    {
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        EXPECT_TRUE(chassis.canCloseDevices(services));
    }

    // Test where chassis is not present
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(false));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        EXPECT_FALSE(chassis.canCloseDevices(services));
    }

    // Test where chassis is not available
    {
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(true));
        EXPECT_CALL(mockMonitor, isAvailable()).WillOnce(Return(false));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        EXPECT_FALSE(chassis.canCloseDevices(services));
    }

    // Test where exception thrown trying to get status
    {
        Chassis chassis{4, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent())
            .WillOnce(
                Throw(std::runtime_error{"Present property value could not be "
                                         "obtained."}));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal,
                    logError("Unable to determine status of chassis 4: Present "
                             "property value could not be obtained."))
            .Times(1);

        EXPECT_TRUE(chassis.canCloseDevices(services));
    }
}

TEST_F(ChassisTests, CanConfigure)
{
    // Test where all statuses are good
    {
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        EXPECT_TRUE(chassis.canConfigure(services));
    }

    // Test where chassis is not present
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(false));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal,
                    logInfo("Unable to configure chassis 2: Chassis is not "
                            "present"))
            .Times(1);

        EXPECT_FALSE(chassis.canConfigure(services));
    }

    // Test where chassis is not enabled
    {
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(true));
        EXPECT_CALL(mockMonitor, isEnabled()).WillOnce(Return(false));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal,
                    logInfo("Unable to configure chassis 3: Chassis is not "
                            "enabled"))
            .Times(1);

        EXPECT_FALSE(chassis.canConfigure(services));
    }

    // Test where chassis is not available
    {
        Chassis chassis{4, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(true));
        EXPECT_CALL(mockMonitor, isEnabled()).WillOnce(Return(true));
        EXPECT_CALL(mockMonitor, isAvailable()).WillOnce(Return(false));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal,
                    logInfo("Unable to configure chassis 4: Chassis is not "
                            "available"))
            .Times(1);

        EXPECT_FALSE(chassis.canConfigure(services));
    }

    // Test where exception thrown trying to get status
    {
        Chassis chassis{5, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent())
            .WillOnce(
                Throw(std::runtime_error{"Present property value could not be "
                                         "obtained."}));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal,
                    logError("Unable to configure chassis 5: Present property "
                             "value could not be obtained."))
            .Times(1);

        EXPECT_FALSE(chassis.canConfigure(services));
    }
}

TEST_F(ChassisTests, CanMonitor)
{
    // Test where all statuses are good
    {
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        EXPECT_TRUE(chassis.canMonitor(services));
    }

    // Test where chassis is not present
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(false));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        EXPECT_FALSE(chassis.canMonitor(services));
    }

    // Test where chassis is not powered on
    {
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(true));
        EXPECT_CALL(mockMonitor, isPoweredOn()).WillOnce(Return(false));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        EXPECT_FALSE(chassis.canMonitor(services));
    }

    // Test where chassis is not available
    {
        Chassis chassis{4, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(true));
        EXPECT_CALL(mockMonitor, isPoweredOn()).WillOnce(Return(true));
        EXPECT_CALL(mockMonitor, isAvailable()).WillOnce(Return(false));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        EXPECT_FALSE(chassis.canMonitor(services));
    }

    // Test where exception thrown trying to get status
    {
        Chassis chassis{5, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        std::runtime_error error{
            "Present property value could not be obtained."};
        EXPECT_CALL(mockMonitor, isPresent())
            .WillOnce(Throw(error))
            .WillOnce(Throw(error))
            .WillOnce(Throw(error))
            .WillOnce(Throw(error))
            .WillOnce(Throw(error))
            .WillOnce(Return(false))
            .WillOnce(Throw(error))
            .WillOnce(Throw(error))
            .WillOnce(Throw(error))
            .WillOnce(Throw(error));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError(
                "Unable to determine status of chassis 5: Present property "
                "value could not be obtained."))
            .Times(6);

        // Call canMonitor() 10 times. Should log error message 3 times, then
        // stop until isPresent() works, then log 3 more times.
        for (auto i = 1; i <= 10; ++i)
        {
            EXPECT_FALSE(chassis.canMonitor(services));
        }
    }
}

TEST_F(ChassisTests, ClearCache)
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
    Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{},
                    std::move(devices)};

    // Cache presence value in PresenceDetection
    MockServices services{};
    presenceDetectionPtr->execute(services, *system, chassis, *devicePtr);
    EXPECT_TRUE(presenceDetectionPtr->getCachedPresence().has_value());

    // Clear cached data in Chassis
    chassis.clearCache();

    // Verify presence value no longer cached in PresenceDetection
    EXPECT_FALSE(presenceDetectionPtr->getCachedPresence().has_value());
}

TEST_F(ChassisTests, ClearErrorHistory)
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
    Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{},
                    std::move(devices)};

    // Create lambda that sets MockServices expectations.  The lambda allows
    // us to set expectations multiple times without duplicate code.
    auto setExpectations = [&chassis](MockServices& services) {
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

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
            chassis.monitorSensors(services, *system);
        }
    }

    // Clear error history
    chassis.clearErrorHistory();

    // Monitor sensors 10 more times.  Verify errors logged again.
    {
        // Create mock services.  Set expectations via lambda.
        MockServices services{};
        setExpectations(services);

        for (int i = 1; i <= 10; ++i)
        {
            chassis.monitorSensors(services, *system);
        }
    }

    // Test where getting chassis status fails
    {
        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        std::runtime_error error{
            "Present property value could not be obtained."};
        EXPECT_CALL(mockMonitor, isPresent()).WillRepeatedly(Throw(error));

        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError(
                "Unable to determine status of chassis 1: Present property "
                "value could not be obtained."))
            .Times(6);

        // Call canMonitor() 10 times. Should log error message 3 times.
        for (auto i = 1; i <= 10; ++i)
        {
            EXPECT_FALSE(chassis.canMonitor(services));
        }

        // Clear error history
        chassis.clearErrorHistory();

        // Call canMonitor() 10 more times. Should log error message 3 times.
        for (auto i = 1; i <= 10; ++i)
        {
            EXPECT_FALSE(chassis.canMonitor(services));
        }
    }
}

TEST_F(ChassisTests, CloseDevices)
{
    // Test where no devices were specified in constructor
    {
        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Closing devices in chassis 2")).Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Chassis
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        // Call closeDevices()
        chassis.closeDevices(services);
    }

    // Test where devices are closed because all statuses are good
    {
        std::vector<std::unique_ptr<Device>> devices{};

        // Create mock services.  Expect logDebug() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Closing devices in chassis 1")).Times(1);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Device vdd0_reg
        {
            // Create mock I2CInterface: isOpen() and close() should be called
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*i2cInterface, close).Times(1);

            // Create Device
            auto device = std::make_unique<Device>(
                "vdd0_reg", true,
                "/xyz/openbmc_project/inventory/"
                "system/chassis/motherboard/vdd0_reg",
                std::move(i2cInterface));
            devices.emplace_back(std::move(device));
        }

        // Create Device vdd1_reg
        {
            // Create mock I2CInterface: isOpen() and close() should be called
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*i2cInterface, close).Times(1);

            // Create Device
            auto device = std::make_unique<Device>(
                "vdd1_reg", true,
                "/xyz/openbmc_project/inventory/"
                "system/chassis/motherboard/vdd1_reg",
                std::move(i2cInterface));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        // Call closeDevices()
        chassis.closeDevices(services);
    }

    // Test where device closing is skipped because chassis is not present
    {
        std::vector<std::unique_ptr<Device>> devices{};

        // Create mock services.  No logDebug() should be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);

        // Create Device vdd0_reg
        {
            // Create mock I2CInterface: should NOT be called
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            EXPECT_CALL(*i2cInterface, isOpen).Times(0);
            EXPECT_CALL(*i2cInterface, close).Times(0);

            // Create Device
            auto device = std::make_unique<Device>(
                "vdd0_reg", true,
                "/xyz/openbmc_project/inventory/"
                "system/chassis/motherboard/vdd0_reg",
                std::move(i2cInterface));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(false));

        // Call closeDevices()
        chassis.closeDevices(services);
    }

    // Test where devices are closed because exception occurred getting chassis
    // status
    {
        std::vector<std::unique_ptr<Device>> devices{};

        // Create mock services.  Expect logDebug() and logError() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logDebug("Closing devices in chassis 4")).Times(1);
        EXPECT_CALL(journal,
                    logError("Unable to determine status of chassis 4: Present "
                             "property value could not be obtained."))
            .Times(1);

        // Create Device vdd0_reg
        {
            // Create mock I2CInterface: isOpen() and close() should be called
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*i2cInterface, close).Times(1);

            // Create Device
            auto device = std::make_unique<Device>(
                "vdd0_reg", true,
                "/xyz/openbmc_project/inventory/"
                "system/chassis/motherboard/vdd0_reg",
                std::move(i2cInterface));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{4, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent())
            .WillOnce(
                Throw(std::runtime_error{"Present property value could not be "
                                         "obtained."}));

        // Call closeDevices()
        chassis.closeDevices(services);
    }
}

TEST_F(ChassisTests, Configure)
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
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        // Call configure()
        chassis.configure(services, *system);
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
            auto configuration =
                std::make_unique<Configuration>(1.3, std::move(actions));

            // Create Device
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            auto device = std::make_unique<Device>(
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
            auto configuration =
                std::make_unique<Configuration>(1.2, std::move(actions));

            // Create Device
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            auto device = std::make_unique<Device>(
                "vdd1_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd1_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(configuration));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        // Call configure()
        chassis.configure(services, *system);
    }

    // Test where configuration is skipped because chassis status is not valid
    {
        std::vector<std::unique_ptr<Device>> devices{};

        // Create mock services.  Expect logError() to be called.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logInfo(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal, logDebug(A<const std::string&>())).Times(0);
        EXPECT_CALL(journal,
                    logInfo("Unable to configure chassis 3: Chassis is not "
                            "present"))
            .Times(1);

        // Create Device vdd0_reg
        {
            // Create Configuration
            std::vector<std::unique_ptr<Action>> actions{};
            auto configuration =
                std::make_unique<Configuration>(1.3, std::move(actions));

            // Create Device
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            auto device = std::make_unique<Device>(
                "vdd0_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd0_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(configuration));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(false));

        // Call configure()
        chassis.configure(services, *system);
    }
}

TEST_F(ChassisTests, DetectPhaseFaults)
{
    // Test where no devices were specified in constructor
    {
        // Create mock services.  No errors should be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        // Create Chassis
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        // Call detectPhaseFaults() 5 times.  Should do nothing.
        for (int i = 1; i <= 5; ++i)
        {
            chassis.detectPhaseFaults(services, *system);
        }
    }

    // Test where devices were specified in constructor
    {
        // Create mock services with the following expectations:
        // - 2 error messages in journal for N phase fault detected in reg0
        // - 2 error messages in journal for N phase fault detected in reg1
        // - 1 N phase fault error logged for reg0
        // - 1 N phase fault error logged for reg1
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator reg0: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator reg0: count=2"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator reg1: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator reg1: count=2"))
            .Times(1);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(2);

        std::vector<std::unique_ptr<Device>> devices{};

        // Create Device reg0
        {
            // Create PhaseFaultDetection
            auto action =
                std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);
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
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "reg0",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(configuration), std::move(phaseFaultDetection));
            devices.emplace_back(std::move(device));
        }

        // Create Device reg1
        {
            // Create PhaseFaultDetection
            auto action =
                std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);
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
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "reg1",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(configuration), std::move(phaseFaultDetection));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        // Call detectPhaseFaults() 5 times
        for (int i = 1; i <= 5; ++i)
        {
            chassis.detectPhaseFaults(services, *system);
        }
    }

    // Test where detection is skipped because chassis status is not valid
    {
        // Create mock services.  No errors should be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        std::vector<std::unique_ptr<Device>> devices{};

        // Create Device reg0
        {
            // Create PhaseFaultDetection
            auto action =
                std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);
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
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "reg0",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(configuration), std::move(phaseFaultDetection));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillRepeatedly(Return(true));
        EXPECT_CALL(mockMonitor, isPoweredOn()).WillRepeatedly(Return(false));

        // Call detectPhaseFaults() 5 times
        for (int i = 1; i <= 5; ++i)
        {
            chassis.detectPhaseFaults(services, *system);
        }
    }
}

TEST_F(ChassisTests, GetDevices)
{
    // Test where no devices were specified in constructor
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};
        EXPECT_EQ(chassis.getDevices().size(), 0);
    }

    // Test where devices were specified in constructor
    {
        // Create vector of Device objects
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(createDevice("vdd_reg1"));
        devices.emplace_back(createDevice("vdd_reg2"));

        // Create Chassis
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};
        EXPECT_EQ(chassis.getDevices().size(), 2);
        EXPECT_EQ(chassis.getDevices()[0]->getID(), "vdd_reg1");
        EXPECT_EQ(chassis.getDevices()[1]->getID(), "vdd_reg2");
    }
}

TEST_F(ChassisTests, GetInventoryPath)
{
    Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};
    EXPECT_EQ(chassis.getInventoryPath(), defaultInventoryPath);
}

TEST_F(ChassisTests, GetMonitorOptions)
{
    // Test where all options are false (default)
    {
        ChassisStatusMonitorOptions options{};
        Chassis chassis{1, defaultInventoryPath, options};
        const ChassisStatusMonitorOptions& returnedOptions =
            chassis.getMonitorOptions();
        EXPECT_FALSE(returnedOptions.isPresentMonitored);
        EXPECT_FALSE(returnedOptions.isAvailableMonitored);
        EXPECT_FALSE(returnedOptions.isEnabledMonitored);
    }

    // Test where all options are true
    {
        ChassisStatusMonitorOptions options{};
        options.isPresentMonitored = true;
        options.isAvailableMonitored = true;
        options.isEnabledMonitored = true;
        Chassis chassis{2, defaultInventoryPath, options};
        const ChassisStatusMonitorOptions& returnedOptions =
            chassis.getMonitorOptions();
        EXPECT_TRUE(returnedOptions.isPresentMonitored);
        EXPECT_TRUE(returnedOptions.isAvailableMonitored);
        EXPECT_TRUE(returnedOptions.isEnabledMonitored);
    }

    // Test where some options are true
    {
        ChassisStatusMonitorOptions options{};
        options.isPresentMonitored = true;
        options.isAvailableMonitored = false;
        options.isEnabledMonitored = true;
        Chassis chassis{3, defaultInventoryPath, options};
        const ChassisStatusMonitorOptions& returnedOptions =
            chassis.getMonitorOptions();
        EXPECT_TRUE(returnedOptions.isPresentMonitored);
        EXPECT_FALSE(returnedOptions.isAvailableMonitored);
        EXPECT_TRUE(returnedOptions.isEnabledMonitored);
    }
}

TEST_F(ChassisTests, GetNumber)
{
    Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};
    EXPECT_EQ(chassis.getNumber(), 3);
}

TEST_F(ChassisTests, GetStatusMonitor)
{
    Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};

    // Test where fails: Monitoring not initialized
    {
        try
        {
            chassis.getStatusMonitor();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 2");
        }
    }

    // Test where works
    {
        MockServices services{};
        chassis.initializeMonitoring(services);
        ChassisStatusMonitor& monitor = chassis.getStatusMonitor();
        MockChassisStatusMonitor& mockMonitor =
            static_cast<MockChassisStatusMonitor&>(monitor);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(true));
        EXPECT_TRUE(chassis.isPresent());
    }
}

TEST_F(ChassisTests, InitializeMonitoring)
{
    Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};
    EXPECT_THROW(chassis.getStatusMonitor(), std::runtime_error);
    MockServices services{};
    chassis.initializeMonitoring(services);
    EXPECT_NO_THROW(chassis.getStatusMonitor());
}

TEST_F(ChassisTests, IsAvailable)
{
    // Test where works
    {
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isAvailable())
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        EXPECT_TRUE(chassis.isAvailable());
        EXPECT_FALSE(chassis.isAvailable());
    }

    // Test where fails: Monitoring has not been initialized
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};
        try
        {
            chassis.isAvailable();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 2");
        }
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    {
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isAvailable())
            .WillOnce(
                Throw(std::runtime_error{"Available property value could not "
                                         "be obtained."}));

        try
        {
            chassis.isAvailable();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(),
                         "Available property value could not be obtained.");
        }
    }
}

TEST_F(ChassisTests, IsEnabled)
{
    // Test where works
    {
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isEnabled())
            .WillOnce(Return(false))
            .WillOnce(Return(true));

        EXPECT_FALSE(chassis.isEnabled());
        EXPECT_TRUE(chassis.isEnabled());
    }

    // Test where fails: Monitoring has not been initialized
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};
        try
        {
            chassis.isEnabled();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 2");
        }
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    {
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isEnabled())
            .WillOnce(
                Throw(std::runtime_error{"Enabled property value could not be "
                                         "obtained."}));

        try
        {
            chassis.isEnabled();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(),
                         "Enabled property value could not be obtained.");
        }
    }
}

TEST_F(ChassisTests, IsPoweredOn)
{
    // Test where works
    {
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPoweredOn())
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        EXPECT_TRUE(chassis.isPoweredOn());
        EXPECT_FALSE(chassis.isPoweredOn());
    }

    // Test where fails: Monitoring has not been initialized
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};
        try
        {
            chassis.isPoweredOn();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 2");
        }
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    {
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPoweredOn())
            .WillOnce(Throw(std::runtime_error{
                "Power good property value could not be obtained."}));

        try
        {
            chassis.isPoweredOn();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(),
                         "Power good property value could not be obtained.");
        }
    }
}

TEST_F(ChassisTests, IsPresent)
{
    // Test where works
    {
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent())
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        EXPECT_TRUE(chassis.isPresent());
        EXPECT_FALSE(chassis.isPresent());
    }

    // Test where fails: Monitoring has not been initialized
    {
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{}};
        try
        {
            chassis.isPresent();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 2");
        }
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    {
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        MockServices services{};
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent())
            .WillOnce(
                Throw(std::runtime_error{"Present property value could not be "
                                         "obtained."}));

        try
        {
            chassis.isPresent();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(),
                         "Present property value could not be obtained.");
        }
    }
}

TEST_F(ChassisTests, MonitorSensors)
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
        Chassis chassis{1, defaultInventoryPath, ChassisStatusMonitorOptions{}};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        // Call monitorSensors().  Should do nothing.
        chassis.monitorSensors(services, *system);
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

            // Create Device
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            std::unique_ptr<Configuration> deviceConfiguration{};
            std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
            std::vector<std::unique_ptr<Rail>> rails{};
            rails.emplace_back(std::move(rail));
            auto device = std::make_unique<Device>(
                "vdd0_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd0_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(deviceConfiguration), std::move(phaseFaultDetection),
                std::move(rails));
            devices.emplace_back(std::move(device));
        }

        // Create Device vdd1_reg
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
            auto rail = std::make_unique<Rail>("vdd1", std::move(configuration),
                                               std::move(sensorMonitoring));

            // Create Device
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            std::unique_ptr<Configuration> deviceConfiguration{};
            std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
            std::vector<std::unique_ptr<Rail>> rails{};
            rails.emplace_back(std::move(rail));
            auto device = std::make_unique<Device>(
                "vdd1_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd1_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(deviceConfiguration), std::move(phaseFaultDetection),
                std::move(rails));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis that contains Devices
        Chassis chassis{2, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        setChassisStatusToGood(chassis);

        // Call monitorSensors()
        chassis.monitorSensors(services, *system);
    }

    // Test where monitoring is skipped because chassis status is not valid
    {
        // Create mock services.  No Sensors methods should be called.
        MockServices services{};
        MockSensors& sensors = services.getMockSensors();
        EXPECT_CALL(sensors, startRail).Times(0);
        EXPECT_CALL(sensors, setValue).Times(0);
        EXPECT_CALL(sensors, endRail).Times(0);

        std::vector<std::unique_ptr<Device>> devices{};

        // Create Device vdd0_reg
        {
            // Create SensorMonitoring for Rail
            auto action = std::make_unique<MockAction>();
            EXPECT_CALL(*action, execute).Times(0);
            std::vector<std::unique_ptr<Action>> actions{};
            actions.emplace_back(std::move(action));
            auto sensorMonitoring =
                std::make_unique<SensorMonitoring>(std::move(actions));

            // Create Rail
            std::unique_ptr<Configuration> configuration{};
            auto rail = std::make_unique<Rail>("vdd0", std::move(configuration),
                                               std::move(sensorMonitoring));

            // Create Device
            auto i2cInterface = std::make_unique<i2c::MockedI2CInterface>();
            std::unique_ptr<PresenceDetection> presenceDetection{};
            std::unique_ptr<Configuration> deviceConfiguration{};
            std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
            std::vector<std::unique_ptr<Rail>> rails{};
            rails.emplace_back(std::move(rail));
            auto device = std::make_unique<Device>(
                "vdd0_reg", true,
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "vdd0_reg",
                std::move(i2cInterface), std::move(presenceDetection),
                std::move(deviceConfiguration), std::move(phaseFaultDetection),
                std::move(rails));
            devices.emplace_back(std::move(device));
        }

        // Create Chassis
        Chassis chassis{3, defaultInventoryPath, ChassisStatusMonitorOptions{},
                        std::move(devices)};

        // Initialize monitoring
        chassis.initializeMonitoring(services);
        MockChassisStatusMonitor& mockMonitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(mockMonitor, isPresent()).WillOnce(Return(false));

        // Call monitorSensors()
        chassis.monitorSensors(services, *system);
    }
}
