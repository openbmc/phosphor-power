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
#include "action_environment.hpp"
#include "device.hpp"
#include "id_map.hpp"
#include "set_device_action.hpp"

#include <exception>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(SetDeviceActionTests, Constructor)
{
    SetDeviceAction action{"regulator1"};
    EXPECT_EQ(action.getDeviceID(), "regulator1");
}

TEST(SetDeviceActionTests, Execute)
{
    // Create IDMap
    IDMap idMap{};
    Device reg1{"regulator1"};
    idMap.addDevice(reg1);
    Device reg2{"regulator2"};
    idMap.addDevice(reg2);

    // Create ActionEnvironment
    ActionEnvironment env{idMap, "regulator1"};

    // Create action
    SetDeviceAction action{"regulator2"};

    // Execute action
    try
    {
        EXPECT_EQ(env.getDeviceID(), "regulator1");
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(env.getDeviceID(), "regulator2");
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(SetDeviceActionTests, GetDeviceID)
{
    SetDeviceAction action{"io_expander_0"};
    EXPECT_EQ(action.getDeviceID(), "io_expander_0");
}
