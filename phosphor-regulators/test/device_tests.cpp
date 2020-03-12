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
#include "configuration.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "mock_action.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "test_utils.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::test_utils;

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
