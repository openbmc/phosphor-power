/**
 * Copyright © 2025 IBM Corporation
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

#include "config.h"

#include "chassis.hpp"
#include "chassis_status_monitor.hpp"
#include "mock_chassis_status_monitor.hpp"
#include "mock_device.hpp"
#include "mock_services.hpp"
#include "power_interface.hpp"
#include "power_sequencer_device.hpp"
#include "system.hpp"

#include <stddef.h> // for size_t

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Throw;

/**
 * Creates a Chassis object with one mock power sequencer device.
 *
 * @param number Chassis number within the system. Must be >= 1.
 * @param inventoryPath D-Bus inventory path of the chassis
 * @return Chassis object
 */
std::unique_ptr<Chassis> createChassis(size_t number,
                                       const std::string& inventoryPath)
{
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    powerSequencers.emplace_back(std::make_unique<MockDevice>());
    ChassisStatusMonitorOptions monitorOptions;
    return std::make_unique<Chassis>(
        number, inventoryPath, std::move(powerSequencers), monitorOptions);
}

/**
 * Returns the MockChassisStatusMonitor for the Chassis object at the specified
 * index within the system's vector of Chassis.
 *
 * Assumes that Chassis::initializeMonitoring() has been called with a
 * MockServices parameter.
 *
 * Throws an exception if initializeMonitoring() has not been called.
 *
 * @param system System object
 * @param i Index of chassis
 * @return MockChassisStatusMonitor reference
 */
MockChassisStatusMonitor& getMockStatusMonitor(System& system, int i)
{
    return static_cast<MockChassisStatusMonitor&>(
        system.getChassis()[i]->getStatusMonitor());
}

/**
 * Sets up the MockChassisStatusMonitor for the specified Chassis to repeatedly
 * return good status for all properties.
 *
 * Throws an exception if initializeMonitoring() has not been called.
 *
 * @param system System object
 * @param i Index of chassis
 */
void setChassisStatusToGood(System& system, int i)
{
    auto& monitor = getMockStatusMonitor(system, i);
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
}

/**
 * Sets up the MockChassisStatusMonitor for the specified Chassis to repeatedly
 * return good status for all properties except isPresent.
 *
 * Throws an exception if initializeMonitoring() has not been called.
 *
 * @param system System object
 * @param i Index of chassis
 */
void setChassisStatusToGoodExceptIsPresent(System& system, int i)
{
    auto& monitor = getMockStatusMonitor(system, i);
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
}

/**
 * Sets up the MockChassisStatusMonitor for the specified Chassis to repeatedly
 * return good status for all properties except isEnabled.
 *
 * Throws an exception if initializeMonitoring() has not been called.
 *
 * @param system System object
 * @param i Index of chassis
 */
void setChassisStatusToGoodExceptIsEnabled(System& system, int i)
{
    auto& monitor = getMockStatusMonitor(system, i);
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
}

/**
 * Sets up the MockChassisStatusMonitor for the specified Chassis to repeatedly
 * return good status for all properties except isAvailable.
 *
 * Throws an exception if initializeMonitoring() has not been called.
 *
 * @param system System object
 * @param i Index of chassis
 */
void setChassisStatusToGoodExceptIsAvailable(System& system, int i)
{
    auto& monitor = getMockStatusMonitor(system, i);
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
}

/**
 * Sets up the MockChassisStatusMonitor for the specified Chassis to repeatedly
 * return good status for all properties except isInputPowerGood.
 *
 * Throws an exception if initializeMonitoring() has not been called.
 *
 * @param system System object
 * @param i Index of chassis
 */
void setChassisStatusToGoodExceptIsInputPowerGood(System& system, int i)
{
    auto& monitor = getMockStatusMonitor(system, i);
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
}

/**
 * Returns the MockDevice for the Chassis object at the specified index within
 * the system's vector of Chassis.
 *
 * @param system System object
 * @param i Index of chassis
 * @return MockDevice reference
 */
MockDevice& getMockDevice(System& system, int i)
{
    return static_cast<MockDevice&>(
        *(system.getChassis()[i]->getPowerSequencers()[0]));
}

/*
 * Note: There is one TEST per method in the System class. Since private
 * methods cannot be called directly from a testcase, private methods are tested
 * indirectly by calling public methods.
 */

TEST(SystemTests, Constructor)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis"));
    System system{std::move(chassis)};

    EXPECT_EQ(system.getChassis().size(), 1);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_EQ(system.getChassis()[0]->getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis");
    EXPECT_THROW(system.getPowerState(), std::runtime_error);
    EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    EXPECT_FALSE(system.isInPowerStateTransition());
    EXPECT_FALSE(system.hasDBusInterface());
    EXPECT_THROW(system.getDBusInterface(), std::runtime_error);
}

TEST(SystemTests, GetChassis)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    chassis.emplace_back(
        createChassis(3, "/xyz/openbmc_project/inventory/system/chassis_3"));
    chassis.emplace_back(
        createChassis(7, "/xyz/openbmc_project/inventory/system/chassis7"));
    System system{std::move(chassis)};

    EXPECT_EQ(system.getChassis().size(), 3);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_EQ(system.getChassis()[0]->getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis1");
    EXPECT_EQ(system.getChassis()[1]->getNumber(), 3);
    EXPECT_EQ(system.getChassis()[1]->getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis_3");
    EXPECT_EQ(system.getChassis()[2]->getNumber(), 7);
    EXPECT_EQ(system.getChassis()[2]->getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis7");
}

TEST(SystemTests, InitializeMonitoring)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    chassis.emplace_back(
        createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
    System system{std::move(chassis)};
    MockServices services;

    // Test before initializeMonitoring() is called
    EXPECT_THROW(system.getChassis()[0]->getStatusMonitor(),
                 std::runtime_error);
    EXPECT_THROW(system.getChassis()[1]->getStatusMonitor(),
                 std::runtime_error);

    // Test after initializeMonitoring() is called
    system.initializeMonitoring(services);
    system.getChassis()[0]->getStatusMonitor();
    system.getChassis()[1]->getStatusMonitor();
}

TEST(SystemTests, GetPowerState)
{
    // Test where fails: Power state not set
    try
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};

        system.getPowerState();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "System power state could not be obtained");
    }

    // Test where works: Value is off: Initial value set by monitor()
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
    }

    // Test where works: Value is on: Explicitly set by setPowerState()
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);

        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
    }
}

