/**
 * Copyright © 2019 IBM Corporation
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
#include "../version.hpp"

#include <gtest/gtest.h>

TEST(Version, GetLatest)
{
    // Input 2 different version where primary versions are different
    std::vector<std::string> input = {"00000110", "01100110"};
    EXPECT_EQ("01100110", version::getLatest(input));

    // Input 3 different version where secondary versions are different
    input = {"11223366", "11223355", "11223344"};
    EXPECT_EQ("11223366", version::getLatest(input));

    // Input has 3 same versions
    input = {"11112222", "11112222", "11112222"};
    EXPECT_EQ("11112222", version::getLatest(input));

    // Input has one version
    input = {"11112222"};
    EXPECT_EQ("11112222", version::getLatest(input));

    // Input empty
    input = {};
    EXPECT_EQ("", version::getLatest(input));
}
