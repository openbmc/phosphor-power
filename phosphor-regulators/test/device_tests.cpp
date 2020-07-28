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
#include "journal.hpp"
#include "mock_action.hpp"
#include "mock_journal.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "system.hpp"
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

using ::testing::Return;
using ::testing::Throw;

TEST(DeviceTests, Constructor)
{
    // Test where only required parameters are specified
    {
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();
        Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                      std::move(i2cInterface)};
        EXPECT_EQ(device.getID(), "vdd_reg");
        EXPECT_EQ(device.isRegulator(), true);
        EXPECT_EQ(device.getFRU(), "/system/chassis/motherboard/reg2");
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
        Device device{"vdd_reg",
                      false,
                      "/system/chassis/motherboard/reg1",
                      std::move(i2cInterface),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(rails)};
        EXPECT_EQ(device.getID(), "vdd_reg");
        EXPECT_EQ(device.isRegulator(), false);
        EXPECT_EQ(device.getFRU(), "/system/chassis/motherboard/reg1");
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
    Device device{"vdd_reg",
                  false,
                  "/system/chassis/motherboard/reg2",
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

TEST(DeviceTests, Close)
{
    // Test where works: I2C interface is not open
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*i2cInterface, close).Times(0);

        // Create Device
        Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                      std::move(i2cInterface)};

        // Close Device
        journal::clear();
        device.close();
        EXPECT_EQ(journal::getErrMessages().size(), 0);
    }

    // Test where works: I2C interface is open
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, close).Times(1);

        // Create Device
        Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                      std::move(i2cInterface)};

        // Close Device
        journal::clear();
        device.close();
        EXPECT_EQ(journal::getErrMessages().size(), 0);
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

        // Create Device
        Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                      std::move(i2cInterface)};

        // Close Device
        journal::clear();
        device.close();
        std::vector<std::string> expectedErrMessages{
            "I2CException: Failed to close: bus /dev/i2c-1, addr 0x70",
            "Unable to close device vdd_reg"};
        EXPECT_EQ(journal::getErrMessages(), expectedErrMessages);
    }
}

TEST(DeviceTests, Configure)
{
    // Test where Configuration and Rails were not specified in constructor
    {
        // Create mock services.
        MockServices services{};

        // Create Device
        std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "reg1", true, "/system/chassis/motherboard/reg1",
            std::move(i2cInterface));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call configure().  Should do nothing.
        journal::clear();
        devicePtr->configure(services, system, *chassisPtr);
        EXPECT_EQ(journal::getDebugMessages().size(), 0);
        EXPECT_EQ(journal::getErrMessages().size(), 0);
    }

    // Test where Configuration and Rails were specified in constructor
    {
        // Create mock services.
        MockServices services{};

        std::vector<std::unique_ptr<Rail>> rails{};

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
            "reg1", true, "/system/chassis/motherboard/reg1",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration), std::move(rails));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Call configure().  For the Device and both Rails, should execute the
        // Configuration and log a debug message.
        journal::clear();
        devicePtr->configure(services, system, *chassisPtr);
        std::vector<std::string> expectedDebugMessages{
            "Configuring reg1", "Configuring vdd0: volts=1.300000",
            "Configuring vio0: volts=3.200000"};
        EXPECT_EQ(journal::getDebugMessages(), expectedDebugMessages);
        EXPECT_EQ(journal::getErrMessages().size(), 0);
    }
}

TEST(DeviceTests, GetConfiguration)
{
    // Test where Configuration was not specified in constructor
    {
        Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
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
        Device device{"vdd_reg",
                      true,
                      "/system/chassis/motherboard/reg2",
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
    Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                  std::move(createI2CInterface())};
    EXPECT_EQ(device.getFRU(), "/system/chassis/motherboard/reg2");
}

TEST(DeviceTests, GetI2CInterface)
{
    std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
    i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();
    Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                  std::move(i2cInterface)};
    EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
}

TEST(DeviceTests, GetID)
{
    Device device{"vdd_reg", false, "/system/chassis/motherboard/reg2",
                  std::move(createI2CInterface())};
    EXPECT_EQ(device.getID(), "vdd_reg");
}

TEST(DeviceTests, GetPresenceDetection)
{
    // Test where PresenceDetection was not specified in constructor
    {
        Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
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
        Device device{"vdd_reg", false, "/system/chassis/motherboard/reg2",
                      std::move(createI2CInterface()),
                      std::move(presenceDetection)};
        EXPECT_NE(device.getPresenceDetection(), nullptr);
        EXPECT_EQ(device.getPresenceDetection()->getActions().size(), 1);
    }
}

TEST(DeviceTests, GetRails)
{
    // Test where no rails were specified in constructor
    {
        Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
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
        Device device{"vdd_reg",
                      false,
                      "/system/chassis/motherboard/reg2",
                      std::move(createI2CInterface()),
                      std::move(presenceDetection),
                      std::move(configuration),
                      std::move(rails)};
        EXPECT_EQ(device.getRails().size(), 2);
        EXPECT_EQ(device.getRails()[0]->getID(), "vdd0");
        EXPECT_EQ(device.getRails()[1]->getID(), "vdd1");
    }
}

TEST(DeviceTests, IsRegulator)
{
    Device device{"vdd_reg", false, "/system/chassis/motherboard/reg2",
                  std::move(createI2CInterface())};
    EXPECT_EQ(device.isRegulator(), false);
}