TEST(SystemTests, SetPowerState)
{
    // Test where fails: Monitoring not initialized
    try
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "System monitoring has not been initialized");
    }

    // Test where fails: Already at specified state: off: State initialized by
    // monitor()
    try
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);

        system.setPowerState(PowerState::off, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to set system to state off: Already at requested state");
    }

    // Test where fails: Already at specified state: on: State initialized by
    // previous call to setPowerState()
    try
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);

        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);

        system.setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to set system to state on: Already at requested state");
    }

    // Test where fails: No chassis can be set to specified state
    try
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillOnce(Return(false));
        }
        {
            setChassisStatusToGoodExceptIsEnabled(system, 1);
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isEnabled).WillOnce(Return(false));
        }
        EXPECT_CALL(
            services,
            logInfoMsg(
                "Unable to set chassis 1 to state on: Chassis is not present"));
        EXPECT_CALL(
            services,
            logInfoMsg(
                "Unable to set chassis 2 to state on: Chassis is not enabled"));

        system.setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to set system to state on: No chassis can be set to that state");
    }

    // Test where works: on: Is initial state: All chassis selected: First one
    // fails: Verify continues to second chassis
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        std::string deviceName{"pseq0"};
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn)
                .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
            EXPECT_CALL(device, getName).WillOnce(ReturnRef(deviceName));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to set chassis 1 to state on: Unable to power on device pseq0 in chassis 1: Unable to write GPIO"));

        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_TRUE(system.isInPowerStateTransition());
    }

    // Test where works: off: Is not initial state: Only some chassis selected
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsAvailable(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).Times(0);
            EXPECT_CALL(device, getPowerGood).Times(0);
            EXPECT_CALL(device, powerOff).Times(0);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg(
                "Unable to set chassis 1 to state off: Chassis is not available"));
        EXPECT_CALL(services, logInfoMsg("Powering off system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering off chassis 2")).Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_FALSE(system.isInPowerStateTransition());

        system.setPowerState(PowerState::off, services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_TRUE(system.isInPowerStateTransition());
    }

    // Test where error history is cleared
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))   // initial monitor
            .WillOnce(Return(false))  // monitor that detects pgood fault
            .WillOnce(Return(false))  // monitor that logs pgood fault
            .WillOnce(Return(false))  // extra monitor #1 after pgood fault
            .WillOnce(Return(false))  // extra monitor #2 after pgood fault
            .WillOnce(Return(false))  // monitor after power off
            .WillOnce(Return(true))   // monitor after power on
            .WillOnce(Return(false))  // monitor that detects pgood fault
            .WillOnce(Return(false)); // monitor that logs pgood fault
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, powerOff).Times(1);
        EXPECT_CALL(device, findPgoodFault).WillRepeatedly(Return(""));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(2);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.Shutdown",
                             Entry::Level::Critical, _))
            .Times(2);
        EXPECT_CALL(services, createBMCDump).Times(2);
        EXPECT_CALL(services, hardPowerOff).Times(2);
        EXPECT_CALL(services, logInfoMsg("Powering off system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        std::chrono::milliseconds delay{0};
        system.getChassis()[0]->setPowerGoodFaultLogDelay(delay);

        // Initial monitor. System is on. Not in transition.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor. Chassis power good is false. Power good fault. Error is not
        // logged until a delay timer expires.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor. Power good fault delay timer has expired. Power good fault
        // is logged. System creates BMC dump and powers off system via systemd.
        // Since power off is mocked, no power off actually happens. Need to do
        // manually later in this test.
        system.monitor(services);

        // Monitor two more times. This should not cause additional dumps or
        // power off requests.
        system.monitor(services);
        system.monitor(services);

        // Power off system (this would normally happen via systemd)
        system.setPowerState(PowerState::off, services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor. System is off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Power on. This clears the error history.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor. System is on.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor. Chassis power good is false. Power good fault.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor. Logs power good fault. System creates BMC dump and powers
        // off system via systemd.
        system.monitor(services);
    }
}

TEST(SystemTests, GetSelectedChassis)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    chassis.emplace_back(
        createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
    System system{std::move(chassis)};
    MockServices services;

    // Test where empty
    EXPECT_TRUE(system.getSelectedChassis().empty());

    // Test where not empty
    system.initializeMonitoring(services);
    {
        setChassisStatusToGoodExceptIsAvailable(system, 0);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(false));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).Times(0);
        EXPECT_CALL(device, getPowerGood).Times(0);
    }
    {
        setChassisStatusToGood(system, 1);

        auto& device = getMockDevice(system, 1);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
    }
    EXPECT_CALL(services,
                logInfoMsg("Chassis 2 power state is on and power good is on"))
        .Times(1);
    EXPECT_CALL(services,
                logInfoMsg("System power state is on and power good is on"))
        .Times(1);

    system.monitor(services);
    EXPECT_EQ(system.getSelectedChassis().size(), 1);
    EXPECT_TRUE(system.getSelectedChassis().contains(2));
}

