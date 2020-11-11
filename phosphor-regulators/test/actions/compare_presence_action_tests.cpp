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
#include "action_environment.hpp"
#include "action_error.hpp"
#include "compare_presence_action.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "id_map.hpp"
#include "mock_action.hpp"
#include "mock_presence_service.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"

#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::Return;
using ::testing::Throw;

TEST(ComparePresenceActionTests, Constructor)
{
    ComparePresenceAction action{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu3", true};
    EXPECT_EQ(action.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu3");
    EXPECT_EQ(action.getValue(), true);
}

TEST(ComparePresenceActionTests, Execute)
{
    // Test where works: actual value is true.
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();

        // Create Device, IDMap, MockServices and ActionEnvironment
        // Actual value isPresent() is true
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService,
                    isPresent("/xyz/openbmc_project/inventory/system/chassis/"
                              "motherboard/cpu2"))
            .Times(2)
            .WillRepeatedly(Return(true));
        ActionEnvironment env{idMap, "reg1", services};

        // Test where works: expected value is true, return value is true.
        {
            ComparePresenceAction action{"/xyz/openbmc_project/inventory/"
                                         "system/chassis/motherboard/cpu2",
                                         true};
            EXPECT_EQ(action.execute(env), true);
        }

        // Test where works: expected value is false, return value is false.
        {
            ComparePresenceAction action{"/xyz/openbmc_project/inventory/"
                                         "system/chassis/motherboard/cpu2",
                                         false};
            EXPECT_EQ(action.execute(env), false);
        }
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where actual value is false.
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();

        // Create Device, IDMap, MockServices and ActionEnvironment
        // Actual value isPresent() is false
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService,
                    isPresent("/xyz/openbmc_project/inventory/system/chassis/"
                              "motherboard/cpu2"))
            .Times(2)
            .WillRepeatedly(Return(false));
        ActionEnvironment env{idMap, "reg1", services};

        // Test where works: expected value is true, return value is false.
        {
            ComparePresenceAction action{"/xyz/openbmc_project/inventory/"
                                         "system/chassis/motherboard/cpu2",
                                         true};
            EXPECT_EQ(action.execute(env), false);
        }

        // Test where works: expected value is false, return value is true.
        {
            ComparePresenceAction action{"/xyz/openbmc_project/inventory/"
                                         "system/chassis/motherboard/cpu2",
                                         false};
            EXPECT_EQ(action.execute(env), true);
        }
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails. Reading presence fails.
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();

        // Create Device, IDMap, MockServices and ActionEnvironment
        // PresenceService cannot get the presence
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService,
                    isPresent("/xyz/openbmc_project/inventory/system/chassis/"
                              "motherboard/cpu2"))
            .Times(1)
            .WillOnce(Throw(std::runtime_error(
                "PresenceService cannot get the presence.")));

        ActionEnvironment env{idMap, "reg1", services};

        ComparePresenceAction action{"/xyz/openbmc_project/inventory/"
                                     "system/chassis/motherboard/cpu2",
                                     true};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(), "ActionError: compare_presence: { fru: "
                               "/xyz/openbmc_project/inventory/system/chassis/"
                               "motherboard/cpu2, value: true }");
        try
        {
            // Re-throw inner exception from MockPresenceService.
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& r_error)
        {
            EXPECT_STREQ(r_error.what(),
                         "PresenceService cannot get the presence.");
        }
        catch (...)
        {
            ADD_FAILURE() << "Should not have caught exception.";
        }
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(ComparePresenceActionTests, GetFRU)
{
    ComparePresenceAction action{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2", true};
    EXPECT_EQ(action.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2");
}

TEST(ComparePresenceActionTests, GetValue)
{
    ComparePresenceAction action{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu3",
        false};
    EXPECT_EQ(action.getValue(), false);
}

TEST(ComparePresenceActionTests, ToString)
{
    ComparePresenceAction action{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2", true};
    EXPECT_EQ(action.toString(),
              "compare_presence: { fru: "
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2, "
              "value: true }");
}
