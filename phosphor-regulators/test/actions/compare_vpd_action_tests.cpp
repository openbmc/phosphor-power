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
#include "compare_vpd_action.hpp"
#include "id_map.hpp"

#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

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
    // TODO: Not implemented yet
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