TEST(SystemTests, GetPowerGood)
{
    // Test where fails: Power good not set
    try
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};

        system.getPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "System power good could not be obtained");
    }

    // Test where works: Value is off
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    }

    // Test where works: Value is on
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }
}

TEST(SystemTests, IsInPowerStateTransition)
{
    // Tested by SetPowerState
}

TEST(SystemTests, Monitor)
{
    // Test where fails: Monitoring not initialized
    try
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.monitor(services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "System monitoring has not been initialized");
    }

    // Test where fails: Cannot monitor any chassis
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent)
                .WillRepeatedly(Throw(std::runtime_error{
                    "Present property value could not be obtained."}));
        }
        {
            setChassisStatusToGoodExceptIsPresent(system, 1);
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent)
                .WillRepeatedly(Throw(std::runtime_error{
                    "Present property value could not be obtained."}));
        }
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to monitor chassis 1: Present property value could not be obtained."));
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to monitor chassis 2: Present property value could not be obtained."));

        system.monitor(services);
        EXPECT_THROW(system.getPowerState(), std::runtime_error);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
        EXPECT_TRUE(system.getSelectedChassis().empty());
    }

    // Test where works: Can only monitor some chassis. First chassis throws
    // exception. Verify continues to second chassis. Sets initial selected
    // chassis. Sets power good and initial power state. Sets value of in state
    // transition.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent)
                .WillRepeatedly(Throw(std::runtime_error{
                    "Present property value could not be obtained."}));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to monitor chassis 1: Present property value could not be obtained."));

        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
        EXPECT_FALSE(system.isInPowerStateTransition());
    }

    // Test where works: Can monitor all chassis. Updates in state transition.
    // Detects power good fault.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(false))       // Initial monitor
                .WillRepeatedly(Return(true)); // Monitor after power on
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(false))  // Initial monitor
                .WillOnce(Return(true))   // Monitor after power on
                .WillOnce(Return(false)); // Power good fault
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, findPgoodFault).WillRepeatedly(Return(""));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 2"))
            .Times(1);

        // Initial monitor. System is off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Power on system. In power state transition is true.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor. System is on. In power state transition is false.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor again. Power good fault detected.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_FALSE(system.isInPowerStateTransition());
    }

    // Test where works: Detects chassis with invalid status.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGoodExceptIsPresent(system, 0);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent)
            .WillOnce(Return(true)) // Chassis::updatePowerGood
            .WillOnce(Return(true)) // Chassis::updatePowerGood again
            .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
            .WillOnce(Return(true)) // System::setInitialSelectedChassisIfNeeded
            .WillOnce(Return(true)) // Chassis::canSetPowerState
            .WillOnce(Return(true)) // Chassis::canSetPowerState again
            .WillRepeatedly(Return(false));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, close).Times(1);

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Chassis 1 requested power state is on, but chassis is not present"))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerOff).Times(1);

        // Initial monitor. System is off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Power on system. In power state transition is true.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor. Chassis 1 is missing. Creates dump and powers off system.
        system.monitor(services);
    }
}

TEST(SystemTests, SetPowerGoodTimeout)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    chassis.emplace_back(
        createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
    System system{std::move(chassis)};

    // Verify the default chassis timeout values, which are set by a meson
    // option expressed in seconds. Chassis::getPowerGoodTimeout() returns
    // duration expressed in milliseconds. std::chrono classes are able to
    // handle conversion.
    std::chrono::seconds defaultValueInSecs{PGOOD_TIMEOUT};
    for (auto& chassis : system.getChassis())
    {
        EXPECT_EQ(chassis->getPowerGoodTimeout(), defaultValueInSecs);
        EXPECT_EQ(chassis->getPowerGoodTimeout().count(),
                  (PGOOD_TIMEOUT * 1000));
    }

    // Set to new value in seconds
    std::chrono::seconds newValueInSecs{5};
    system.setPowerGoodTimeout(newValueInSecs);
    for (auto& chassis : system.getChassis())
    {
        EXPECT_EQ(chassis->getPowerGoodTimeout(), newValueInSecs);
        EXPECT_EQ(chassis->getPowerGoodTimeout().count(), 5000);
    }

    // Set to new value in milliseconds
    std::chrono::milliseconds newValueInMillis{400};
    system.setPowerGoodTimeout(newValueInMillis);
    for (auto& chassis : system.getChassis())
    {
        EXPECT_EQ(chassis->getPowerGoodTimeout(), newValueInMillis);
        EXPECT_EQ(chassis->getPowerGoodTimeout().count(), 400);
    }

    // Verify value is set in D-Bus interface
    {
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        system.monitor(services);
        EXPECT_TRUE(system.hasDBusInterface());
        EXPECT_EQ(system.getDBusInterface().getPgoodTimeoutProperty(),
                  PGOOD_TIMEOUT);

        system.setPowerGoodTimeout(std::chrono::seconds{5});
        EXPECT_EQ(system.getDBusInterface().getPgoodTimeoutProperty(), 5);
    }
}

TEST(SystemTests, SetPowerSupplyError)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    chassis.emplace_back(
        createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
    System system{std::move(chassis)};

    // Verify default value is the empty string
    for (auto& chassis : system.getChassis())
    {
        EXPECT_TRUE(chassis->getPowerSupplyError().empty());
    }

    // Set to error string
    std::string error{
        "xyz.openbmc_project.Power.PowerSupply.Error.IoutOCFault"};
    system.setPowerSupplyError(error);
    for (auto& chassis : system.getChassis())
    {
        EXPECT_EQ(chassis->getPowerSupplyError(), error);
    }

    // Set to empty string
    system.setPowerSupplyError("");
    for (auto& chassis : system.getChassis())
    {
        EXPECT_TRUE(chassis->getPowerSupplyError().empty());
    }
}

