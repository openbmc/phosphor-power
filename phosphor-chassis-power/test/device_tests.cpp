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
#include "chassis.hpp"
#include "device.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::chassis;
/*using namespace phosphor::power::chassis::test_utils;*/

using ::testing::A;
using ::testing::Ref;
using ::testing::Return;
using ::testing::Throw;
using ::testing::TypedEq;

class DeviceTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the device object needed for calling Device methods.
     */
    DeviceTests() : ::testing::Test{} {}

  protected:
    Device device{"DeviceName", "DeviceDirection", "DevicePolarity"};
};

TEST_F(DeviceTests, getName)
{
    EXPECT_EQ(device.getName(), "DeviceName");
}

TEST_F(DeviceTests, getDirection)
{
    EXPECT_EQ(device.getDirection(), "DeviceDirection");
}

TEST_F(DeviceTests, getPolarity)
{
    EXPECT_EQ(device.getPolarity(), "DevicePolarity");
}
