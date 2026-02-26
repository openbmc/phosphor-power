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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::chassis;
/*using namespace phosphor::power::chassis::test_utils;*/

using ::testing::A;
using ::testing::Ref;
using ::testing::Return;
using ::testing::Throw;
using ::testing::TypedEq;

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
    Gpio gpio{"GpioName", parseDirection("Input"), parsePolarity("Low")};
    EXPECT_EQ(gpio.getName(), "GpioName");
    EXPECT_EQ(gpio.getDirection(), gpioDirection::Input);
    EXPECT_EQ(gpio.getPolarity(), gpioPolarity::Low);
}

TEST_F(GpioTests, GpioOutputHigh)
{
    Gpio gpio{"GpioName", parseDirection("Output"), parsePolarity("High")};
    EXPECT_EQ(gpio.getName(), "GpioName");
    EXPECT_EQ(gpio.getDirection(), gpioDirection::Output);
    EXPECT_EQ(gpio.getPolarity(), gpioPolarity::High);
}

TEST_F(GpioTests, GpioInvalidDirection)
{
    try
    {
        Gpio gpio{"GpioName", parseDirection("NoInput"), parsePolarity("High")};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid direction value: NoInput");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST_F(GpioTests, GpioInvalidPolarity)
{
    try
    {
        Gpio gpio{"GpioName", parseDirection("Output"),
                  parsePolarity("NoPolarity")};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid polarity value: NoPolarity");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}
