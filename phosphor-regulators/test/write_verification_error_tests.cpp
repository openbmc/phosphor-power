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
    WriteVerificationError error("device: vdd1, register: 0x21, "
                                 "value_written: 0xAD, value_read: 0x00");
    EXPECT_STREQ(error.what(),
                 "WriteVerificationError: device: vdd1, register: 0x21, "
                 "value_written: 0xAD, value_read: 0x00");
}

TEST(WriteVerificationErrorTests, What)
{
    WriteVerificationError error("device: vdd2, register: 0x21, "
                                 "value_written: 0x32, value_read: 0x33");
    EXPECT_STREQ(error.what(),
                 "WriteVerificationError: device: vdd2, register: 0x21, "
                 "value_written: 0x32, value_read: 0x33");
}
