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
#include "power_sequencer_device.hpp"
#include "rail.hpp"
#include "ucd90160_device.hpp"

#include <stddef.h> // for size_t

#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgReferee;
using ::testing::Throw;

/**
 * Creates a real PowerSequencerDevice instance.
 *
 * PowerSequencerDevice is an abstract base class. The actual object type
 * created is a UCD90160Device.
 *
 * @param bus I2C bus for the device
 * @param address I2C address for the device
 * @return real PowerSequencerDevice
 */
std::unique_ptr<PowerSequencerDevice> createRealPowerSequencer(uint8_t bus,
                                                               uint16_t address)
{
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails;
    return std::make_unique<UCD90160Device>(
        bus, address, powerControlGPIOName, powerGoodGPIOName,
        std::move(rails));
}

/**
 * Creates a mock PowerSequencerDevice instance.
 *
 * PowerSequencerDevice is an abstract base class. The actual object type
 * created is a MockDevice.
 *
 * @return mock PowerSequencerDevice
 */
std::unique_ptr<PowerSequencerDevice> createMockPowerSequencer()
{
    return std::make_unique<MockDevice>();
}

/**
 * Creates a Chassis with the specified number of mock PowerSequencerDevice
 * instances.
 *
 * @param powerSequencerCount Number of mock power sequencers to add to chassis
 * @return chassis that was created
 */
std::unique_ptr<Chassis> createChassis(size_t powerSequencerCount)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    for (size_t i = 0; i < powerSequencerCount; ++i)
    {
        powerSequencers.emplace_back(createMockPowerSequencer());
    }
    ChassisStatusMonitorOptions monitorOptions;
    return std::make_unique<Chassis>(
        number, inventoryPath, std::move(powerSequencers), monitorOptions);
}

/**
 * Returns the MockChassisStatusMonitor within a Chassis.
 *
 * Assumes that initializeMonitoring() has been called with a MockServices
 * parameter.
 *
 * Throws an exception if initializeMonitoring() has not been called.
 *
 * @param chassis Chassis object
 * @return MockChassisStatusMonitor reference
 */
MockChassisStatusMonitor& getMockStatusMonitor(Chassis& chassis)
{
    return static_cast<MockChassisStatusMonitor&>(chassis.getStatusMonitor());
}

/**
 * Returns a reference to the MockDevice at the specified index within the
 * chassis's vector of power sequencer devices.
 *
 * @param chassis Chassis object
 * @param i Index of power sequencer device
 * @return MockDevice reference
 */
MockDevice& getMockDevice(Chassis& chassis, int i)
{
    return static_cast<MockDevice&>(*(chassis.getPowerSequencers()[i]));
}

/**
 * Sets up the MockChassisStatusMonitor to repeatedly return good status for all
 * properties.
 *
 * @param chassis Chassis object
 */
void setChassisStatusToGood(Chassis& chassis)
{
    auto& monitor = getMockStatusMonitor(chassis);
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
}

TEST(ChassisTests, PowerGoodFault)
{
    PowerGoodFault fault{};
    EXPECT_FALSE(fault.wasTimeout);
    EXPECT_FALSE(fault.wasLogged);
}

TEST(ChassisTests, Constructor)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    powerSequencers.emplace_back(createRealPowerSequencer(3, 0x70));
    ChassisStatusMonitorOptions monitorOptions;
    monitorOptions.isPresentMonitored = true;
    monitorOptions.isAvailableMonitored = false;
    monitorOptions.isEnabledMonitored = true;
    monitorOptions.isPowerStateMonitored = true; // Invalid; ctor sets to false
    monitorOptions.isPowerGoodMonitored = true;  // Invalid; ctor sets to false
    monitorOptions.isInputPowerStatusMonitored = false;
    monitorOptions.isPowerSuppliesStatusMonitored = true;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};

    EXPECT_EQ(chassis.getNumber(), number);
    EXPECT_EQ(chassis.getInventoryPath(), inventoryPath);
    EXPECT_EQ(chassis.getPowerSequencers().size(), 1);
    EXPECT_EQ(chassis.getPowerSequencers()[0]->getBus(), 3);
    EXPECT_EQ(chassis.getPowerSequencers()[0]->getAddress(), 0x70);
    EXPECT_TRUE(chassis.getMonitorOptions().isPresentMonitored);
    EXPECT_FALSE(chassis.getMonitorOptions().isAvailableMonitored);
    EXPECT_TRUE(chassis.getMonitorOptions().isEnabledMonitored);
    EXPECT_FALSE(chassis.getMonitorOptions().isPowerStateMonitored);
    EXPECT_FALSE(chassis.getMonitorOptions().isPowerGoodMonitored);
    EXPECT_FALSE(chassis.getMonitorOptions().isInputPowerStatusMonitored);
    EXPECT_TRUE(chassis.getMonitorOptions().isPowerSuppliesStatusMonitored);
    EXPECT_THROW(chassis.getStatusMonitor(), std::runtime_error);
    EXPECT_THROW(chassis.getPowerState(), std::runtime_error);
    EXPECT_THROW(chassis.getPowerGood(), std::runtime_error);
    EXPECT_FALSE(chassis.isInPowerStateTransition());
    EXPECT_FALSE(chassis.hasPowerGoodFault());
    EXPECT_THROW(chassis.getPowerGoodFault(), std::runtime_error);
    EXPECT_EQ(chassis.getPowerGoodTimeout(),
              std::chrono::seconds{PGOOD_TIMEOUT});
    EXPECT_EQ(chassis.getPowerGoodFaultLogDelay(), std::chrono::seconds{7});
    EXPECT_TRUE(chassis.getPowerSupplyError().empty());
}

TEST(ChassisTests, GetNumber)
{
    size_t number{2};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis2"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};

    EXPECT_EQ(chassis.getNumber(), number);
}

TEST(ChassisTests, GetInventoryPath)
{
    size_t number{3};
    std::string inventoryPath{
        "/xyz/openbmc_project/inventory/system/chassis_3"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};

    EXPECT_EQ(chassis.getInventoryPath(), inventoryPath);
}

