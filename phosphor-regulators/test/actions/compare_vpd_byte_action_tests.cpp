/**
 * Copyright © 2026 IBM Corporation
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
#include "compare_vpd_byte_action.hpp"
#include "id_map.hpp"
#include "mock_services.hpp"
#include "mock_vpd.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::Throw;

using namespace phosphor::power::regulators;

TEST(CompareVPDByteActionTests, Constructor)
{
    // Test where mask is not specified
    {
        CompareVPDByteAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", 0, 0x32};
        EXPECT_EQ(
            action.getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action.getKeyword(), "CCIN");
        EXPECT_EQ(action.getPosition(), 0);
        EXPECT_EQ(action.getValue(), 0x32);
        EXPECT_EQ(action.getMask(), 0xFF);
    }

    // Test where mask is specified
    {
        CompareVPDByteAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "HW", 1, 0x02, 0x7F};
        EXPECT_EQ(
            action.getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action.getKeyword(), "HW");
        EXPECT_EQ(action.getPosition(), 1);
        EXPECT_EQ(action.getValue(), 0x02);
        EXPECT_EQ(action.getMask(), 0x7F);
    }
}

TEST(CompareVPDByteActionTests, Execute)
{
    // Test where works: Equal: Position 0: No mask
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"Model"};
        std::vector<uint8_t> actualValue{0x41, 0x42, 0x43, 0x44};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        CompareVPDByteAction action{fru, keyword, 0, 0x41};
        EXPECT_TRUE(action.execute(environment));
    }

    // Test where works: Equal: Position > 0: Mask
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"HW"};
        std::vector<uint8_t> actualValue{0x00, 0x12, 0xD7};
        // Actual byte at position 2: 0xD7 = 1101 0111
        // Mask: 0x7E = 0111 1110
        // Masked actual: 0x56 = 0101 0110
        // Expected value: 0x56 = 0101 0110

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        CompareVPDByteAction action{fru, keyword, 2, 0x56, 0x7E};
        EXPECT_TRUE(action.execute(environment));
    }

    // Test where works: Not equal: Position 0: Mask
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"Model"};
        std::vector<uint8_t> actualValue{0xD7, 0x00, 0x12};
        // Actual byte at position 0: 0xD7 = 1101 0111
        // Mask: 0x7E = 0111 1110
        // Masked actual: 0x56 = 0101 0110
        // Expected value: 0x16 = 0001 0110

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        CompareVPDByteAction action{fru, keyword, 0, 0x16, 0x7E};
        EXPECT_FALSE(action.execute(environment));
    }

    // Test where works: Not equal: Position > 0: No mask
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"HW"};
        std::vector<uint8_t> actualValue{0x00, 0x12, 0xD7};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        CompareVPDByteAction action{fru, keyword, 1, 0x11};
        EXPECT_FALSE(action.execute(environment));
    }

    // Test where works: Not equal: Position past end of actual byte array
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"HW"};
        std::vector<uint8_t> actualValue{0x41, 0x42};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        CompareVPDByteAction action{fru, keyword, 2, 0x42};
        EXPECT_FALSE(action.execute(environment));
    }

    // Test where works: Not equal: Actual byte array is empty
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"HW"};
        std::vector<uint8_t> actualValue{};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        CompareVPDByteAction action{fru, keyword, 0, 0x41};
        EXPECT_FALSE(action.execute(environment));
    }

    // Test where fails: Exception thrown when trying to get actual VPD value
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"Model"};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(
                Throw(std::runtime_error{"D-Bus error: Invalid object path"}));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        try
        {
            CompareVPDByteAction action{fru, keyword, 0, 0x41};
            action.execute(environment);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const ActionError& e)
        {
            EXPECT_STREQ(e.what(), "ActionError: compare_vpd_byte: { fru: "
                                   "/xyz/openbmc_project/inventory/system, "
                                   "keyword: Model, position: 0, value: 0x41, "
                                   "mask: 0xFF }");
            try
            {
                // Re-throw inner exception
                std::rethrow_if_nested(e);
                ADD_FAILURE() << "Should not have reached this line.";
            }
            catch (const std::runtime_error& re)
            {
                EXPECT_STREQ(re.what(), "D-Bus error: Invalid object path");
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
}

TEST(CompareVPDByteActionTests, GetFRU)
{
    CompareVPDByteAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        0, 0x32};
    EXPECT_EQ(action.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
}

TEST(CompareVPDByteActionTests, GetKeyword)
{
    CompareVPDByteAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        0, 0x32};
    EXPECT_EQ(action.getKeyword(), "CCIN");
}

TEST(CompareVPDByteActionTests, GetPosition)
{
    CompareVPDByteAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        1, 0x32};
    EXPECT_EQ(action.getPosition(), 1);
}

TEST(CompareVPDByteActionTests, GetValue)
{
    CompareVPDByteAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        0, 0x32};
    EXPECT_EQ(action.getValue(), 0x32);
}

TEST(CompareVPDByteActionTests, GetMask)
{
    // Test where mask was not specified (default 0xFF)
    {
        CompareVPDByteAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", 0, 0x32};
        EXPECT_EQ(action.getMask(), 0xFF);
    }

    // Test where mask was specified
    {
        CompareVPDByteAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", 1, 0x32, 0x7E};
        EXPECT_EQ(action.getMask(), 0x7E);
    }
}

TEST(CompareVPDByteActionTests, ToString)
{
    CompareVPDByteAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "HW", 1,
        0x02, 0x7F};
    EXPECT_EQ(action.toString(),
              "compare_vpd_byte: { fru: "
              "/xyz/openbmc_project/inventory/system/"
              "chassis/disk_backplane, keyword: HW, "
              "position: 1, value: 0x2, mask: 0x7F }");
}
