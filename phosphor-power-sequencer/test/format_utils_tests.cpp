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

#include "format_utils.hpp"

#include <array>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::sequencer::format_utils;

TEST(FormatUtilsTests, toString)
{
    // Test with spans from integer vector
    {
        std::vector<int> vec{1, 3, 5, 7, 9, 11};
        std::span<int> spn{vec};
        EXPECT_EQ(toString(spn), "[1, 3, 5, 7, 9, 11]");
        EXPECT_EQ(toString(spn.subspan(0, 4)), "[1, 3, 5, 7]");
        EXPECT_EQ(toString(spn.subspan(3, 3)), "[7, 9, 11]");
        EXPECT_EQ(toString(spn.subspan(5, 1)), "[11]");
        EXPECT_EQ(toString(spn.subspan(0, 0)), "[]");
    }

    // Test with spans from double vector
    {
        std::vector<double> vec{2.1, -3.9, 21.03};
        std::span<double> spn{vec};
        EXPECT_EQ(toString(spn), "[2.1, -3.9, 21.03]");
    }

    // Test with span from empty vector
    {
        std::vector<int> vec{};
        std::span<int> spn{vec};
        EXPECT_EQ(toString(spn), "[]");
    }

    // Test with spans from string array
    {
        std::array<std::string, 3> ary{"cow", "horse", "zebra"};
        std::span<std::string> spn{ary};
        EXPECT_EQ(toString(spn), "[cow, horse, zebra]");
        EXPECT_EQ(toString(spn.subspan(1, 2)), "[horse, zebra]");
    }
}
