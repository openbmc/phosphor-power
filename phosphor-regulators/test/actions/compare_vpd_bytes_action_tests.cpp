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
#include "compare_vpd_bytes_action.hpp"
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

// Test for CompareVPDBytesAction(fru, keyword, values)
TEST(CompareVPDBytesActionTests, Constructor1)
{
    // Test where works: non-empty values vector
    {
        std::vector<uint8_t> values{0x32, 0x44, 0x33, 0x35};
        CompareVPDBytesAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", values};
        EXPECT_EQ(
            action.getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action.getKeyword(), "CCIN");
        EXPECT_EQ(action.getValues().size(), 4);
        EXPECT_EQ(action.getValues()[0], 0x32);
        EXPECT_EQ(action.getValues()[1], 0x44);
        EXPECT_EQ(action.getValues()[2], 0x33);
        EXPECT_EQ(action.getValues()[3], 0x35);
        EXPECT_EQ(action.getMasks().size(), 4);
        EXPECT_EQ(action.getMasks()[0], 0xFF);
        EXPECT_EQ(action.getMasks()[1], 0xFF);
        EXPECT_EQ(action.getMasks()[2], 0xFF);
        EXPECT_EQ(action.getMasks()[3], 0xFF);
    }

    // Test where works: empty values vector
    {
        std::vector<uint8_t> values{};
        CompareVPDBytesAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", values};
        EXPECT_EQ(
            action.getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action.getKeyword(), "CCIN");
        EXPECT_EQ(action.getValues().size(), 0);
        EXPECT_EQ(action.getMasks().size(), 0);
    }
}

// Test for CompareVPDBytesAction(fru, keyword, values, masks)
TEST(CompareVPDBytesActionTests, Constructor2)
{
    // Test where works: non-empty values vector
    {
        std::vector<uint8_t> values{0x56, 0x14};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        CompareVPDBytesAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "Model", values, masks};
        EXPECT_EQ(
            action.getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action.getKeyword(), "Model");
        EXPECT_EQ(action.getValues().size(), 2);
        EXPECT_EQ(action.getValues()[0], 0x56);
        EXPECT_EQ(action.getValues()[1], 0x14);
        EXPECT_EQ(action.getMasks().size(), 2);
        EXPECT_EQ(action.getMasks()[0], 0x7E);
        EXPECT_EQ(action.getMasks()[1], 0x3C);
    }

    // Test where works: empty values vector
    {
        std::vector<uint8_t> values{};
        std::vector<uint8_t> masks{};
        CompareVPDBytesAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", values, masks};
        EXPECT_EQ(
            action.getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action.getKeyword(), "CCIN");
        EXPECT_EQ(action.getValues().size(), 0);
        EXPECT_EQ(action.getMasks().size(), 0);
    }

    // Test where fails: Masks vector different size than values vector
    try
    {
        std::vector<uint8_t> values{0x56, 0x14, 0xFE};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        CompareVPDBytesAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", values, masks};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Masks vector has invalid size");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(CompareVPDBytesActionTests, Execute)
{
    // Test where works: Equal: Empty values vector
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"Model"};
        std::vector<uint8_t> actualValue{};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        std::vector<uint8_t> values{};
        CompareVPDBytesAction action{fru, keyword, values};
        EXPECT_TRUE(action.execute(environment));
    }

    // Test where works: Equal: No masks
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

        std::vector<uint8_t> values{0x41, 0x42, 0x43, 0x44};
        CompareVPDBytesAction action{fru, keyword, values};
        EXPECT_TRUE(action.execute(environment));
    }

    // Test where works: Equal: With masks
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"Model"};
        // Actual: 0xD7 = 1101 0111, 0x96 = 1001 0110
        std::vector<uint8_t> actualValue{0xD7, 0x96};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        // Masks: 0x7E = 0111 1110, 0x3C = 0011 1100
        // Results: 0x56 = 0101 0110, 0x14 = 0001 0100
        std::vector<uint8_t> values{0x56, 0x14};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        CompareVPDBytesAction action{fru, keyword, values, masks};
        EXPECT_TRUE(action.execute(environment));
    }

    // Test where works: Not equal: actual value != expected value
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

        std::vector<uint8_t> values{0x41, 0x42, 0x43, 0x45};
        CompareVPDBytesAction action{fru, keyword, values};
        EXPECT_FALSE(action.execute(environment));
    }

    // Test where works: Not equal: actual value has more bytes than expected
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"HW"};
        std::vector<uint8_t> actualValue{0x41, 0x42, 0x43, 0x44};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        std::vector<uint8_t> values{0x41, 0x42, 0x43};
        CompareVPDBytesAction action{fru, keyword, values};
        EXPECT_FALSE(action.execute(environment));
    }

    // Test where works: Not equal: actual value has fewer bytes than expected
    {
        std::string fru{"/xyz/openbmc_project/inventory/system"};
        std::string keyword{"HW"};
        std::vector<uint8_t> actualValue{0x41};

        MockServices services{};
        MockVPD& vpd = services.getMockVPD();
        EXPECT_CALL(vpd, getValue(fru, keyword))
            .Times(1)
            .WillOnce(Return(actualValue));

        IDMap idMap{};
        ActionEnvironment environment{idMap, "", services};

        std::vector<uint8_t> values{0x41, 0x42};
        CompareVPDBytesAction action{fru, keyword, values};
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
            std::vector<uint8_t> values{0x41, 0x42};
            CompareVPDBytesAction action{fru, keyword, values};
            action.execute(environment);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const ActionError& e)
        {
            EXPECT_STREQ(e.what(), "ActionError: compare_vpd_bytes: { fru: "
                                   "/xyz/openbmc_project/inventory/system, "
                                   "keyword: Model, values: [ 0x41, 0x42 ], "
                                   "masks: [ 0xFF, 0xFF ] }");
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

TEST(CompareVPDBytesActionTests, GetFRU)
{
    std::vector<uint8_t> values{0x32, 0x44, 0x33, 0x35};
    CompareVPDBytesAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        values};
    EXPECT_EQ(action.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
}

TEST(CompareVPDBytesActionTests, GetKeyword)
{
    std::vector<uint8_t> values{0x32, 0x44, 0x33, 0x35};
    CompareVPDBytesAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        values};
    EXPECT_EQ(action.getKeyword(), "CCIN");
}