TEST(SystemTests, ClearErrorHistory)
{
    // Tested by SetPowerState
}

TEST(SystemTests, HasDBusInterface)
{
    // The test variations are covered by SetPowerGoodValue
}

TEST(SystemTests, GetDBusInterface)
{
    // The test variations are covered by SetPowerGoodValue
}

TEST(SystemTests, SetPowerStateValue)
{
    // The test variations are covered by SetPowerGoodValue
}

TEST(SystemTests, SetPowerGoodValue)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    System system{std::move(chassis)};
    MockServices services;

    system.initializeMonitoring(services);

    setChassisStatusToGood(system, 0);

    auto& device = getMockDevice(system, 0);
    EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
    EXPECT_CALL(device, powerOn).Times(1);
    EXPECT_CALL(device, getPowerGood)
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(device, powerOff).Times(1);

    EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
    EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
    EXPECT_CALL(services,
                logInfoMsg("Chassis 1 power state is on and power good is on"))
        .Times(1);
    EXPECT_CALL(services,
                logInfoMsg("System power state is on and power good is on"))
        .Times(1);
    EXPECT_CALL(services, logInfoMsg("Powering off system")).Times(1);
    EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

    // Verify power state, power good, and D-Bus interface initially not valid
    EXPECT_THROW(system.getPowerState(), std::runtime_error);
    EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    EXPECT_FALSE(system.hasDBusInterface());
    EXPECT_THROW(system.getDBusInterface(), std::runtime_error);

    // Power on system. Power good and D-Bus interface not valid.
    system.setPowerState(PowerState::on, services);
    EXPECT_EQ(system.getPowerState(), PowerState::on);
    EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    EXPECT_FALSE(system.hasDBusInterface());

    // Monitor. Power good on. Creates D-Bus interface.
    system.monitor(services);
    EXPECT_EQ(system.getPowerState(), PowerState::on);
    EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    EXPECT_TRUE(system.hasDBusInterface());
    EXPECT_EQ(system.getDBusInterface().getChassisNumber(), 0);
    EXPECT_EQ(system.getDBusInterface().getStateProperty(), 1);
    EXPECT_EQ(system.getDBusInterface().getPgoodProperty(), 1);
    EXPECT_EQ(system.getDBusInterface().getPgoodTimeoutProperty(),
              PGOOD_TIMEOUT);

    // Power off system
    system.setPowerState(PowerState::off, services);
    EXPECT_EQ(system.getPowerState(), PowerState::off);
    EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    EXPECT_EQ(system.getDBusInterface().getStateProperty(), 0);
    EXPECT_EQ(system.getDBusInterface().getPgoodProperty(), 1);

    // Monitor. Power good off.
    system.monitor(services);
    EXPECT_EQ(system.getPowerState(), PowerState::off);
    EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    EXPECT_EQ(system.getDBusInterface().getStateProperty(), 0);
    EXPECT_EQ(system.getDBusInterface().getPgoodProperty(), 0);
}

TEST(SystemTests, CreateDBusInterfaceIfPossible)
{
    // The test variations are covered by SetPowerGoodValue
}

TEST(SystemTests, VerifyMonitoringInitialized)
{
    // Tested by Monitor
}

TEST(SystemTests, verifyCanSetPowerState)
{
    // Tested by SetPowerState
}

TEST(SystemTests, GetChassisForNewPowerState)
{
    // Tested by SetPowerState
}

TEST(SystemTests, SetChassisPowerState)
{
    // Tested by SetPowerState
}

TEST(SystemTests, MonitorChassisStatus)
{
    // Tested by Monitor
}

TEST(SystemTests, SetInitialSelectedChassisIfNeeded)
{
    // Does not set: Already set by setPowerState()
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering off system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering off chassis 2")).Times(1);

        system.setPowerState(PowerState::off, services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
        EXPECT_TRUE(system.getSelectedChassis().contains(2));

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
    }

    // All chassis have good status
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
    }

    // Some chassis have good status: One is not present
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
    }

    // No chassis have good status: One is not available: One does not have good
    // input power
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsAvailable(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).Times(0);
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            setChassisStatusToGoodExceptIsInputPowerGood(system, 1);
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isInputPowerGood)
                .WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_TRUE(system.getSelectedChassis().empty());
    }

    // First chassis throws exception when finding status: Verify still checks
    // second chassis
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent)
                .WillRepeatedly(Throw(std::runtime_error{
                    "Present property value could not be obtained."}));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).Times(0);
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to monitor chassis 1: Present property value could not be obtained."));

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
    }

    // All chassis off
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
    }

    // All chassis on
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
    }

    // One chassis on and one off
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
    }
}

TEST(SystemTests, SetPowerGood)
{
    // Does not set: No chassis selected
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            setChassisStatusToGoodExceptIsPresent(system, 1);
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_TRUE(system.getSelectedChassis().empty());
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    }

    // All chassis are selected
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    }

    // Some chassis are selected
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }

    // First selected chassis throws an exception getting power good: Verify
    // continues to second chassis
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);

        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    }

    // All selected chassis are on
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }

    // All selected chassis are off
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    }

    // Selected chassis have different power good values. In state transition.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 2"))
            .Times(0);

        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.isInPowerStateTransition());

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.isInPowerStateTransition());
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    }

    // Selected chassis have different power good values. Not in state
    // transition.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);

        // Initial monitor. Both chassis power goods are on. System power good
        // is set to on. Not in transition.
        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_FALSE(system.isInPowerStateTransition());
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);

        // Second monitor. Power good changes to off in chassis 1. Power good
        // fault. System power good changes to off since not in transition and
        // at least one chassis is off.
        system.monitor(services);
        EXPECT_FALSE(system.isInPowerStateTransition());
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    }
}

