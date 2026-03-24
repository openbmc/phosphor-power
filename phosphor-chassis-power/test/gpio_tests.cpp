/**
 * Copyright © 2026 IBM Corporation
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
#include "gpio.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::chassis;
/*using namespace phosphor::power::chassis::test_utils;*/

class GpioTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the GPIO object needed for calling Gpio methods.
     */
    GpioTests() : ::testing::Test{} {}

  protected:
};

TEST_F(GpioTests, GpioInputLow)
{
    Gpio gpio{"GpioName", GpioDirection::Input, GpioPolarity::Low};
    EXPECT_EQ(gpio.getName(), "GpioName");
    EXPECT_EQ(gpio.getDirection(), GpioDirection::Input);
    EXPECT_EQ(gpio.getPolarity(), GpioPolarity::Low);
}

TEST_F(GpioTests, GpioOutputHigh)
{
    Gpio gpio{"GpioName", GpioDirection::Output, GpioPolarity::High};
    EXPECT_EQ(gpio.getName(), "GpioName");
    EXPECT_EQ(gpio.getDirection(), GpioDirection::Output);
    EXPECT_EQ(gpio.getPolarity(), GpioPolarity::High);
}
