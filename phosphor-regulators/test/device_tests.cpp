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
#include "device.hpp"

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(DeviceTests, Constructor)
{
    Device device("vdd_vrm2");
    EXPECT_EQ(device.getID(), "vdd_vrm2");
}

TEST(DeviceTests, GetID)
{
    Device device("vio_vrm");
    EXPECT_EQ(device.getID(), "vio_vrm");
}