TEST(SystemTests, ShouldUseChassisPowerGood)
{
    // Test where not used. Chassis not in selected set.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(false))       // Initial monitor
                .WillRepeatedly(Return(true)); // After power on
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            setChassisStatusToGoodExceptIsPresent(system, 1);
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).Times(0);
            EXPECT_CALL(device, powerOn).Times(0);
            EXPECT_CALL(device, close).Times(2);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg(
                "Unable to set chassis 2 to state on: Chassis is not present"))
            .Times(1);

        // Initial monitor. Only chassis 1 is selected. System is off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));

        // Power on system. Only chassis 1 is powered on.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);

        // Monitor. Chassis 1 is on. Chassis 2 is off (due to being not
        // present), but it not selected so its power good is not used. System
        // power good should be on.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
    }

    // Test where not used. Power is on and not in state transition. Chassis is
    // not present.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent)
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::updatePowerGood again
                .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
                .WillOnce(Return(true)) // Chassis::checkForInvalidStatus
                .WillOnce(
                    Return(true)) // System::setInitialSelectedChassisIfNeeded
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::updatePowerGood again
                .WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
        }
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Chassis 1 requested power state is on, but chassis is not present"))
            .Times(1);

        // Initial monitor. Both chassis present with power on. Both selected.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);

        // Monitor. Chassis 1 no longer present. System power good should remain
        // on because chassis 1 power good should not be used.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }

    // Test where not used. Power is on and not in state transition. Chassis is
    // not available.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsAvailable(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isAvailable)
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
                .WillOnce(Return(true)) // Chassis::checkForInvalidStatus
                .WillOnce(
                    Return(true)) // System::setInitialSelectedChassisIfNeeded
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Chassis 1 requested power state is on, but chassis is not available"))
            .Times(1);

        // Initial monitor. Both chassis present with power on. Both selected.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);

        // Monitor. Chassis 1 now not available and power good is false. System
        // power good should remain on because chassis 1 power good should not
        // be used.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }

    // Test where not used. Power is on and not in state transition. Chassis
    // does not have good input power.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsInputPowerGood(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isInputPowerGood)
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::updatePowerGood again
                .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
                .WillOnce(Return(true)) // Chassis::checkForInvalidStatus
                .WillOnce(
                    Return(true)) // System::setInitialSelectedChassisIfNeeded
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::updatePowerGood again
                .WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Chassis 1 requested power state is on, but chassis does not have input power"))
            .Times(1);

        // Initial monitor. Both chassis have good input power and are powered
        // on. Both selected.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);

        // Monitor. Chassis 1 no longer has good input power. System power good
        // should remain on because chassis 1 power good should not be used.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }

    // Test where not used. Exception thrown getting chassis status.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsAvailable(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isAvailable)
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
                .WillOnce(Return(true)) // Chassis::checkForInvalidStatus
                .WillOnce(
                    Return(true)) // System::setInitialSelectedChassisIfNeeded
                .WillOnce(Return(true))  // Chassis::updatePowerGood
                .WillOnce(Return(false)) // Chassis::checkForPowerGoodError
                .WillOnce(Return(true))  // Chassis::checkForInvalidStatus
                .WillOnce(Throw(std::runtime_error{
                    "Available property value could not be obtained."}));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Return(false));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        // Initial monitor. Both chassis available with power on. Both selected.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);

        // Monitor. Chassis 1 throws exception getting available property and
        // power good is false. System power good should remain on because
        // chassis 1 power good should not be used.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }

    // Test where used. Power state is not defined.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGoodExceptIsAvailable(system, 0);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isAvailable)
            .WillOnce(Return(true)) // Chassis::updatePowerGood
            .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
            .WillOnce(Return(true)) // Chassis::checkForInvalidStatus
            .WillOnce(Return(true)) // System::setInitialSelectedChassisIfNeeded
            .WillRepeatedly(Return(false));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        // Initial monitor. Chassis is powered on. Chassis is unavailable at the
        // point where shouldUseChassisPowerGood() is called. Should not matter
        // because the system power state is not yet defined.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
    }

    // Test where used. Power state is off.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGoodExceptIsAvailable(system, 0);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isAvailable)
            .WillOnce(Return(true)) // Chassis::canSetPowerState
            .WillOnce(Return(true)) // Chassis::canSetPowerState again
            .WillOnce(Return(true)) // Chassis::updatePowerGood
            .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
            .WillRepeatedly(Return(false));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOff).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        EXPECT_CALL(services, logInfoMsg("Powering off system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        // Power off system. This will set chassis and system power state to
        // off. System and chassis power good are undefined.
        system.setPowerState(PowerState::off, services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);

        // Monitor. Chassis 1 is unavailable at the point where
        // shouldUseChassisPowerGood() is called. Should not matter because the
        // system power state is off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    }

    // Test where used. Power state is on. In transition.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGoodExceptIsAvailable(system, 0);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isAvailable)
            .WillOnce(Return(true)) // Chassis::canSetPowerState
            .WillOnce(Return(true)) // Chassis::canSetPowerState again
            .WillOnce(Return(true)) // Chassis::updatePowerGood
            .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
            .WillOnce(Return(true)) // Chassis::checkForInvalidStatus
            .WillRepeatedly(Return(false));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is on and power good is off"))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerOff).Times(1);

        // Power on system. This will set chassis and system power state to
        // on. System and chassis power good are undefined.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor. Chassis 1 is unavailable at the point where
        // shouldUseChassisPowerGood() is called. Should not matter because the
        // system power state is in transition. System will request dump and
        // power off due to chassis being in bad status.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.isInPowerStateTransition());
    }

    // Test where used. Chassis status is good.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(false))
            .WillOnce(Return(false))
            .WillOnce(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        // Initial power state is off
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Power on
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor. Chassis is still off. System in transition.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Chassis power good changes to on
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_FALSE(system.isInPowerStateTransition());
    }
}