TEST(ChassisTests, GetPowerSequencers)
{
    size_t number{2};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis2"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    powerSequencers.emplace_back(createRealPowerSequencer(3, 0x70));
    powerSequencers.emplace_back(createRealPowerSequencer(4, 0x32));
    powerSequencers.emplace_back(createRealPowerSequencer(10, 0x16));
    ChassisStatusMonitorOptions monitorOptions;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};

    EXPECT_EQ(chassis.getPowerSequencers().size(), 3);
    EXPECT_EQ(chassis.getPowerSequencers()[0]->getBus(), 3);
    EXPECT_EQ(chassis.getPowerSequencers()[0]->getAddress(), 0x70);
    EXPECT_EQ(chassis.getPowerSequencers()[1]->getBus(), 4);
    EXPECT_EQ(chassis.getPowerSequencers()[1]->getAddress(), 0x32);
    EXPECT_EQ(chassis.getPowerSequencers()[2]->getBus(), 10);
    EXPECT_EQ(chassis.getPowerSequencers()[2]->getAddress(), 0x16);
}

TEST(ChassisTests, GetMonitorOptions)
{
    size_t number{3};
    std::string inventoryPath{
        "/xyz/openbmc_project/inventory/system/chassis_3"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    monitorOptions.isPresentMonitored = false;
    monitorOptions.isAvailableMonitored = true;
    monitorOptions.isEnabledMonitored = false;
    monitorOptions.isInputPowerStatusMonitored = true;
    monitorOptions.isPowerSuppliesStatusMonitored = false;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};

    EXPECT_FALSE(chassis.getMonitorOptions().isPresentMonitored);
    EXPECT_TRUE(chassis.getMonitorOptions().isAvailableMonitored);
    EXPECT_FALSE(chassis.getMonitorOptions().isEnabledMonitored);
    EXPECT_TRUE(chassis.getMonitorOptions().isInputPowerStatusMonitored);
    EXPECT_FALSE(chassis.getMonitorOptions().isPowerSuppliesStatusMonitored);
}

TEST(ChassisTests, InitializeMonitoring)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);
    MockServices services;

    // Test where it is called first time
    EXPECT_THROW(chassis->getStatusMonitor(), std::runtime_error);
    chassis->initializeMonitoring(services);
    ChassisStatusMonitor* monitorPtr = &(chassis->getStatusMonitor());

    // Test where it is called second time
    chassis->initializeMonitoring(services);
    EXPECT_NE(monitorPtr, &(chassis->getStatusMonitor()));
}

TEST(ChassisTests, GetStatusMonitor)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis->getStatusMonitor();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where works
    {
        chassis->initializeMonitoring(services);
        ChassisStatusMonitor& monitor = chassis->getStatusMonitor();
        MockChassisStatusMonitor& mockMonitor =
            static_cast<MockChassisStatusMonitor&>(monitor);
        EXPECT_CALL(mockMonitor, isPresent()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis->isPresent());
    }
}

TEST(ChassisTests, IsPresent)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis->isPresent();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Present property value could not be obtained."}));
        chassis->isPresent();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Present property value could not be obtained.");
    }

    // Test where works: true
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis->isPresent());
    }

    // Test where works: false
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent()).Times(1).WillOnce(Return(false));
        EXPECT_FALSE(chassis->isPresent());
    }
}

TEST(ChassisTests, IsAvailable)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis->isAvailable();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isAvailable())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Available property value could not be obtained."}));
        chassis->isAvailable();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(),
                     "Available property value could not be obtained.");
    }

    // Test where works: true
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isAvailable()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis->isAvailable());
    }

    // Test where works: false
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isAvailable()).Times(1).WillOnce(Return(false));
        EXPECT_FALSE(chassis->isAvailable());
    }
}

TEST(ChassisTests, IsEnabled)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis->isEnabled();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isEnabled())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Enabled property value could not be obtained."}));
        chassis->isEnabled();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Enabled property value could not be obtained.");
    }

    // Test where works: true
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isEnabled()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis->isEnabled());
    }

    // Test where works: false
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isEnabled()).Times(1).WillOnce(Return(false));
        EXPECT_FALSE(chassis->isEnabled());
    }
}

TEST(ChassisTests, IsInputPowerGood)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis->isInputPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isInputPowerGood())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Input power Status property value could not be obtained."}));
        chassis->isInputPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Input power Status property value could not be obtained.");
    }

    // Test where works: true
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isInputPowerGood())
            .Times(1)
            .WillOnce(Return(true));
        EXPECT_TRUE(chassis->isInputPowerGood());
    }

    // Test where works: false
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isInputPowerGood())
            .Times(1)
            .WillOnce(Return(false));
        EXPECT_FALSE(chassis->isInputPowerGood());
    }
}

TEST(ChassisTests, IsPowerSuppliesPowerGood)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis->isPowerSuppliesPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPowerSuppliesPowerGood())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Power supplies power Status property value could not be obtained."}));
        chassis->isPowerSuppliesPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Power supplies power Status property value could not be obtained.");
    }

    // Test where works: true
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPowerSuppliesPowerGood())
            .Times(1)
            .WillOnce(Return(true));
        EXPECT_TRUE(chassis->isPowerSuppliesPowerGood());
    }

    // Test where works: false
    {
        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPowerSuppliesPowerGood())
            .Times(1)
            .WillOnce(Return(false));
        EXPECT_FALSE(chassis->isPowerSuppliesPowerGood());
    }
}

TEST(ChassisTests, GetPowerState)
{
    // Test where fails: Power state not set
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);

        chassis->getPowerState();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(),
                     "Power state could not be obtained for chassis 1");
    }

    // Test where works: Value is off
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        chassis->monitor(services);

        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
    }

    // Test where works: Value is on
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        chassis->setPowerState(PowerState::on, services);

        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    }
}

