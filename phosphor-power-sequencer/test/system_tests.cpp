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
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

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
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillOnce(Return(false));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn)
                .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
            EXPECT_CALL(device, getName).WillOnce(ReturnRef(deviceName));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to set chassis 1 to state on: Unable to power on device pseq0 in chassis 1: Unable to write GPIO"));

        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(false));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).Times(0);
            EXPECT_CALL(device, getPowerGood).Times(0);
            EXPECT_CALL(device, powerOff).Times(0);
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
        }
        EXPECT_CALL(
            services,
            logInfoMsg(
                "Unable to set chassis 1 to state off: Chassis is not available"));

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
        EXPECT_EQ(system.getPowerState(), PowerState::on);

        system.setPowerState(PowerState::off, services);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
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
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(false));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).Times(0);
        EXPECT_CALL(device, getPowerGood).Times(0);
    }
    {
        auto& monitor = getMockStatusMonitor(system, 1);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 1);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
    }
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
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

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
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        system.monitor(services);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }
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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent)
                .WillRepeatedly(Throw(std::runtime_error{
                    "Present property value could not be obtained."}));
        }
        {
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

    // Test where works: Can only monitor some chassis: First chassis throws
    // exception: Verify continues to second chassis
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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent)
                .WillRepeatedly(Throw(std::runtime_error{
                    "Present property value could not be obtained."}));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to monitor chassis 1: Present property value could not be obtained."));

        system.monitor(services);
        EXPECT_EQ(system.getPowerState(), PowerState::on);
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
    }
}

TEST(SystemTests, Monitor_SetInitialSelectedChassisIfNeeded)
{
    // The following are tests for the setInitialSelectedChassisIfNeeded()
    // private method. Since that method cannot be called directly in a test
    // case, it is called indirectly using the public method monitor().

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(false));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).Times(0);
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood)
                .WillRepeatedly(Return(false));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent)
                .WillRepeatedly(Throw(std::runtime_error{
                    "Present property value could not be obtained."}));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).Times(0);
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(1));
    }
}

TEST(SystemTests, Monitor_SetPowerGood)
{
    // The following are tests for the setPowerGood() private method. Since that
    // method cannot be called directly in a test case, it is called indirectly
    // using the public method monitor().

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 1);
        EXPECT_TRUE(system.getSelectedChassis().contains(2));
        EXPECT_EQ(system.getPowerGood(), PowerGood::on);
    }

    // First selected chassis throws an exception reading power good: Verify
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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
    }

    // Selected chassis have different power good values
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
            auto& monitor = getMockStatusMonitor(system, 0);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            auto& monitor = getMockStatusMonitor(system, 1);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            auto& device = getMockDevice(system, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        system.setPowerState(PowerState::on, services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);

        system.monitor(services);
        EXPECT_EQ(system.getSelectedChassis().size(), 2);
        EXPECT_THROW(system.getPowerGood(), std::runtime_error);
    }
}

TEST(SystemTests, Monitor_SetInitialPowerStateIfNeeded)
{
    // The following are tests for the setInitialPowerStateIfNeeded() private
    // method. Since that method cannot be called directly in a test case, it is
    // called indirectly using the public method monitor().

    // Does not set: Already set
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(
            createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
        System system{std::move(chassis)};
        MockServices services;

        system.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

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
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

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
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

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
        auto& monitor = getMockStatusMonitor(system, 0);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(system, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        system.monitor(services);
        EXPECT_EQ(system.getPowerGood(), PowerGood::off);
        EXPECT_EQ(system.getPowerState(), PowerState::off);
    }
}

TEST(SystemTests, SetPowerGoodTimeOut)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(
        createChassis(1, "/xyz/openbmc_project/inventory/system/chassis1"));
    chassis.emplace_back(
        createChassis(2, "/xyz/openbmc_project/inventory/system/chassis2"));
    System system{std::move(chassis)};

    // Verify the default chassis timeout values, which are set by a meson
    // option expressed in seconds. Chassis::getPowerGoodTimeOut() returns
    // duration expressed in milliseconds. std::chrono classes are able to
    // handle conversion.
    std::chrono::seconds defaultValueInSecs{PGOOD_TIMEOUT};
    for (auto& chassis : system.getChassis())
    {
        EXPECT_EQ(chassis->getPowerGoodTimeOut(), defaultValueInSecs);
        EXPECT_EQ(chassis->getPowerGoodTimeOut().count(),
                  (PGOOD_TIMEOUT * 1000));
    }

    // Set to new value in seconds
    std::chrono::seconds newValueInSecs{5};
    system.setPowerGoodTimeOut(newValueInSecs);
    for (auto& chassis : system.getChassis())
    {
        EXPECT_EQ(chassis->getPowerGoodTimeOut(), newValueInSecs);
        EXPECT_EQ(chassis->getPowerGoodTimeOut().count(), 5000);
    }

    // Set to new value in milliseconds
    std::chrono::milliseconds newValueInMillis{400};
    system.setPowerGoodTimeOut(newValueInMillis);
    for (auto& chassis : system.getChassis())
    {
        EXPECT_EQ(chassis->getPowerGoodTimeOut(), newValueInMillis);
        EXPECT_EQ(chassis->getPowerGoodTimeOut().count(), 400);
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