TEST(SystemTests, SetInitialPowerStateIfNeeded)
{
    // Does not set: Already set
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is on and power good is off"))
            .Times(1);

        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);

        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
    }

    // Does not set: Power good does not have value
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));

        system.monitor(services);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
        EXPECT_THROW(system.getPowerState(), std::runtime_error);
    }

    // Sets to on
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
    }

    // Sets to off
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);

        system.monitor(services);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
    }
}

TEST(SystemTests, UpdateInPowerStateTransition)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    System system{std::move(chassis)};
    MockServices services;

    system.initializeMonitoring(services);
    setChassisStatusToGood(system, 0);

    auto& device = getMockDevice(system, 0);
    EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
    EXPECT_CALL(device, getPowerGood)
        .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}))
        .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}))
        .WillOnce(Return(false))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(device, powerOn).Times(1);
    EXPECT_CALL(device, powerOff).Times(1);

    EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
    EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
    EXPECT_CALL(services,
                logInfoMsg("Chassis 1 power state is on and power good is off"))
        .Times(1);
    EXPECT_CALL(services,
                logInfoMsg("System power state is on and power good is off"))
        .Times(1);
    EXPECT_CALL(services, logInfoMsg("Powering off system")).Times(1);
    EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

    // Test where not updated: Not in transition and powerState is not defined
    system.monitor(services);
    EXPECT_THROW(system.getPowerState(), std::runtime_error);
    EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    EXPECT_FALSE(system.isInPowerStateTransition());

    // Power on system
    system.setPowerState(PowerState::on, services);
    EXPECT_EQ(system.getPowerState(), PowerState::on);
    EXPECT_EQ(system.getSelectedChassis().size(), 1);
    EXPECT_TRUE(system.isInPowerStateTransition());

    // Test where not updated: powerGood is not defined
    system.monitor(services);
    EXPECT_EQ(system.getPowerState(), PowerState::on);
    EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    EXPECT_TRUE(system.isInPowerStateTransition());

    // Test where not updated: powerState is on and powerGood is off
    system.monitor(services);
    EXPECT_EQ(system.getPowerState(), PowerState::on);
    EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    EXPECT_TRUE(system.isInPowerStateTransition());

    // Test where updated: powerState and powerGood are both on
    system.monitor(services);
    EXPECT_EQ(system.getPowerState(), PowerState::on);
    EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    EXPECT_FALSE(system.isInPowerStateTransition());

    // Power off system
    system.setPowerState(PowerState::off, services);
    EXPECT_EQ(system.getPowerState(), PowerState::off);
    EXPECT_EQ(system.getSelectedChassis().size(), 1);
    EXPECT_TRUE(system.isInPowerStateTransition());

    // Test where not updated: powerState is off and powerGood is on
    system.monitor(services);
    EXPECT_EQ(system.getPowerState(), PowerState::off);
    EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    EXPECT_TRUE(system.isInPowerStateTransition());

    // Test where updated: powerState and powerGood are both off
    system.monitor(services);
    EXPECT_EQ(system.getPowerState(), PowerState::off);
    EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    EXPECT_FALSE(system.isInPowerStateTransition());
}