TEST(ChassisTests, CanSetPowerState)
{
    // Test where fails: Monitoring not initialized
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);

        chassis->canSetPowerState(PowerState::on);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where true: All chassis status values good
    // Initial power state not set
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillOnce(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillOnce(Return(true));

        auto [canSet, reason] = chassis->canSetPowerState(PowerState::on);
        EXPECT_TRUE(canSet);
        EXPECT_TRUE(reason.empty());
    }

    // Test where true: Chassis is not enabled, but request is off
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(false));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);

        auto [canSet, reason] = chassis->canSetPowerState(PowerState::off);
        EXPECT_TRUE(canSet);
        EXPECT_TRUE(reason.empty());
    }

    // Test where false: Chassis is already at the requested state
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);

        auto [canSet, reason] = chassis->canSetPowerState(PowerState::on);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis is already at requested state");
    }

    // Test where false: Chassis is not present
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(false));

        auto [canSet, reason] = chassis->canSetPowerState(PowerState::on);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis is not present");
    }

    // Test where false: Chassis is not enabled and request is on
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(false));

        auto [canSet, reason] = chassis->canSetPowerState(PowerState::on);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis is not enabled");
    }

    // Test where false: Chassis does not have input power
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillOnce(Return(false));

        auto [canSet, reason] = chassis->canSetPowerState(PowerState::on);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis does not have input power");
    }

    // Test where false: Chassis is not available
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillOnce(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillOnce(Return(false));

        auto [canSet, reason] = chassis->canSetPowerState(PowerState::on);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis is not available");
    }

    // Test where false: Unable to determine chassis status
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent)
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Present property value could not be obtained."}));

        auto [canSet, reason] = chassis->canSetPowerState(PowerState::on);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(
            reason,
            "Error determining chassis status: Present property value could not be obtained.");
    }
}

TEST(ChassisTests, SetPowerState)
{
    // Test where fails: Monitoring not initialized
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(0);
        MockServices services;

        chassis->setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent)
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Present property value could not be obtained."}));

        chassis->setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to set chassis 1 to state on: Error determining chassis status: Present property value could not be obtained.");
    }

    // Test where fails: New power state not allowed based on chassis status
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(false));

        chassis->setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to set chassis 1 to state on: Chassis is not present");
    }

    // Test where fails: Power on
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        std::string deviceName{"pseq0"};
        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, powerOn)
            .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
        EXPECT_CALL(device, getName).WillOnce(ReturnRef(deviceName));

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        chassis->setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to power on device pseq0 in chassis 1: Unable to write GPIO");
    }

    // Test where fails: Power off
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        std::string deviceName{"pseq0"};
        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, powerOff)
            .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
        EXPECT_CALL(device, getName).WillOnce(ReturnRef(deviceName));

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        chassis->setPowerState(PowerState::off, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to power off device pseq0 in chassis 1: Unable to write GPIO");
    }

    // Test where works: Power on: Clears error history
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        EXPECT_CALL(device, powerOn).Times(1);

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        chassis->setPowerSupplyError(
            "xyz.openbmc_project.Power.PowerSupply.Error.IoutOCFault");

        // Call monitor to set initial power state to off
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_FALSE(chassis->getPowerSupplyError().empty());

        // Call setPowerState to set power state to on
        chassis->setPowerState(PowerState::on, services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_TRUE(chassis->getPowerSupplyError().empty());
    }

    // Test where works: Power off: Does not clear error history
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        EXPECT_CALL(device, powerOff).Times(1);

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        chassis->setPowerSupplyError(
            "xyz.openbmc_project.Power.PowerSupply.Error.IoutOCFault");

        // Call monitor to set initial power state to on
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_FALSE(chassis->getPowerSupplyError().empty());

        // Call setPowerState to set power state to off
        chassis->setPowerState(PowerState::off, services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_FALSE(chassis->getPowerSupplyError().empty());
    }
}

TEST(ChassisTests, GetPowerGood)
{
    // Test where fails: Power good not set
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);

        chassis->getPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(),
                     "Power good could not be obtained for chassis 1");
    }

    // Test where works: Value is off
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Test where works: Value is on
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    }
}

TEST(ChassisTests, IsInPowerStateTransition)
{
    std::unique_ptr<Chassis> chassis = createChassis(1);
    MockServices services;

    chassis->initializeMonitoring(services);
    setChassisStatusToGood(*chassis);

    auto& device = getMockDevice(*chassis, 0);
    EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
    EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
    EXPECT_CALL(device, powerOn).Times(1);

    EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

    // Test where value is false
    chassis->monitor(services);
    EXPECT_EQ(chassis->getPowerState(), PowerState::off);
    EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    EXPECT_FALSE(chassis->isInPowerStateTransition());

    // Test where value is true
    chassis->setPowerState(PowerState::on, services);
    EXPECT_TRUE(chassis->isInPowerStateTransition());
    chassis->monitor(services);
    EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    EXPECT_TRUE(chassis->isInPowerStateTransition());
}

TEST(ChassisTests, Monitor)
{
    // Test where fails: Monitoring not initialized
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(0);
        MockServices services;

        chassis->monitor(services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where works: Power good is on for first monitor: Sets initial power
    // state and power good values: Power good changes to off: Power good fault
    // detected
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);

        // Monitor: Power good is on: Sets initial power state/power good
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good changes to off: Power good fault detected
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);
    }
}

TEST(ChassisTests, HasPowerGoodFault)
{
    std::unique_ptr<Chassis> chassis = createChassis(1);
    MockServices services;

    chassis->initializeMonitoring(services);
    setChassisStatusToGood(*chassis);

    auto& device = getMockDevice(*chassis, 0);
    EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
    EXPECT_CALL(device, getPowerGood)
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
        .Times(1);

    // Test where false: Power good is true
    chassis->monitor(services);
    EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    EXPECT_FALSE(chassis->hasPowerGoodFault());

    // Test where true: Power good changed to false
    chassis->monitor(services);
    EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    EXPECT_TRUE(chassis->hasPowerGoodFault());
}

