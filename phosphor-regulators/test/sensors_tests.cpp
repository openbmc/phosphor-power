/**
 * Copyright © 2021 IBM Corporation
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
#include "sensors.hpp"

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::sensors;

TEST(SensorsTests, ToString)
{
    EXPECT_EQ(toString(SensorType::iout), "iout");
    EXPECT_EQ(toString(SensorType::iout_peak), "iout_peak");
    EXPECT_EQ(toString(SensorType::iout_valley), "iout_valley");
    EXPECT_EQ(toString(SensorType::pout), "pout");
    EXPECT_EQ(toString(SensorType::temperature), "temperature");
    EXPECT_EQ(toString(SensorType::temperature_peak), "temperature_peak");
    EXPECT_EQ(toString(SensorType::vout), "vout");
    EXPECT_EQ(toString(SensorType::vout_peak), "vout_peak");
    EXPECT_EQ(toString(SensorType::vout_valley), "vout_valley");
}
