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
#include "action_environment.hpp"
#include "action_error.hpp"
#include "compare_vpd_action.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "id_map.hpp"
#include "mock_action.hpp"
#include "mock_services.hpp"
#include "mock_vpd_service.hpp"
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

TEST(CompareVPDActionTests, Constructor)
{
    CompareVPDAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        "2D35"};
    EXPECT_EQ(action.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
    EXPECT_EQ(action.getKeyword(), "CCIN");
    EXPECT_EQ(action.getValue(), "2D35");
}

TEST(CompareVPDActionTests, Execute)
{
    // Test where works: actual keyword value is "2D35".
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();

        // Create Device, IDMap, MockServices and ActionEnvironment
        // Actual keyword value getValue() is "2D35"
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        MockVPDService& vpdService = services.getMockVPDService();
        EXPECT_CALL(
            vpdService,
            getValue(
                "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
                "CCIN"))
            .Times(2)
            .WillRepeatedly(Return("2D35"));
        ActionEnvironment env{idMap, "reg1", services};

        // Test where works: expected value is 2D35, return value is true.
        {
            CompareVPDAction action{
                "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
                "CCIN", "2D35"};
            EXPECT_EQ(action.execute(env), true);
        }

        // Test where works: expected value is VALUE, return value is false.
        {
            CompareVPDAction action{
                "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
                "CCIN", "VALUE"};
            EXPECT_EQ(action.execute(env), false);
        }
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails. Reading value fails.
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();

        // Create Device, IDMap, MockServices and ActionEnvironment
        // vpdService cannot get the value
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        MockVPDService& vpdService = services.getMockVPDService();
        EXPECT_CALL(
            vpdService,
            getValue(
                "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
                "CCIN"))
            .Times(1)
            .WillOnce(
                Throw(std::runtime_error("vpdService cannot get the value.")));

        ActionEnvironment env{idMap, "reg1", services};

        CompareVPDAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", "2D35"};
        EXPECT_EQ(action.execute(env), false);
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(), "ActionError: compare_vpd: { fru: "
                               "/xyz/openbmc_project/inventory/system/chassis/"
                               "disk_backplane, keyword: CCIN, value: 2D35 }");
        try
        {
            // Re-throw inner exception from MockVPDService.
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& r_error)
        {
            EXPECT_STREQ(r_error.what(), "vpdService cannot get the value.");
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

TEST(CompareVPDActionTests, GetFRU)
{
    CompareVPDAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        "2D35"};
    EXPECT_EQ(action.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
}

TEST(CompareVPDActionTests, GetKeyword)
{
    CompareVPDAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        "2D35"};
    EXPECT_EQ(action.getKeyword(), "CCIN");
}

TEST(CompareVPDActionTests, GetValue)
{
    CompareVPDAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        "2D35"};
    EXPECT_EQ(action.getValue(), "2D35");
}

TEST(CompareVPDActionTests, ToString)
{
    CompareVPDAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        "2D35"};
    EXPECT_EQ(action.toString(), "compare_vpd: { fru: "
                                 "/xyz/openbmc_project/inventory/system/"
                                 "chassis/disk_backplane, keyword: "
                                 "CCIN, value: 2D35 }");
}