TEST(ChassisTests, GetPowerGoodFault)
{
    std::unique_ptr<Chassis> chassis = createChassis(1);
    MockServices services;

    chassis->initializeMonitoring(services);
    setChassisStatusToGood(*chassis);

    auto& device = getMockDevice(*chassis, 0);
    EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
    EXPECT_CALL(device, getPowerGood)
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
        .Times(1);

    // Test where fails: No fault detected: Power good is true
    chassis->monitor(services);
    EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    EXPECT_FALSE(chassis->hasPowerGoodFault());
    try
    {
        chassis->getPowerGoodFault();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "No power good fault detected in chassis 1");
    }

    // Test where works: Fault detected: Power good changed to false
    chassis->monitor(services);
    EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    EXPECT_TRUE(chassis->hasPowerGoodFault());
    EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
    EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);
}

TEST(ChassisTests, CloseDevices)
{
    // Test where all devices were already closed
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(false));
            EXPECT_CALL(device, close).Times(0);
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(false));
            EXPECT_CALL(device, close).Times(0);
        }

        chassis->closeDevices();
    }

    // Test where all devices were open
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, close).Times(1);
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, close).Times(1);
        }

        chassis->closeDevices();
    }

    // Test where closing first device fails. Verify still closes second device.
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, close)
                .WillOnce(Throw(std::runtime_error{"Unable to close device"}));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, close).Times(1);
        }

        chassis->closeDevices();
    }
}

TEST(ChassisTests, ClearErrorHistory)
{
    std::unique_ptr<Chassis> chassis = createChassis(1);
    MockServices services;

    chassis->initializeMonitoring(services);
    setChassisStatusToGood(*chassis);

    auto& device = getMockDevice(*chassis, 0);
    EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
    EXPECT_CALL(device, getPowerGood)
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
        .Times(1);

    // Power good is initially on: No fault or power supply error
    chassis->monitor(services);
    EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    EXPECT_FALSE(chassis->hasPowerGoodFault());
    EXPECT_TRUE(chassis->getPowerSupplyError().empty());

    // Set power supply error
    std::string error{
        "xyz.openbmc_project.Power.PowerSupply.Error.IoutOCFault"};
    chassis->setPowerSupplyError(error);

    // Power good fault detected: Power good changed to false
    chassis->monitor(services);
    EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    EXPECT_TRUE(chassis->hasPowerGoodFault());
    EXPECT_EQ(chassis->getPowerSupplyError(), error);

    // Clear error history
    chassis->clearErrorHistory();
    EXPECT_FALSE(chassis->hasPowerGoodFault());
    EXPECT_TRUE(chassis->getPowerSupplyError().empty());
}

TEST(ChassisTests, GetPowerGoodTimeout)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);

    // Verify the default timeout value, which is a meson option expressed in
    // seconds. getPowerGoodTimeout() returns duration expressed in
    // milliseconds. std::chrono classes are able to handle conversion.
    std::chrono::seconds defaultValueInSecs{PGOOD_TIMEOUT};
    EXPECT_EQ(chassis->getPowerGoodTimeout(), defaultValueInSecs);
    EXPECT_EQ(chassis->getPowerGoodTimeout().count(), (PGOOD_TIMEOUT * 1000));

    // Set to new value in seconds
    std::chrono::seconds newValueInSecs{5};
    chassis->setPowerGoodTimeout(newValueInSecs);
    EXPECT_EQ(chassis->getPowerGoodTimeout(), newValueInSecs);
    EXPECT_EQ(chassis->getPowerGoodTimeout().count(), 5000);

    // Set to new value in milliseconds
    std::chrono::milliseconds newValueInMillis{400};
    chassis->setPowerGoodTimeout(newValueInMillis);
    EXPECT_EQ(chassis->getPowerGoodTimeout(), newValueInMillis);
    EXPECT_EQ(chassis->getPowerGoodTimeout().count(), 400);
}

TEST(ChassisTests, SetPowerGoodTimeout)
{
    // The test variations are covered by GetPowerGoodTimeout
}

TEST(ChassisTests, GetPowerGoodFaultLogDelay)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);

    // Verify the default delay value
    std::chrono::seconds defaultValueInSecs{7};
    EXPECT_EQ(chassis->getPowerGoodFaultLogDelay(), defaultValueInSecs);
    EXPECT_EQ(chassis->getPowerGoodFaultLogDelay().count(), 7000);

    // Set to new value in seconds
    std::chrono::seconds newValueInSecs{5};
    chassis->setPowerGoodFaultLogDelay(newValueInSecs);
    EXPECT_EQ(chassis->getPowerGoodFaultLogDelay(), newValueInSecs);
    EXPECT_EQ(chassis->getPowerGoodFaultLogDelay().count(), 5000);

    // Set to new value in milliseconds
    std::chrono::milliseconds newValueInMillis{400};
    chassis->setPowerGoodFaultLogDelay(newValueInMillis);
    EXPECT_EQ(chassis->getPowerGoodFaultLogDelay(), newValueInMillis);
    EXPECT_EQ(chassis->getPowerGoodFaultLogDelay().count(), 400);
}

TEST(ChassisTests, SetPowerGoodFaultLogDelay)
{
    // The test variations are covered by GetPowerGoodFaultLogDelay
}

TEST(ChassisTests, GetPowerSupplyError)
{
    std::unique_ptr<Chassis> chassis = createChassis(0);

    // Verify default value is the empty string
    EXPECT_TRUE(chassis->getPowerSupplyError().empty());

    // Set to error string
    std::string error{
        "xyz.openbmc_project.Power.PowerSupply.Error.IoutOCFault"};
    chassis->setPowerSupplyError(error);
    EXPECT_EQ(chassis->getPowerSupplyError(), error);

    // Set to empty string
    chassis->setPowerSupplyError("");
    EXPECT_TRUE(chassis->getPowerSupplyError().empty());
}

TEST(ChassisTests, SetPowerSupplyError)
{
    // The test variations are covered by GetPowerSupplyError
}