TEST(CompareVPDBytesActionTests, GetValues)
{
    std::vector<uint8_t> values{0x56, 0x14};
    std::vector<uint8_t> masks{0x7E, 0x3C};
    CompareVPDBytesAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        values, masks};
    EXPECT_EQ(action.getValues().size(), 2);
    EXPECT_EQ(action.getValues()[0], 0x56);
    EXPECT_EQ(action.getValues()[1], 0x14);
}

TEST(CompareVPDBytesActionTests, GetMasks)
{
    // Test where masks were not specified (default 0xFF)
    {
        std::vector<uint8_t> values{0x56, 0x14, 0xDA};
        CompareVPDBytesAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", values};
        EXPECT_EQ(action.getMasks().size(), 3);
        EXPECT_EQ(action.getMasks()[0], 0xFF);
        EXPECT_EQ(action.getMasks()[1], 0xFF);
        EXPECT_EQ(action.getMasks()[2], 0xFF);
    }

    // Test where masks were specified
    {
        std::vector<uint8_t> values{0x56, 0x14};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        CompareVPDBytesAction action{
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane",
            "CCIN", values, masks};
        EXPECT_EQ(action.getMasks().size(), 2);
        EXPECT_EQ(action.getMasks()[0], 0x7E);
        EXPECT_EQ(action.getMasks()[1], 0x3C);
    }
}

TEST(CompareVPDBytesActionTests, ToString)
{
    std::vector<uint8_t> values{0x56, 0x14};
    std::vector<uint8_t> masks{0x7E, 0x3C};
    CompareVPDBytesAction action{
        "/xyz/openbmc_project/inventory/system/chassis/disk_backplane", "CCIN",
        values, masks};
    EXPECT_EQ(action.toString(),
              "compare_vpd_bytes: { fru: "
              "/xyz/openbmc_project/inventory/system/"
              "chassis/disk_backplane, keyword: CCIN, "
              "values: [ 0x56, 0x14 ], masks: [ 0x7E, 0x3C ] }");
}
