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
using ::testing::Throw;

/**
 * Creates a PowerSequencerDevice instance.
 *
 * PowerSequencerDevice is an abstract base class. The actual object type
 * created is a UCD90160Device.
 *
 * @param bus I2C bus for the device
 * @param address I2C address for the device
 * @return PowerSequencerDevice instance
 */
std::unique_ptr<PowerSequencerDevice> createPowerSequencer(uint8_t bus,
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
 * Returns the MockChassisStatusMonitor within a Chassis.
 *
 * Assumes that initializeStatusMonitoring() has been called with a MockServices
 * parameter.
 *
 * Throws an exception if initializeStatusMonitoring() has not been called.
 *
 * @param chassis Chassis object
 * @return MockChassisStatusMonitor reference
 */
MockChassisStatusMonitor& getMockStatusMonitor(Chassis& chassis)
{
    return static_cast<MockChassisStatusMonitor&>(chassis.getStatusMonitor());
}

TEST(ChassisTests, Constructor)
{
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    powerSequencers.emplace_back(createPowerSequencer(3, 0x70));
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
    powerSequencers.emplace_back(createPowerSequencer(3, 0x70));
    powerSequencers.emplace_back(createPowerSequencer(4, 0x32));
    powerSequencers.emplace_back(createPowerSequencer(10, 0x16));
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

TEST(ChassisTests, InitializeStatusMonitoring)
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
    chassis.initializeStatusMonitoring(services);
    ChassisStatusMonitor* monitorPtr = &(chassis.getStatusMonitor());

    // Test where it is called second time
    chassis.initializeStatusMonitoring(services);
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
        EXPECT_STREQ(e.what(),
                     "Status monitoring not initialized for chassis 1");
    }

    // Test where works
    {
        chassis.initializeStatusMonitoring(services);
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
        EXPECT_STREQ(e.what(),
                     "Status monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeStatusMonitoring(services);
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
        chassis.initializeStatusMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPresent()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis.isPresent());
    }

    // Test where works: false
    {
        chassis.initializeStatusMonitoring(services);
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
        EXPECT_STREQ(e.what(),
                     "Status monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeStatusMonitoring(services);
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
        chassis.initializeStatusMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isAvailable()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis.isAvailable());
    }

    // Test where works: false
    {
        chassis.initializeStatusMonitoring(services);
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
        EXPECT_STREQ(e.what(),
                     "Status monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeStatusMonitoring(services);
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
        chassis.initializeStatusMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isEnabled()).Times(1).WillOnce(Return(true));
        EXPECT_TRUE(chassis.isEnabled());
    }

    // Test where works: false
    {
        chassis.initializeStatusMonitoring(services);
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
        EXPECT_STREQ(e.what(),
                     "Status monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeStatusMonitoring(services);
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
        chassis.initializeStatusMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isInputPowerGood())
            .Times(1)
            .WillOnce(Return(true));
        EXPECT_TRUE(chassis.isInputPowerGood());
    }

    // Test where works: false
    {
        chassis.initializeStatusMonitoring(services);
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
        EXPECT_STREQ(e.what(),
                     "Status monitoring not initialized for chassis 1");
    }

    // Test where fails: ChassisStatusMonitor throws an exception
    try
    {
        chassis.initializeStatusMonitoring(services);
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
        chassis.initializeStatusMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPowerSuppliesPowerGood())
            .Times(1)
            .WillOnce(Return(true));
        EXPECT_TRUE(chassis.isPowerSuppliesPowerGood());
    }

    // Test where works: false
    {
        chassis.initializeStatusMonitoring(services);
        auto& monitor = getMockStatusMonitor(chassis);
        EXPECT_CALL(monitor, isPowerSuppliesPowerGood())
            .Times(1)
            .WillOnce(Return(false));
        EXPECT_FALSE(chassis.isPowerSuppliesPowerGood());
    }
}
