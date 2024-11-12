/**
 * Copyright © 2024 IBM Corporation
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
#include "../utils.hpp"

#include <stdexcept>
#include <tuple>

#include <gtest/gtest.h>

using namespace utils;

TEST(TestUtils, getDeviceName)
{
    auto ret = getDeviceName("");
    EXPECT_TRUE(ret.empty());

    ret = getDeviceName("/sys/bus/i2c/devices/3-0069");
    EXPECT_EQ("3-0069", ret);

    ret = getDeviceName("/sys/bus/i2c/devices/3-0069/");
    EXPECT_EQ("3-0069", ret);
}

TEST(TestUtils, parseDeviceName)
{
    auto [id, addr] = parseDeviceName("3-0068");
    EXPECT_EQ(3, id);
    EXPECT_EQ(0x68, addr);

    std::tie(id, addr) = parseDeviceName("11-0069");
    EXPECT_EQ(11, id);
    EXPECT_EQ(0x69, addr);

    EXPECT_THROW(parseDeviceName("no-number"), std::invalid_argument);

    EXPECT_DEATH(parseDeviceName("invalid"), "");
}