TEST(ChassisTests, VerifyMonitoringInitialized)
{
    // The test variations are covered by GetStatusMonitor
}

TEST(ChassisTests, GetCurrentTime)
{
    // The test variations are covered by CheckForPowerGoodError
}

TEST(ChassisTests, OpenDeviceIfNeeded)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Test where works: Device was not open
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        chassis->monitor(services);
    }

    // Test where works: Device already open
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, open).Times(0);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis->monitor(services);
    }
}

TEST(ChassisTests, UpdatePowerGood)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(0);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent)
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Present property value could not be obtained."}));

        chassis->monitor(services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Present property value could not be obtained.");
    }

    // Test where not present: Sets power state and power good to off:
    // Closes devices
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(false));

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, close).Times(1);

        // Call monitor(). Chassis not present. Device closed. powerState
        // and powerGood set to off.
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Test where input power not good: Sets power state and power good to off:
    // Closes devices
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(false));

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, close).Times(1);

        // Call monitor(). Input power not good. Device closed. powerState
        // and powerGood set to off.
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Test where not available: Does not set initial power state or power good
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(*chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(false));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).Times(0);
        EXPECT_CALL(device, open).Times(0);
        EXPECT_CALL(device, close).Times(0);
        EXPECT_CALL(device, getPowerGood).Times(0);

        chassis->monitor(services);
        EXPECT_THROW(chassis->getPowerState(), std::runtime_error);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);
    }

    // Test where fails: Unable to read power good from power sequencer
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));

        chassis->monitor(services);
        EXPECT_THROW(chassis->getPowerState(), std::runtime_error);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);
    }

    // Test where works: Sets initial power state and power good
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    }
}

TEST(ChassisTests, ReadPowerGood)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Test with two power sequencers: Unable to open first power sequencer:
    // Second power sequencer is on: No previous power good value.
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(false));
            EXPECT_CALL(device, open)
                .WillOnce(Throw(std::runtime_error{"Unable to open device"}));
            EXPECT_CALL(device, getPowerGood).Times(0);
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(false));
            EXPECT_CALL(device, open).Times(1);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

        chassis->monitor(services);
        EXPECT_THROW(chassis->getPowerState(), std::runtime_error);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);
    }

    // Test with two power sequencers: Unable to read power good from first
    // power sequencer: Second power sequencer is off: Not in transition: Has
    // previous power good value
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Return(false));
        }

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);

        // Monitor first time. Reading power good works. powerState and
        // powerGood set to on.
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);

        // Monitor second time. Reading power good fails on first power
        // sequencer. Since second power sequencer is off and not in transition,
        // powerGood changed to off.
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Test with no power sequencers: Chassis power good should default to on
    {
        std::unique_ptr<Chassis> chassis = createChassis(0);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    }

    // Test with one power sequencer: Power good is on
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    }

    // Test with one power sequencer: Power good is off
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Test with two power sequencers: Power good is on
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    }

    // Test with two power sequencers: Power good is off
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Test with two power sequencers: One on and one off: Not in state
    // transition, so power good set to off
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
        }

        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Test with two power sequencers: One on and one off: In state transition,
    // so power good not changed: Has previous power good value
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Return(false));
            EXPECT_CALL(device, powerOff).Times(1);
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillOnce(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
        }

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        // Monitor first time. Both devices are on. Sets powerState and
        // powerGood to on.
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);

        // Power off chassis
        chassis->setPowerState(PowerState::off, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);

        // Monitor second time. First device is off. Second device is on. Does
        // not change powerGood.
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    }
}

TEST(ChassisTests, SetInitialPowerStateIfNeeded)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Test where initial power state set to on and then not changed on
    // subsequent calls
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);

        // Monitor first time: Power good is on: Sets power state to on
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);

        // Monitor second time: Power good is off: Does not change power state
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    }

    // Test where initial power state set to off and then not changed on
    // subsequent calls
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(false))
            .WillOnce(Return(true));

        // Monitor first time: Power good is off: Sets power state to off
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);

        // Monitor second time: Power good is on: Does not change power state
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
    }

    // Test where initial power state not set because power good is not defined
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));

        // Monitor: Reading power good fails: Power state not set since power
        // good is undefined
        chassis->monitor(services);
        EXPECT_THROW(chassis->getPowerState(), std::runtime_error);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);
    }
}

TEST(ChassisTests, PowerOn)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Test where fails: Unable to open first power sequencer device. Verify
    // continues to second device.
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        std::string deviceName{"pseq1"};
        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(false));
            EXPECT_CALL(device, open)
                .WillOnce(Throw(std::runtime_error{"Unable to open device"}));
            EXPECT_CALL(device, powerOn).Times(0);
            EXPECT_CALL(device, getName).WillOnce(ReturnRef(deviceName));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(false));
            EXPECT_CALL(device, open).Times(1);
            EXPECT_CALL(device, powerOn).Times(1);
        }

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        chassis->setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to power on device pseq1 in chassis 1: Unable to open device");
    }

    // Test where fails: Unable to power on first power sequencer device. Verify
    // continues to second device.
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        std::string deviceName{"pseq1"};
        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, powerOn)
                .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
            EXPECT_CALL(device, getName).WillOnce(ReturnRef(deviceName));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, powerOn).Times(1);
        }

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        chassis->setPowerState(PowerState::on, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to power on device pseq1 in chassis 1: Unable to write GPIO");
    }

    // Test where works
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, open).Times(2);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, open).Times(2);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));
            EXPECT_CALL(device, powerOn).Times(1);
        }

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        // Call monitor to set initial power state to off
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);

        // Call setPowerState to set power state to on
        chassis->setPowerState(PowerState::on, services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
    }
}