TEST(SystemTests, CheckForPowerGoodFaults)
{
    // Test where none detected: System power state is not defined
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));

        EXPECT_CALL(services, createBMCDump).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        // Monitor. Chassis throws exception trying to read the GPIO
        system.monitor(services);
        EXPECT_THROW(system.getPowerState(), std::runtime_error);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
        EXPECT_TRUE(system.getSelectedChassis().empty());
    }

    // Test where none detected: System power state is off
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));
        EXPECT_CALL(device, powerOff).Times(1);

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.Shutdown",
                             Entry::Level::Critical, _))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerOff).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering off system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        system.getChassis()[0]->setPowerGoodFaultLogDelay(
            std::chrono::milliseconds(0));

        // Monitor. System and chassis are powered on.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);

        // Monitor again. Chassis has a power good fault. Fault not logged yet.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasLogged);

        // Monitor again. Power good fault logged. System requests dump and
        // power off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(system.getChassis()[0]->getPowerGoodFault().wasLogged);

        // Power off system.
        system.setPowerState(PowerState::off, services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);

        // Clear error history so System could request dump/poweroff again
        system.clearErrorHistory();

        // Monitor several times. Verify system is off. Verify chassis still has
        // power good fault. Verify System does not request dump or power off
        // again.
        system.monitor(services);
        system.monitor(services);
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(system.getChassis()[0]->getPowerGoodFault().wasLogged);
    }

    // Test where none detected: Chassis with power good fault not selected
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsAvailable(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isAvailable)
                .WillOnce(Return(false)) // Chassis::updatePowerGood
                .WillOnce(Return(false)) // Chassis::checkForPowerGoodError
                .WillOnce(
                    Return(false)) // System::setInitialSelectedChassisIfNeeded
                .WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.Shutdown",
                             Entry::Level::Critical, _))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        system.getChassis()[0]->setPowerGoodFaultLogDelay(
            std::chrono::milliseconds(0));

        // Monitor. First chassis is not available, so it is not selected.
        // Second chassis is on, so system is on.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));

        // Monitor again. Chassis 1 is now available and on.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);

        // Monitor again. Chassis 1 has a power good fault. Fault not logged
        // yet. System power is still on since chassis 1 not in selected set.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasLogged);

        // Monitor again. Power good fault logged. System does not request dump
        // or power off since chassis 1 is not in the selected set.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(system.getChassis()[0]->getPowerGoodFault().wasLogged);
    }

    // Test where none detected: No chassis have power good fault
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg).Times(0);
        EXPECT_CALL(services, logError).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);
        EXPECT_CALL(services, createBMCDump).Times(0);

        // Monitor. System is on. No power good fault.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_FALSE(system.getChassis()[0]->hasPowerGoodFault());

        // Monitor again
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_FALSE(system.getChassis()[0]->hasPowerGoodFault());
    }

    // Test where no action taken: Chassis has timeout power good fault
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services,
                    logErrorMsg("Power on failed in chassis 1: Timeout"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.PowerOnTimeout",
                             Entry::Level::Critical, _))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        system.getChassis()[0]->setPowerGoodTimeout(
            std::chrono::milliseconds(0));

        // Monitor. System and chassis are powered off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);

        // Power on system
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);

        // Monitor. A timeout power good fault should be logged.
        // Verify System does not request dump or power off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_TRUE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(system.getChassis()[0]->getPowerGoodFault().wasLogged);
    }

    // Test where no action taken: Power good fault has not been logged
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        // Monitor. System and chassis are powered on.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);

        // Monitor again. Chassis 1 has a power good fault. Fault not logged
        // yet. System power is now off. Verify System does not request dump or
        // power off since fault not logged yet.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasLogged);
    }

    // Test where fault detected in first chassis. In power state transition.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(false))
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.Shutdown",
                             Entry::Level::Critical, _))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerCycle).Times(1);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        system.getChassis()[0]->setPowerGoodFaultLogDelay(
            std::chrono::milliseconds(0));

        // Monitor. Both chassis and system are off.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);

        // Power on system
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Chassis 1 is now on. Chassis 2 is still off. System
        // power good is still off. System still in transition.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Chassis 1 has power good fault. Fault not logged yet.
        // System still in transition.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasLogged);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Power good fault logged. System requests dump and
        // power cycle.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(system.getChassis()[0]->getPowerGoodFault().wasLogged);
        EXPECT_TRUE(system.isInPowerStateTransition());
    }

    // Test where fault detected in second chassis. After system powered on (not
    // in transition).
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(false))
                .WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(false))
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 2"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.Shutdown",
                             Entry::Level::Critical, _))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerCycle).Times(1);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        system.getChassis()[1]->setPowerGoodFaultLogDelay(
            std::chrono::milliseconds(0));

        // Monitor. Both chassis and system are off.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);

        // Power on system
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Both chassis are on, so the system power good changes
        // to on. System no longer in transition.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor again. Chassis 2 has power good fault. Fault not logged yet.
        // System power good changes to off.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[1]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[1]->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(system.getChassis()[1]->getPowerGoodFault().wasLogged);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor again. Power good fault logged. System requests dump and
        // power cycle.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[1]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[1]->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(system.getChassis()[1]->getPowerGoodFault().wasLogged);
        EXPECT_FALSE(system.isInPowerStateTransition());
    }
}

TEST(SystemTests, CheckForInvalidChassisStatus)
{
    // Test where bad chassis status ignored: System power state not defined
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGoodExceptIsPresent(system, 0);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).Times(0);
        EXPECT_CALL(device, close).Times(1);

        EXPECT_CALL(services, createBMCDump).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);

        // Monitor. Chassis is not present, but no action taken since system
        // power state is not defined.
        system.monitor(services);
        EXPECT_THROW(system.getPowerState(), std::runtime_error);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
        EXPECT_TRUE(system.getSelectedChassis().empty());
    }

    // Test where bad chassis status ignored: System power state is off
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGoodExceptIsPresent(system, 0);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent)
            .WillOnce(Return(true)) // Chassis::updatePowerGood
            .WillOnce(Return(true)) // Chassis::updatePowerGood again
            .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
            .WillOnce(Return(true)) // System::setInitialSelectedChassisIfNeeded
            .WillRepeatedly(Return(false));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        EXPECT_CALL(device, close).Times(1);

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        // Monitor. Chassis is present. Chassis and system power are off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);

        // Monitor. Chassis is not present, but no action taken since system
        // power state is not defined.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
    }

    // Test where bad chassis status ignored: System not in state transition
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGoodExceptIsPresent(system, 0);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent)
            .WillOnce(Return(true)) // Chassis::updatePowerGood
            .WillOnce(Return(true)) // Chassis::updatePowerGood again
            .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
            .WillOnce(Return(true)) // Chassis::checkForInvalidStatus
            .WillOnce(Return(true)) // System::setInitialSelectedChassisIfNeeded
            .WillRepeatedly(Return(false));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        EXPECT_CALL(device, close).Times(1);

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("System power state is on and power good is on"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Chassis 1 requested power state is on, but chassis is not present"))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        // Monitor. Chassis is present. Chassis and system power are on. Not in
        // state transition.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor. Chassis is not present, but no action taken since system
        // is not in state transition.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_FALSE(system.isInPowerStateTransition());
    }

    // Test where bad chassis status ignored: Chassis was not selected
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            setChassisStatusToGoodExceptIsAvailable(system, 1);
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).Times(0);
            EXPECT_CALL(device, powerOn).Times(0);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg(
                "Unable to set chassis 2 to state on: Chassis is not available"));
        EXPECT_CALL(services, createBMCDump).Times(0);
        EXPECT_CALL(services, hardPowerOff).Times(0);

        // Monitor. First chassis is off, so system is off. Second chassis is
        // not available, so it is not selected.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Power on the system. System is now in transition. Second chassis is
        // not powered on since it is not selected.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Chassis 1 is still off, so System power good is still
        // off. System is still in transition. Chassis 2 is in a bad state, but
        // no action is taken since chassis is not selected.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.isInPowerStateTransition());
    }

    // Test where bad chassis status detected: Not present. Chassis 1.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsPresent(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent)
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::updatePowerGood again
                .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
                .WillOnce(
                    Return(true)) // System::setInitialSelectedChassisIfNeeded
                .WillOnce(Return(true)) // Chassis::canSetPowerState
                .WillOnce(Return(true)) // Chassis::canSetPowerState again
                .WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, close).Times(1);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Chassis 1 requested power state is on, but chassis is not present"))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerCycle).Times(1);

        // Monitor. Both chassis are present and powered off. Both should be
        // selected. System is powered off and not in transition.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Power on the system. System is now in transition.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Chassis 2 is still off, so System power good is still
        // off. System is still in transition. Chassis 1 is now not present, so
        // System should request dump and power cycle.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.isInPowerStateTransition());
    }

    // Test where bad chassis status detected: Not available. Chassis 2.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            setChassisStatusToGoodExceptIsAvailable(system, 1);
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isAvailable)
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
                .WillOnce(
                    Return(true)) // System::setInitialSelectedChassisIfNeeded
                .WillOnce(Return(true)) // Chassis::canSetPowerState
                .WillOnce(Return(true)) // Chassis::canSetPowerState again
                .WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Chassis 2 requested power state is on, but chassis is not available"))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerCycle).Times(1);

        // Monitor. Both chassis are available and powered off. Both should be
        // selected. System is powered off and not in transition.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Power on the system. System is now in transition.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Chassis 1 is still off, so System power good is still
        // off. System is still in transition. Chassis 2 is now not available,
        // so System should request dump and power cycle.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.isInPowerStateTransition());
    }

    // Test where bad chassis status detected: No input power. Chassis 1.
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGoodExceptIsInputPowerGood(system, 0);
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isInputPowerGood)
                .WillOnce(Return(true)) // Chassis::updatePowerGood
                .WillOnce(Return(true)) // Chassis::updatePowerGood again
                .WillOnce(Return(true)) // Chassis::checkForPowerGoodError
                .WillOnce(
                    Return(true)) // System::setInitialSelectedChassisIfNeeded
                .WillOnce(Return(true)) // Chassis::canSetPowerState
                .WillOnce(Return(true)) // Chassis::canSetPowerState again
                .WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, close).Times(1);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Chassis 1 requested power state is on, but chassis does not have input power"))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerCycle).Times(1);

        // Monitor. Both chassis have good input power and powered off. Both
        // should be selected. System is powered off and not in transition.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Power on the system. System is now in transition.
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Chassis 2 is still off, so System power good is still
        // off. System is still in transition. Chassis 1 now has no input power,
        // so System should request dump and power cycle.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.isInPowerStateTransition());
    }
}

