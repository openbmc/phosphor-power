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

TEST_F(GpioTests, Constructor)
{
    // Test where GPIO is Input direction and Low polarity, no default value
    {
        BMCGpio gpio{"GpioName", GpioDirection::Input, GpioPolarity::Low};
        EXPECT_EQ(gpio.getName(), "GpioName");
        EXPECT_EQ(gpio.getDirection(), GpioDirection::Input);
        EXPECT_EQ(gpio.getPolarity(), GpioPolarity::Low);
        // Verify getDefaultValue() returns std::nullopt when no default is
        // provided
        EXPECT_FALSE(gpio.getDefaultValue().has_value());
    }

    // Test where GPIO is Output direction and High polarity, no default value
    {
        BMCGpio gpio{"GpioName", GpioDirection::Output, GpioPolarity::High};
        EXPECT_EQ(gpio.getName(), "GpioName");
        EXPECT_EQ(gpio.getDirection(), GpioDirection::Output);
        EXPECT_EQ(gpio.getPolarity(), GpioPolarity::High);
        // Verify getDefaultValue() returns std::nullopt when no default is
        // provided
        EXPECT_FALSE(gpio.getDefaultValue().has_value());
    }

    // Test where default value Low
    {
        BMCGpio gpio{"GpioName", GpioDirection::Input, GpioPolarity::High, 0};
        EXPECT_EQ(gpio.getName(), "GpioName");
        EXPECT_EQ(gpio.getDirection(), GpioDirection::Input);
        EXPECT_EQ(gpio.getPolarity(), GpioPolarity::High);
        // Verify getDefaultValue() returns the provided default value (Low)
        EXPECT_TRUE(gpio.getDefaultValue().has_value());
        EXPECT_EQ(gpio.getDefaultValue().value(), 0);
    }

    // Test where default value High
    {
        BMCGpio gpio{"GpioName", GpioDirection::Output, GpioPolarity::Low, 1};
        EXPECT_EQ(gpio.getName(), "GpioName");
        EXPECT_EQ(gpio.getDirection(), GpioDirection::Output);
        EXPECT_EQ(gpio.getPolarity(), GpioPolarity::Low);
        // Verify getDefaultValue() returns the provided default value (High)
        EXPECT_TRUE(gpio.getDefaultValue().has_value());
        EXPECT_EQ(gpio.getDefaultValue().value(), 1);
    }

    // Test where default value with explicit std::nullopt
    {
        BMCGpio gpio{"GpioName", GpioDirection::Input, GpioPolarity::Low,
                     std::nullopt};
        EXPECT_EQ(gpio.getName(), "GpioName");
        EXPECT_EQ(gpio.getDirection(), GpioDirection::Input);
        EXPECT_EQ(gpio.getPolarity(), GpioPolarity::Low);
        // Verify getDefaultValue() returns std::nullopt when explicitly passed
        EXPECT_FALSE(gpio.getDefaultValue().has_value());
    }
}
