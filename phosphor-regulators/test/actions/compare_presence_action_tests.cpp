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
#include "action.hpp"
#include "action_environment.hpp"
#include "compare_presence_action.hpp"
#include "id_map.hpp"
#include "mock_action.hpp"

#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(ComparePresenceActionTests, Constructor)
{
    ComparePresenceAction action{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu3", true};
    EXPECT_EQ(action.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu3");
    EXPECT_EQ(action.getValue(), true);
}

TEST(ComparePresenceActionTests, Execute)
{
    // TODO: Not implemented yet
}

TEST(ComparePresenceActionTests, GetFRU)
{
    ComparePresenceAction action{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2", true};
    EXPECT_EQ(action.getFRU(),
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2");
}

TEST(ComparePresenceActionTests, GetValue)
{
    ComparePresenceAction action{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu3",
        false};
    EXPECT_EQ(action.getValue(), false);
}

TEST(ComparePresenceActionTests, ToString)
{
    ComparePresenceAction action{
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2", true};
    EXPECT_EQ(action.toString(),
              "compare_presence: { fru: "
              "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2, "
              "value: true }");
}
