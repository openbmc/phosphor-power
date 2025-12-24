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

#include "chassis.hpp"
#include "chassis_status_monitor.hpp"
#include "mock_chassis_status_monitor.hpp"
#include "mock_device.hpp"
#include "mock_services.hpp"
#include "power_sequencer_device.hpp"
#include "rail.hpp"
#include "ucd90160_device.hpp"

#include <stddef.h> // for size_t

#include <cstdint>
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
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};
    MockServices services;

    // Test where it is called first time
    EXPECT_THROW(chassis.getStatusMonitor(), std::runtime_error);
    chassis.initializeMonitoring(services);
    ChassisStatusMonitor* monitorPtr = &(chassis.getStatusMonitor());

    // Test where it is called second time
    chassis.initializeMonitoring(services);
    EXPECT_NE(monitorPtr, &(chassis.getStatusMonitor()));
}

TEST(ChassisTests, GetStatusMonitor)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis.getStatusMonitor();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where works
    {
        chassis.initializeMonitoring(services);
        ChassisStatusMonitor& monitor = chassis.getStatusMonitor();
        MockChassisStatusMonitor& mockMonitor =
            static_cast<MockChassisStatusMonitor&>(monitor);
        EXPECT_CALL(mockMonitor, isPresent()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis.isPresent());
    }
}

TEST(ChassisTests, IsPresent)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    monitorOptions.isPresentMonitored = true;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis.isPresent();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Present property value could not be obtained."}));
        chassis.isPresent();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Present property value could not be obtained.");
    }

    // Test where works: true
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis.isPresent());
    }

    // Test where works: false
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent()).Times(1).WillOnce(Return(false));
        EXPECT_FALSE(chassis.isPresent());
    }
}

TEST(ChassisTests, IsAvailable)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    monitorOptions.isAvailableMonitored = true;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis.isAvailable();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isAvailable())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Available property value could not be obtained."}));
        chassis.isAvailable();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(),
                     "Available property value could not be obtained.");
    }

    // Test where works: true
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isAvailable()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis.isAvailable());
    }

    // Test where works: false
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isAvailable()).Times(1).WillOnce(Return(false));
        EXPECT_FALSE(chassis.isAvailable());
    }
}

TEST(ChassisTests, IsEnabled)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    monitorOptions.isEnabledMonitored = true;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis.isEnabled();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isEnabled())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Enabled property value could not be obtained."}));
        chassis.isEnabled();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Enabled property value could not be obtained.");
    }

    // Test where works: true
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isEnabled()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis.isEnabled());
    }

    // Test where works: false
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isEnabled()).Times(1).WillOnce(Return(false));
        EXPECT_FALSE(chassis.isEnabled());
    }
}

TEST(ChassisTests, IsInputPowerGood)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    monitorOptions.isInputPowerStatusMonitored = true;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis.isInputPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isInputPowerGood())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Input power Status property value could not be obtained."}));
        chassis.isInputPowerGood();
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
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isInputPowerGood())
            .Times(1)
            .WillOnce(Return(true));
        EXPECT_TRUE(chassis.isInputPowerGood());
    }

    // Test where works: false
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isInputPowerGood())
            .Times(1)
            .WillOnce(Return(false));
        EXPECT_FALSE(chassis.isInputPowerGood());
    }
}

TEST(ChassisTests, IsPowerSuppliesPowerGood)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    ChassisStatusMonitorOptions monitorOptions;
    monitorOptions.isPowerSuppliesStatusMonitored = true;
    Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                    monitorOptions};
    MockServices services;

    // Test where fails: Monitoring not initialized
    try
    {
        chassis.isPowerSuppliesPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPowerSuppliesPowerGood())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Power supplies power Status property value could not be obtained."}));
        chassis.isPowerSuppliesPowerGood();
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
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPowerSuppliesPowerGood())
            .Times(1)
            .WillOnce(Return(true));
        EXPECT_TRUE(chassis.isPowerSuppliesPowerGood());
    }

    // Test where works: false
    {
        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPowerSuppliesPowerGood())
            .Times(1)
            .WillOnce(Return(false));
        EXPECT_FALSE(chassis.isPowerSuppliesPowerGood());
    }
}

