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
#include "error_history.hpp"

#include <cstdint>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(ErrorHistoryTests, Constructor)
{
    ErrorHistory history{};
    EXPECT_EQ(history.count, 0);
    EXPECT_EQ(history.wasLogged, false);
}

TEST(ErrorHistoryTests, Clear)
{
    ErrorHistory history{};
    history.count = 23;
    history.wasLogged = true;
    history.clear();
    EXPECT_EQ(history.count, 0);
    EXPECT_EQ(history.wasLogged, false);
}

TEST(ErrorHistoryTests, IncrementCount)
{
    ErrorHistory history{};

    // Test where count is not near the max
    EXPECT_EQ(history.count, 0);
    history.incrementCount();
    EXPECT_EQ(history.count, 1);
    history.incrementCount();
    EXPECT_EQ(history.count, 2);

    // Test where count is near the max.  Verify it does not wrap/overflow.
    history.count = SIZE_MAX - 2;
    history.incrementCount();
    EXPECT_EQ(history.count, SIZE_MAX - 1);
    history.incrementCount();
    EXPECT_EQ(history.count, SIZE_MAX);
    history.incrementCount();
    EXPECT_EQ(history.count, SIZE_MAX);
}
