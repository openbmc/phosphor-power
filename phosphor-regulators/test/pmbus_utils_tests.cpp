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
#include "pmbus_utils.hpp"

#include <cstdint>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(PMBusUtilsTests, ParseVoutMode)
{
    uint8_t voutModeValue;
    pmbus_utils::VoutDataFormat format;
    int8_t parameter;

    // Linear format: Exponent is negative: 0b1'1111
    voutModeValue = 0b0001'1111u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::linear);
    EXPECT_EQ(parameter, -1);

    // Linear format: Exponent is negative: 0b1'0000
    voutModeValue = 0b1001'0000u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::linear);
    EXPECT_EQ(parameter, -16);

    // Linear format: Exponent is positive: 0b0'1111
    voutModeValue = 0b1000'1111u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::linear);
    EXPECT_EQ(parameter, 15);

    // Linear format: Exponent is positive: 0b0'0001
    voutModeValue = 0b0000'0001u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::linear);
    EXPECT_EQ(parameter, 1);

    // Linear format: Exponent is zero: 0b0'0000
    voutModeValue = 0b0000'0000u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::linear);
    EXPECT_EQ(parameter, 0);

    // VID format: VID code is 0b1'1111
    voutModeValue = 0b0011'1111u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::vid);
    EXPECT_EQ(parameter, 31);

    // VID format: VID code is 0b1'0000
    voutModeValue = 0b1011'0000u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::vid);
    EXPECT_EQ(parameter, 16);

    // VID format: VID code is 0b0'1111
    voutModeValue = 0b1010'1111u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::vid);
    EXPECT_EQ(parameter, 15);

    // VID format: VID code is 0b0'0001
    voutModeValue = 0b0010'0001u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::vid);
    EXPECT_EQ(parameter, 1);

    // VID format: VID code is 0b0'0000
    voutModeValue = 0b1010'0000u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::vid);
    EXPECT_EQ(parameter, 0);

    // Direct format
    voutModeValue = 0b1100'0000u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::direct);
    EXPECT_EQ(parameter, 0);

    // IEEE format
    voutModeValue = 0b0110'0000u;
    pmbus_utils::parseVoutMode(voutModeValue, format, parameter);
    EXPECT_EQ(format, pmbus_utils::VoutDataFormat::ieee);
    EXPECT_EQ(parameter, 0);
}