TEST(ChassisTests, GetPowerState)
{
    // Test where fails: Power state not set
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};

        chassis.getPowerState();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(),
                     "Power state could not be obtained for chassis 1");
    }

    // Test where works: Value is OFF
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        chassis.monitor(services);

        EXPECT_EQ(chassis.getPowerState(), PowerState::OFF);
    }

    // Test where works: Value is ON
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, powerOn).Times(1);

        chassis.setPowerState(PowerState::ON, services);

        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
    }
}

TEST(ChassisTests, CanSetPowerState)
{
    // Test where fails: Monitoring not initialized
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};

        chassis.canSetPowerState(PowerState::ON);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where true: All chassis status values good
    // Initial power state not set
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillOnce(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillOnce(Return(true));

        auto [canSet, reason] = chassis.canSetPowerState(PowerState::ON);
        EXPECT_TRUE(canSet);
        EXPECT_TRUE(reason.empty());
    }

    // Test where true: Chassis is not enabled, but request is OFF
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(false));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);

        auto [canSet, reason] = chassis.canSetPowerState(PowerState::OFF);
        EXPECT_TRUE(canSet);
        EXPECT_TRUE(reason.empty());
    }

    // Test where false: Chassis is already at the requested state
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);

        auto [canSet, reason] = chassis.canSetPowerState(PowerState::ON);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis is already at requested state");
    }

    // Test where false: Chassis is not present
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(false));

        auto [canSet, reason] = chassis.canSetPowerState(PowerState::ON);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis is not present");
    }

    // Test where false: Chassis is not enabled and request is ON
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(false));

        auto [canSet, reason] = chassis.canSetPowerState(PowerState::ON);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis is not enabled");
    }

    // Test where false: Chassis does not have input power
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillOnce(Return(false));

        auto [canSet, reason] = chassis.canSetPowerState(PowerState::ON);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis does not have input power");
    }

    // Test where false: Chassis is not available
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillOnce(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillOnce(Return(false));

        auto [canSet, reason] = chassis.canSetPowerState(PowerState::ON);
        EXPECT_FALSE(canSet);
        EXPECT_EQ(reason, "Chassis is not available");
    }

    // Test where false: Unable to determine chassis status
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent)
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Present property value could not be obtained."}));

        auto [canSet, reason] = chassis.canSetPowerState(PowerState::ON);
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
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.setPowerState(PowerState::ON, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent)
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Present property value could not be obtained."}));

        chassis.setPowerState(PowerState::ON, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to set chassis 1 to state ON: Error determining chassis status: Present property value could not be obtained.");
    }

    // Test where fails: New power state not allowed based on chassis status
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(false));

        chassis.setPowerState(PowerState::ON, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to set chassis 1 to state ON: Chassis is not present");
    }

    // Test where fails: Power on: Unable to power on first power sequencer
    // device. Verify continues to second device.
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillOnce(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillOnce(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        std::string device0Name{"pseq0"};
        EXPECT_CALL(device0, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device0, powerOn)
            .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
        EXPECT_CALL(device0, getName).WillOnce(ReturnRef(device0Name));

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device1, powerOn).Times(1);

        chassis.setPowerState(PowerState::ON, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to power on device pseq0 in chassis 1: Unable to write GPIO");
    }

    // Test where fails: Power off: Unable to power off first power sequencer
    // device. Verify continues to second device.
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillOnce(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillOnce(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillOnce(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        std::string device0Name{"pseq0"};
        EXPECT_CALL(device0, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device0, powerOff)
            .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
        EXPECT_CALL(device0, getName).WillOnce(ReturnRef(device0Name));

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device1, powerOff).Times(1);

        chassis.setPowerState(PowerState::OFF, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unable to power off device pseq0 in chassis 1: Unable to write GPIO");
    }

    // Test where works: Power on: Devices not open
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen)
            .WillOnce(Return(false))
            .WillOnce(Return(true));
        EXPECT_CALL(device0, open).Times(1);
        EXPECT_CALL(device0, getPowerGood).WillOnce(Return(false));
        EXPECT_CALL(device0, powerOn).Times(1);

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen)
            .WillOnce(Return(false))
            .WillOnce(Return(true));
        EXPECT_CALL(device1, open).Times(1);
        EXPECT_CALL(device1, getPowerGood).WillOnce(Return(false));
        EXPECT_CALL(device1, powerOn).Times(1);

        // Call monitor to set initial power state to OFF
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::OFF);

        // Call setPowerState to set power state to ON
        chassis.setPowerState(PowerState::ON, services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
    }

    // Test where works: Power off: Devices already open
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device0, open).Times(0);
        EXPECT_CALL(device0, getPowerGood).WillOnce(Return(true));
        EXPECT_CALL(device0, powerOff).Times(1);

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device1, open).Times(0);
        EXPECT_CALL(device1, getPowerGood).WillOnce(Return(true));
        EXPECT_CALL(device1, powerOff).Times(1);

        // Call monitor to set initial power state to ON
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);

        // Call setPowerState to set power state to OFF
        chassis.setPowerState(PowerState::OFF, services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::OFF);
    }
}

