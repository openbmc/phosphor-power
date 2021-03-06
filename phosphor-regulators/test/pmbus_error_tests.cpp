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
#include "pmbus_error.hpp"

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(PMBusErrorTests, Constructor)
{
    PMBusError error(
        "VOUT_MODE contains unsupported data format", "vdd_reg",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_EQ(error.getDeviceID(), "vdd_reg");
    EXPECT_EQ(error.getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_STREQ(error.what(),
                 "PMBusError: VOUT_MODE contains unsupported data format");
}

TEST(PMBusErrorTests, GetDeviceID)
{
    PMBusError error(
        "VOUT_MODE contains unsupported data format", "vdd_reg",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_EQ(error.getDeviceID(), "vdd_reg");
}

TEST(PMBusErrorTests, GetInventoryPath)
{
    PMBusError error(
        "VOUT_MODE contains unsupported data format", "vdd_reg",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_EQ(error.getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
}

TEST(PMBusErrorTests, What)
{
    PMBusError error(
        "VOUT_MODE contains unsupported data format", "vdd_reg",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_STREQ(error.what(),
                 "PMBusError: VOUT_MODE contains unsupported data format");
}