TEST(ChassisTests, PowerOff)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Test where fails: Unable to open first power sequencer device. Verify
    // continues to second device.
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        std::string deviceName{"pseq1"};
        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(false));
            EXPECT_CALL(device, open)
                .WillOnce(Throw(std::runtime_error{"Unable to open device"}));
            EXPECT_CALL(device, powerOff).Times(0);
            EXPECT_CALL(device, getName).WillOnce(ReturnRef(deviceName));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(false));
            EXPECT_CALL(device, open).Times(1);
            EXPECT_CALL(device, powerOff).Times(1);
        }

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        chassis->setPowerState(PowerState::off, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to power off device pseq1 in chassis 1: Unable to open device");
    }

    // Test where fails: Unable to power off first power sequencer device.
    // Verify continues to second device.
    try
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        std::string deviceName{"pseq1"};
        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, powerOff)
                .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
            EXPECT_CALL(device, getName).WillOnce(ReturnRef(deviceName));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillOnce(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
        }

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        chassis->setPowerState(PowerState::off, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to power off device pseq1 in chassis 1: Unable to write GPIO");
    }

    // Test where works
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, open).Times(2);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(false));
            EXPECT_CALL(device, open).Times(2);
            EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));
            EXPECT_CALL(device, powerOff).Times(1);
        }

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        // Call monitor to set initial power state to on
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);

        // Call setPowerState to set power state to off
        chassis->setPowerState(PowerState::off, services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
    }
}

TEST(ChassisTests, UpdateInPowerStateTransition)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Does not update: Not in state transition
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(false))
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        EXPECT_FALSE(chassis->isInPowerStateTransition());

        // Monitor: Not in transition: Power state and power good set to off
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_FALSE(chassis->isInPowerStateTransition());

        // Monitor: Not in transition: Power good changes to on: Power state
        // still off
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_FALSE(chassis->isInPowerStateTransition());

        // Monitor: Not in transition: Power good changes to off: Now the same
        // as power state
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
    }

    // Does not update: Power good is not defined
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        EXPECT_FALSE(chassis->isInPowerStateTransition());

        // Power on chassis: Sets power state: In transition: Power good
        // undefined
        chassis->setPowerState(PowerState::on, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);

        // Monitor: Reading power good fails: Does not update in transition
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);
    }

    // Does not update: Power state is on and power good is off
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        // Monitor: Not in transition: Power state and power good set to off
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);

        // Power on chassis: Sets power state: In transition
        chassis->setPowerState(PowerState::on, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);

        // Monitor: Power state on and power good off: Does not update in
        // transition
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Does not update: Power state is off and power good is on
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOff).Times(1);
        EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        // Monitor: Not in transition: Power state and power good set to on
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);

        // Power off chassis: Sets power state: In transition
        chassis->setPowerState(PowerState::off, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);

        // Monitor: Power state off and power good on: Does not update in
        // transition
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    }

    // Does update: Power state and power good both off
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOff).Times(1);
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);

        // Monitor: Not in transition: Power state and power good set to on
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);

        // Power off chassis: Sets power state: In transition
        chassis->setPowerState(PowerState::off, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);

        // Monitor: Power state and power good both off: Updates in transition
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
    }

    // Does update: Power state and power good both on
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(false))
            .WillOnce(Return(true));

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);

        // Monitor: Not in transition: Power state and power good set to off
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);

        // Power on chassis: Sets power state: In transition
        chassis->setPowerState(PowerState::on, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);

        // Monitor: Power state and power good both on: Updates in transition
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
    }
}