TEST(ChassisTests, GetPowerGood)
{
    // Test where fails: Power good not set
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};

        chassis.getPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(),
                     "Power good could not be obtained for chassis 1");
    }

    // Test where works: Value is OFF
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(false));

        chassis.monitor(services);

        EXPECT_EQ(chassis.getPowerGood(), PowerGood::OFF);
    }

    // Test where works: Value is ON
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis.monitor(services);

        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);
    }
}

TEST(ChassisTests, Monitor)
{
    // Test where fails: Monitoring not initialized
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.monitor(services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent)
            .Times(1)
            .WillOnce(Throw(std::runtime_error{
                "Present property value could not be obtained."}));

        chassis.monitor(services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Present property value could not be obtained.");
    }

    // Test where fails: Not present: Sets power state and power good to OFF:
    // Closes devices
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent)
            .WillOnce(Return(true))
            .WillOnce(Return(true))
            .WillOnce(Return(false));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen)
            .WillOnce(Return(false))
            .WillOnce(Return(true));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, close).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        // Monitor first time. Chassis present. Device opened. powerState
        // and powerGood set to ON.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);

        // Monitor second time. Chassis not present. Device closed. powerState
        // and powerGood set to OFF.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::OFF);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::OFF);
    }

    // Test where fails: Input power not good: Sets power state and power good
    // to OFF: Closes devices
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood)
            .WillOnce(Return(true))
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen)
            .WillOnce(Return(false))
            .WillOnce(Return(true));
        EXPECT_CALL(device, open).Times(1);
        EXPECT_CALL(device, close).Times(1);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        // Monitor first time. Input power good. Device opened. powerState and
        // powerGood set to ON.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);

        // Monitor second time. Input power not good. Device closed. powerState
        // and powerGood set to OFF.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::OFF);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::OFF);
    }

    // Test where fails: Not available
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillOnce(Return(false));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).Times(0);
        EXPECT_CALL(device, open).Times(0);
        EXPECT_CALL(device, close).Times(0);
        EXPECT_CALL(device, getPowerGood).Times(0);

        chassis.monitor(services);
        EXPECT_THROW(chassis.getPowerState(), std::runtime_error);
        EXPECT_THROW(chassis.getPowerGood(), std::runtime_error);
    }

    // Test where fails: Unable to open power sequencer: Has no previous power
    // good value: Unable to set initial power state value
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillOnce(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device, open)
            .WillOnce(Throw(std::runtime_error{"Unable to open device"}));
        EXPECT_CALL(device, close).Times(0);
        EXPECT_CALL(device, getPowerGood).Times(0);

        chassis.monitor(services);
        EXPECT_THROW(chassis.getPowerState(), std::runtime_error);
        EXPECT_THROW(chassis.getPowerGood(), std::runtime_error);
    }

    // Test where fails: Unable to read power good from power sequencer: Has
    // previous power good value
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device, open).Times(0);
        EXPECT_CALL(device, getPowerGood)
            .WillOnce(Return(true))
            .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));

        // Monitor first time. Reading power good works. powerState and
        // powerGood set to ON.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);

        // Monitor second time. Reading power good fails. powerState and
        // powerGood unchanged.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);
    }

    // Test where works: Zero power sequencers: Chassis should default to ON
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);
    }

    // Test where works: One power sequencer: Sets initial power state to ON
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device = getMockDevice(chassis, 0);
        EXPECT_CALL(device, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device, open).Times(0);
        EXPECT_CALL(device, getPowerGood).WillOnce(Return(true));

        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);
    }

    // Test where works: Two power sequencers: Does not set initial power state
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device0, powerOn).Times(1);
        EXPECT_CALL(device0, getPowerGood).WillOnce(Return(false));

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device1, powerOn).Times(1);
        EXPECT_CALL(device1, getPowerGood).WillOnce(Return(false));

        // Set power state to ON. Power good still undefined.
        chassis.setPowerState(PowerState::ON, services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_THROW(chassis.getPowerGood(), std::runtime_error);

        // Monitor. Power good is now OFF. Power state is still ON.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::OFF);
    }

    // Test where works: All power sequencers off: Opens devices: Sets initial
    // power state to OFF
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device0, open).Times(1);
        EXPECT_CALL(device0, getPowerGood).WillOnce(Return(false));

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device1, open).Times(1);
        EXPECT_CALL(device1, getPowerGood).WillOnce(Return(false));

        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::OFF);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::OFF);
    }

    // Test where works: One power sequencer on and one off: Has no previous
    // power good value: Devices already open
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device0, open).Times(0);
        EXPECT_CALL(device0, getPowerGood).WillOnce(Return(true));

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device1, open).Times(0);
        EXPECT_CALL(device1, getPowerGood).WillOnce(Return(false));

        chassis.monitor(services);
        EXPECT_THROW(chassis.getPowerState(), std::runtime_error);
        EXPECT_THROW(chassis.getPowerGood(), std::runtime_error);
    }

    // Test where works: One power sequencer on and one off: Has previous power
    // good value
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device0, getPowerGood)
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device1, getPowerGood)
            .WillOnce(Return(true))
            .WillOnce(Return(true));

        // Monitor first time. Both devices are on. Sets powerState and
        // powerGood to ON.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);

        // Monitor second time. First device is off. Second device is on. Does
        // not change powerState and powerGood.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::ON);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);
    }

    // Test where works: All power sequencers on: Already has initial power
    // state value
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        chassis.initializeMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
        EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device0, powerOff).Times(1);
        EXPECT_CALL(device0, getPowerGood).WillOnce(Return(true));

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(device1, powerOff).Times(1);
        EXPECT_CALL(device1, getPowerGood).WillOnce(Return(true));

        // Set power state to OFF. Power good still undefined.
        chassis.setPowerState(PowerState::OFF, services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::OFF);
        EXPECT_THROW(chassis.getPowerGood(), std::runtime_error);

        // Monitor. Power good is now ON. Power state is still OFF.
        chassis.monitor(services);
        EXPECT_EQ(chassis.getPowerState(), PowerState::OFF);
        EXPECT_EQ(chassis.getPowerGood(), PowerGood::ON);
    }
}

TEST(ChassisTests, CloseDevices)
{
    // Test where all devices were already closed
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device0, close).Times(0);

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillOnce(Return(false));
        EXPECT_CALL(device1, close).Times(0);

        chassis.closeDevices();
    }

    // Test where all devices were open
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device0, close).Times(1);

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device1, close).Times(1);

        chassis.closeDevices();
    }

    // Test where closing first device fails. Verify still closes second device.
    {
        size_t number{1};
        std::string inventoryPath{
            "/xyz/openbmc_project/inventory/system/chassis"};
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
        powerSequencers.emplace_back(createMockPowerSequencer());
        powerSequencers.emplace_back(createMockPowerSequencer());
        ChassisStatusMonitorOptions monitorOptions;
        Chassis chassis{number, inventoryPath, std::move(powerSequencers),
                        monitorOptions};
        MockServices services;

        auto& device0 = getMockDevice(chassis, 0);
        EXPECT_CALL(device0, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device0, close)
            .WillOnce(Throw(std::runtime_error{"Unable to close device"}));

        auto& device1 = getMockDevice(chassis, 1);
        EXPECT_CALL(device1, isOpen).WillOnce(Return(true));
        EXPECT_CALL(device1, close).Times(1);

        chassis.closeDevices();
    }
}
