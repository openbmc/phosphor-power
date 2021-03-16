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
#include "write_verification_error.hpp"

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(WriteVerificationErrorTests, Constructor)
{
    WriteVerificationError error(
        "device: vdd1, register: 0x21, value_written: 0xAD, value_read: 0x00",
        "vdd1",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_EQ(error.getDeviceID(), "vdd1");
    EXPECT_EQ(error.getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_STREQ(error.what(),
                 "WriteVerificationError: device: vdd1, register: 0x21, "
                 "value_written: 0xAD, value_read: 0x00");
}

TEST(WriteVerificationErrorTests, GetDeviceID)
{
    WriteVerificationError error(
        "device: vdd1, register: 0x21, value_written: 0xAD, value_read: 0x00",
        "vdd1",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_EQ(error.getDeviceID(), "vdd1");
}

TEST(WriteVerificationErrorTests, GetInventoryPath)
{
    WriteVerificationError error(
        "device: vdd1, register: 0x21, value_written: 0xAD, value_read: 0x00",
        "vdd1",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_EQ(error.getInventoryPath(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
}

TEST(WriteVerificationErrorTests, What)
{
    WriteVerificationError error(
        "device: vdd1, register: 0x21, value_written: 0xAD, value_read: 0x00",
        "vdd1",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2");
    EXPECT_STREQ(error.what(),
                 "WriteVerificationError: device: vdd1, register: 0x21, "
                 "value_written: 0xAD, value_read: 0x00");
}