TEST(ChassisTests, CheckForPowerGoodError)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // State transition: Power on timeout
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services,
                    logErrorMsg("Power on failed in chassis 1: Timeout"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.PowerOnTimeout",
                             Entry::Level::Critical, _))
            .Times(1);

        // Set power good timeout to 10 milliseconds
        std::chrono::milliseconds timeout{10};
        chassis->setPowerGoodTimeout(timeout);

        // Power on chassis: Power good value undefined
        chassis->setPowerState(PowerState::on, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);

        // Monitor: Power good is off: Timeout has not expired
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good is off: Timeout has not expired
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Sleep for timeout time so that timeout will elapse
        std::this_thread::sleep_for(timeout);

        // Monitor: Power good fault due to power on timeout was logged
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_TRUE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Fault should not be logged again
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_TRUE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }

    // State transition: Power off timeout
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOff).Times(1);
        EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);
        EXPECT_CALL(services,
                    logErrorMsg("Power off failed in chassis 1: Timeout"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.PowerOffTimeout",
                             Entry::Level::Critical, _))
            .Times(1);

        // Set power good timeout to 10 milliseconds
        std::chrono::milliseconds timeout{10};
        chassis->setPowerGoodTimeout(timeout);

        // Power off chassis: Power good value undefined
        chassis->setPowerState(PowerState::off, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);

        // Monitor: Power good is on: Timeout has not expired
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good is on: Timeout has not expired
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Sleep for timeout time so that timeout will elapse
        std::this_thread::sleep_for(timeout);

        // Monitor: Power off timeout was logged: Note this is not a power good
        // fault
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Error should not be logged again
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());
    }

    // State transition: Power on completes without timeout: No subsequent power
    // good fault either
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(false))
            .WillOnce(Return(false))
            .WillRepeatedly(Return(true));

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services, logErrorMsg).Times(0);
        EXPECT_CALL(services, logError).Times(0);

        // Set power good timeout to 10 milliseconds
        std::chrono::milliseconds timeout{10};
        chassis->setPowerGoodTimeout(timeout);

        // Power on chassis: Power good value undefined
        chassis->setPowerState(PowerState::on, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);

        // Monitor: Power good is off: No power good fault since in transition:
        // Power good timeout has not expired
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good is off: No power good fault since in transition:
        // Power good timeout has not expired
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good is on: Power on complete with no timeout
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good is still on: Verify no power good fault
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());
    }

    // State transition: Power off completes without timeout
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOff).Times(1);
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));

        EXPECT_CALL(services, logInfoMsg("Powering off chassis 1")).Times(1);
        EXPECT_CALL(services, logErrorMsg).Times(0);
        EXPECT_CALL(services, logError).Times(0);

        // Set power good timeout to 10 milliseconds
        std::chrono::milliseconds timeout{10};
        chassis->setPowerGoodTimeout(timeout);

        // Power off chassis: Power good value undefined
        chassis->setPowerState(PowerState::off, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);

        // Monitor: Power good is on: Timeout has not expired
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good is on: Timeout has not expired
        chassis->monitor(services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good is off: Power off complete with no timeout: No
        // power good fault since power state is off
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::off);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_FALSE(chassis->hasPowerGoodFault());
    }

    // Power good fault detection: Faulted detected: Verify delayed logging
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services,
                    logError("xyz.openbmc_project.Power.Error.Shutdown",
                             Entry::Level::Critical, _))
            .Times(1);

        // Monitor: Power good is on
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Set fault logging delay time to 10 milliseconds
        std::chrono::milliseconds delay{10};
        chassis->setPowerGoodFaultLogDelay(delay);

        // Monitor: Power good is off: Power fault detected: Logging is delayed
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Logging delay not over yet
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Sleep for delay time so that delay will elapse
        std::this_thread::sleep_for(delay);

        // Monitor: Fault was logged
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Fault should not be logged again
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }

    // Power good fault detection skipped: Invalid chassis status
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault).Times(0);

        EXPECT_CALL(services, logErrorMsg).Times(0);
        EXPECT_CALL(services, logError).Times(0);

        // Initial scenario where power state and power good are on: Not in
        // transition
        {
            chassis->initializeMonitoring(services);
            setChassisStatusToGood(*chassis);

            chassis->monitor(services);
            EXPECT_EQ(chassis->getPowerState(), PowerState::on);
            EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
            EXPECT_FALSE(chassis->hasPowerGoodFault());
            EXPECT_FALSE(chassis->isInPowerStateTransition());
        }

        // Test where not present
        {
            chassis->initializeMonitoring(services);
            auto& monitor = getMockStatusMonitor(*chassis);
            EXPECT_CALL(monitor, isPresent)
                .WillOnce(Return(true))         // updatePowerGood()
                .WillOnce(Return(true))         // updatePowerGood()
                .WillRepeatedly(Return(false)); // checkForPowerGoodError()
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            // Chassis is not present when checkforPowerGoodError() called:
            // Power good is off, but not detected
            chassis->monitor(services);
            EXPECT_EQ(chassis->getPowerState(), PowerState::on);
            EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
            EXPECT_FALSE(chassis->hasPowerGoodFault());
        }

        // Test where not available
        {
            chassis->initializeMonitoring(services);
            auto& monitor = getMockStatusMonitor(*chassis);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable)
                .WillOnce(Return(true))         // updatePowerGood()
                .WillRepeatedly(Return(false)); // checkForPowerGoodError()
            EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

            // Chassis is not available when checkforPowerGoodError() called:
            // Power good is off, but not detected
            chassis->monitor(services);
            EXPECT_EQ(chassis->getPowerState(), PowerState::on);
            EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
            EXPECT_FALSE(chassis->hasPowerGoodFault());
        }

        // Test where no input power
        {
            chassis->initializeMonitoring(services);
            auto& monitor = getMockStatusMonitor(*chassis);
            EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
            EXPECT_CALL(monitor, isInputPowerGood)
                .WillOnce(Return(true))         // updatePowerGood()
                .WillOnce(Return(true))         // updatePowerGood()
                .WillRepeatedly(Return(false)); // checkForPowerGoodError()

            // No input power when checkforPowerGoodError() called: Power good
            // is off, but not detected
            chassis->monitor(services);
            EXPECT_EQ(chassis->getPowerState(), PowerState::on);
            EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
            EXPECT_FALSE(chassis->hasPowerGoodFault());
        }
    }

    // Power good fault detection skipped: Power state and power good not
    // defined
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault).Times(0);

        EXPECT_CALL(services, logErrorMsg).Times(0);
        EXPECT_CALL(services, logError).Times(0);

        // Monitor: Power state and power good not defined due to exception
        chassis->monitor(services);
        EXPECT_THROW(chassis->getPowerState(), std::runtime_error);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);
        EXPECT_FALSE(chassis->hasPowerGoodFault());
    }
}

TEST(ChassisTests, HandlePowerGoodTimeout)
{
    // Test variations covered in CheckForPowerGoodError
}

TEST(ChassisTests, HandlePowerGoodFault)
{
    // Test variations covered in CheckForPowerGoodError
}

TEST(ChassisTests, LogPowerOffTimeout)
{
    // Test variations covered in CheckForPowerGoodError
}

TEST(ChassisTests, LogPowerGoodFault)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Test where error logged for specific voltage rail
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        std::string error{
            "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault"};
        std::map<std::string, std::string> additionalData{
            {"RAIL_NAME", "vdd"}, {"DEVICE_NAME", "pseq0"}};

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault(_, "", _))
            .WillOnce(DoAll(SetArgReferee<2>(additionalData), Return(error)));

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services,
                    logError(error, Entry::Level::Critical, additionalData))
            .Times(1);

        // Set fault logging delay time to 0 milliseconds
        std::chrono::milliseconds delay{0};
        chassis->setPowerGoodFaultLogDelay(delay);

        // Monitor: Initially power good is true
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good changes to false: Power good fault detected with
        // delay before logging
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Power good fault logged since delay has elapsed
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }

    // Test where power supply error is logged
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        std::string error{
            "xyz.openbmc_project.Power.PowerSupply.Error.IoutOCFault"};

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services, logError(error, Entry::Level::Critical, _))
            .Times(1);

        // Set fault logging delay time to 0 milliseconds
        std::chrono::milliseconds delay{0};
        chassis->setPowerGoodFaultLogDelay(delay);

        // Set power supply error
        chassis->setPowerSupplyError(error);

        // Monitor: Initially power good is true
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good changes to false: Power good fault detected with
        // delay before logging
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Power good fault logged since delay has elapsed
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }

    // Test where general power on timeout error is logged
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        std::string error{"xyz.openbmc_project.Power.Error.PowerOnTimeout"};

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);
        EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));

        EXPECT_CALL(services, logInfoMsg("Powering on chassis 1")).Times(1);
        EXPECT_CALL(services,
                    logErrorMsg("Power on failed in chassis 1: Timeout"))
            .Times(1);
        EXPECT_CALL(services, logError(error, Entry::Level::Critical, _))
            .Times(1);

        // Set power good timeout to 0 milliseconds
        std::chrono::milliseconds timeout{0};
        chassis->setPowerGoodTimeout(timeout);

        // Power on chassis: Power good value undefined
        chassis->setPowerState(PowerState::on, services);
        EXPECT_TRUE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_THROW(chassis->getPowerGood(), std::runtime_error);

        // Monitor: Power good fault due to power on timeout was logged
        chassis->monitor(services);
        EXPECT_FALSE(chassis->isInPowerStateTransition());
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_TRUE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }

    // Test where general shutdown error is logged
    {
        std::unique_ptr<Chassis> chassis = createChassis(1);
        MockServices services;

        std::string error{"xyz.openbmc_project.Power.Error.Shutdown"};

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        auto& device = getMockDevice(*chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillRepeatedly(Return(false));
        EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services, logError(error, Entry::Level::Critical, _))
            .Times(1);

        // Set fault logging delay time to 0 milliseconds
        std::chrono::milliseconds delay{0};
        chassis->setPowerGoodFaultLogDelay(delay);

        // Monitor: Initially power good is true
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good changes to false: Power good fault detected with
        // delay before logging
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Power good fault logged since delay has elapsed
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }
}