TEST(SystemTests, CreateBMCDump)
{
    // Tested by SetPowerState
}

TEST(SystemTests, HardPowerOff)
{
    // Verify power off performed for a one chassis system. Verify only
    // performed once until error history is cleared
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        setChassisStatusToGood(system, 0);

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(false))
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.Shutdown",
                             Entry::Level::Critical, _))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(2);
        EXPECT_CALL(services, hardPowerOff).Times(2);
        EXPECT_CALL(services, hardPowerCycle).Times(0);

        system.getChassis()[0]->setPowerGoodFaultLogDelay(
            std::chrono::milliseconds(0));

        // Monitor. Chassis and system are off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);

        // Power on system
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Chassis is on, so the system power good changes
        // to on. System no longer in transition.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor again. Chassis has power good fault. Fault not logged yet.
        // System power good changes to off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasLogged);

        // Monitor again. Power good fault logged. System requests power off.
        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[0]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[0]->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(system.getChassis()[0]->getPowerGoodFault().wasLogged);

        // Monitor several two times. Should not request power off again
        system.monitor(services);
        system.monitor(services);

        // Clear error history and monitor again. Should request power off
        // again.
        system.clearErrorHistory();
        system.monitor(services);
    }

    // Verify power cycle performed for a multi-chassis system. Test where
    // exception is thrown by hardPowerCycle().
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        chassis.emplace_back(
            createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        {
            setChassisStatusToGood(system, 0);

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(false))
                .WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            setChassisStatusToGood(system, 1);

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(false))
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));
        }

        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 1 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("Chassis 2 power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg("System power state is off and power good is off"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on system")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logInfoMsg("Powering on chassis 2")).Times(1);
        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 2"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.Shutdown",
                             Entry::Level::Critical, _))
            .Times(1);
        EXPECT_CALL(services, createBMCDump).Times(1);
        EXPECT_CALL(services, hardPowerOff).Times(0);
        EXPECT_CALL(services, hardPowerCycle)
            .WillOnce(Throw(std::runtime_error{"Target not found"}));
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to power cycle system using systemd: Target not found"))
            .Times(1);

        system.getChassis()[1]->setPowerGoodFaultLogDelay(
            std::chrono::milliseconds(0));

        // Monitor. Both chassis and system are off.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);

        // Power on system
        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_TRUE(system.isInPowerStateTransition());

        // Monitor again. Both chassis are on, so the system power good changes
        // to on. System no longer in transition.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_FALSE(system.isInPowerStateTransition());

        // Monitor again. Chassis 2 has power good fault. Fault not logged yet.
        // System power good changes to off.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[1]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[1]->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(system.getChassis()[1]->getPowerGoodFault().wasLogged);

        // Monitor again. Power good fault logged. System requests power cycle.
        system.monitor(services);
        EXPECT_EQ(system.getChassis()[0]->getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getChassis()[1]->getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_TRUE(system.getChassis()[1]->hasPowerGoodFault());
        EXPECT_FALSE(system.getChassis()[1]->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(system.getChassis()[1]->getPowerGoodFault().wasLogged);
    }
}