TEST(PMBusUtilsTests, toString)
{
    // Sensor data format: SensorDataFormat::linear_11
    {
        pmbus_utils::SensorDataFormat format =
            pmbus_utils::SensorDataFormat::linear_11;
        EXPECT_EQ(pmbus_utils::toString(format), "linear_11");
    }

    // Sensor data format: SensorDataFormat::linear_16
    {
        pmbus_utils::SensorDataFormat format =
            pmbus_utils::SensorDataFormat::linear_16;
        EXPECT_EQ(pmbus_utils::toString(format), "linear_16");
    }

    // Sensor value type: SensorValueType::iout
    {
        pmbus_utils::SensorValueType type = pmbus_utils::SensorValueType::iout;
        EXPECT_EQ(pmbus_utils::toString(type), "iout");
    }

    // Sensor value type: SensorValueType::iout_peak
    {
        pmbus_utils::SensorValueType type =
            pmbus_utils::SensorValueType::iout_peak;
        EXPECT_EQ(pmbus_utils::toString(type), "iout_peak");
    }

    // Sensor value type: SensorValueType::iout_valley
    {
        pmbus_utils::SensorValueType type =
            pmbus_utils::SensorValueType::iout_valley;
        EXPECT_EQ(pmbus_utils::toString(type), "iout_valley");
    }

    // Sensor value type: SensorValueType::pout
    {
        pmbus_utils::SensorValueType type = pmbus_utils::SensorValueType::pout;
        EXPECT_EQ(pmbus_utils::toString(type), "pout");
    }

    // Sensor value type: SensorValueType::temperature
    {
        pmbus_utils::SensorValueType type =
            pmbus_utils::SensorValueType::temperature;
        EXPECT_EQ(pmbus_utils::toString(type), "temperature");
    }

    // Sensor value type: SensorValueType::temperature_peak
    {
        pmbus_utils::SensorValueType type =
            pmbus_utils::SensorValueType::temperature_peak;
        EXPECT_EQ(pmbus_utils::toString(type), "temperature_peak");
    }

    // Sensor value type: SensorValueType::vout
    {
        pmbus_utils::SensorValueType type = pmbus_utils::SensorValueType::vout;
        EXPECT_EQ(pmbus_utils::toString(type), "vout");
    }

    // Sensor value type: SensorValueType::vout_peak
    {
        pmbus_utils::SensorValueType type =
            pmbus_utils::SensorValueType::vout_peak;
        EXPECT_EQ(pmbus_utils::toString(type), "vout_peak");
    }

    // Sensor value type: SensorValueType::vout_valley
    {
        pmbus_utils::SensorValueType type =
            pmbus_utils::SensorValueType::vout_valley;
        EXPECT_EQ(pmbus_utils::toString(type), "vout_valley");
    }

    // Vout data format: VoutDataFormat::linear
    {
        pmbus_utils::VoutDataFormat format =
            pmbus_utils::VoutDataFormat::linear;
        EXPECT_EQ(pmbus_utils::toString(format), "linear");
    }

    // Vout data format: VoutDataFormat::vid
    {
        pmbus_utils::VoutDataFormat format = pmbus_utils::VoutDataFormat::vid;
        EXPECT_EQ(pmbus_utils::toString(format), "vid");
    }

    // Vout data format: VoutDataFormat::direct
    {
        pmbus_utils::VoutDataFormat format =
            pmbus_utils::VoutDataFormat::direct;
        EXPECT_EQ(pmbus_utils::toString(format), "direct");
    }

    // Vout data format: VoutDataFormat::ieee
    {
        pmbus_utils::VoutDataFormat format = pmbus_utils::VoutDataFormat::ieee;
        EXPECT_EQ(pmbus_utils::toString(format), "ieee");
    }
}

TEST(PMBusUtilsTests, ConvertToVoutLinear)
{
    double volts;
    int8_t exponent;

    // Exponent > 0: Value is not rounded up
    volts = 13.9;
    exponent = 2;
    // 13.9 / 2^2 == 3.475 = 3
    EXPECT_EQ(pmbus_utils::convertToVoutLinear(volts, exponent), 3);

    // Exponent > 0: Value is rounded up
    volts = 14.0;
    exponent = 2;
    // 14.0 / 2^2 == 3.5 = 4
    EXPECT_EQ(pmbus_utils::convertToVoutLinear(volts, exponent), 4);

    // Exponent = 0: Value is not rounded up
    volts = 2.49;
    exponent = 0;
    // 2.49 / 2^0 == 2.49 = 2
    EXPECT_EQ(pmbus_utils::convertToVoutLinear(volts, exponent), 2);

    // Exponent = 0: Value is rounded up
    volts = 2.51;
    exponent = 0;
    // 2.51 / 2^0 == 2.51 = 3
    EXPECT_EQ(pmbus_utils::convertToVoutLinear(volts, exponent), 3);

    // Exponent < 0: Value is not rounded up
    volts = 1.32613;
    exponent = -8;
    // 1.32613 / 2^-8 == 339.48928 == 339
    EXPECT_EQ(pmbus_utils::convertToVoutLinear(volts, exponent), 339);

    // Exponent < 0: Value is rounded up
    volts = 1.32618;
    exponent = -8;
    // 1.32618 / 2^-8 == 339.50208 == 340
    EXPECT_EQ(pmbus_utils::convertToVoutLinear(volts, exponent), 340);
}