TEST(ChassisTests, FindPowerGoodFaultInRail)
{
    // Since this is a private method it cannot be called directly from a
    // testcase. It is called indirectly using a public method.

    // Test where fault found in rail in first power sequencer
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        std::string error{
            "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault"};
        std::map<std::string, std::string> additionalData{
            {"RAIL_NAME", "vio"}, {"DEVICE_NAME", "pseq0"}};

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(device, findPgoodFault(_, "", _))
                .WillOnce(
                    DoAll(SetArgReferee<2>(additionalData), Return(error)));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
            EXPECT_CALL(device, findPgoodFault).Times(0);
        }

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services,
                    logError(error, Entry::Level::Critical, additionalData))
            .Times(1);

        // Set fault logging delay time to 0 milliseconds
        std::chrono::milliseconds delay{0};
        chassis->setPowerGoodFaultLogDelay(delay);

        // Monitor: Initially power good is true
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good changes to false: Power good fault detected with
        // delay before logging
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Power good fault logged since delay has elapsed
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }

    // Test where fault found in rail in second power sequencer
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        std::string error{
            "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault"};
        std::map<std::string, std::string> additionalData{
            {"RAIL_NAME", "vdd"}, {"DEVICE_NAME", "pseq1"}};

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
            EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(device, findPgoodFault(_, "", _))
                .WillOnce(
                    DoAll(SetArgReferee<2>(additionalData), Return(error)));
        }

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services,
                    logError(error, Entry::Level::Critical, additionalData))
            .Times(1);

        // Set fault logging delay time to 0 milliseconds
        std::chrono::milliseconds delay{0};
        chassis->setPowerGoodFaultLogDelay(delay);

        // Monitor: Initially power good is true
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good changes to false: Power good fault detected with
        // delay before logging
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Power good fault logged since delay has elapsed
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }

    // Test where no fault found in any power sequencer
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        std::string error{"xyz.openbmc_project.Power.Error.Shutdown"};

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
            EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(device, findPgoodFault).WillOnce(Return(""));
        }

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(services, logError(error, Entry::Level::Critical, _))
            .Times(1);

        // Set fault logging delay time to 0 milliseconds
        std::chrono::milliseconds delay{0};
        chassis->setPowerGoodFaultLogDelay(delay);

        // Monitor: Initially power good is true
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good changes to false: Power good fault detected with
        // delay before logging
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Power good fault logged since delay has elapsed
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }

    // Test where exception thrown while trying to find fault in power sequencer
    {
        std::unique_ptr<Chassis> chassis = createChassis(2);
        MockServices services;

        std::string error{"xyz.openbmc_project.Power.Error.Shutdown"};
        std::map<std::string, std::string> additionalData{
            {"ERROR", "Unable to acquire GPIO line"}};

        chassis->initializeMonitoring(services);
        setChassisStatusToGood(*chassis);

        {
            auto& device = getMockDevice(*chassis, 0);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood)
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            EXPECT_CALL(device, findPgoodFault)
                .WillOnce(
                    Throw(std::runtime_error{"Unable to acquire GPIO line"}));
        }
        {
            auto& device = getMockDevice(*chassis, 1);
            EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
            EXPECT_CALL(device, getPowerGood).WillRepeatedly(Return(true));
            EXPECT_CALL(device, findPgoodFault).Times(0);
        }

        EXPECT_CALL(services, logErrorMsg("Power good fault in chassis 1"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Unable to find rail that caused power good fault in chassis 1: Unable to acquire GPIO line"))
            .Times(1);
        EXPECT_CALL(services,
                    logError(error, Entry::Level::Critical, additionalData))
            .Times(1);

        // Set fault logging delay time to 0 milliseconds
        std::chrono::milliseconds delay{0};
        chassis->setPowerGoodFaultLogDelay(delay);

        // Monitor: Initially power good is true
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::on);
        EXPECT_FALSE(chassis->hasPowerGoodFault());

        // Monitor: Power good changes to false: Power good fault detected with
        // delay before logging
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_FALSE(chassis->getPowerGoodFault().wasLogged);

        // Monitor: Power good fault logged since delay has elapsed
        chassis->monitor(services);
        EXPECT_EQ(chassis->getPowerState(), PowerState::on);
        EXPECT_EQ(chassis->getPowerGood(), PowerGood::off);
        EXPECT_TRUE(chassis->hasPowerGoodFault());
        EXPECT_FALSE(chassis->getPowerGoodFault().wasTimeout);
        EXPECT_TRUE(chassis->getPowerGoodFault().wasLogged);
    }
}
