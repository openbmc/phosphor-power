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
#pragma once

#include <cmath>
#include <cstdint>
#include <string>

/**
 * @namespace pmbus_utils
 *
 * Contains utilities for sending PMBus commands over an I2C interface.
 */
namespace phosphor::power::regulators::pmbus_utils
{

/*
 * PMBus command codes.
 *
 * The constant names are all uppercase to match the PMBus documentation.
 *
 * Only the commands that are currently used by this application are defined.
 * See the PMBus documentation for all valid command codes.
 */
const uint8_t VOUT_MODE{0x20u};
const uint8_t VOUT_COMMAND{0x21u};

/**
 * Sensor data format.
 */
enum class SensorDataFormat
{
    /**
     * Linear data format used for values not related to voltage output, such
     * as output current, input voltage, and temperature. Two byte value with
     * an 11-bit, two's complement mantissa and a 5-bit, two's complement
     * exponent.
     */
    linear_11,

    /**
     * Linear data format used for values related to voltage output. Two
     * byte (16-bit), unsigned integer that is raised to the power of an
     * exponent. The exponent is not stored within the two bytes.
     */
    linear_16
};

/**
 * Sensor Value Type.
 */
enum class SensorValueType
{
    /**
     * Output current.
     */
    iout,

    /**
     * Highest output current.
     */
    iout_peak,

    /**
     * Lowest output current.
     */
    iout_valley,

    /**
     * Output power.
     */
    pout,

    /**
     * Temperature.
     */
    temperature,

    /**
     * Highest temperature.
     */
    temperature_peak,

    /**
     * Output voltage.
     */
    vout,

    /**
     * Highest output voltage.
     */
    vout_peak,

    /**
     * Lowest output voltage.
     */
    vout_valley
};

/**
 * Data formats for output voltage.
 *
 * These formats are used for commanding and reading output voltage and related
 * parameters.
 */
enum class VoutDataFormat
{
    /**
     * Linear scale that uses a two byte unsigned binary integer with a scaling
     * factor.
     */
    linear,

    /**
     * Format that supports transmitting VID codes.
     */
    vid,

    /**
     * Direct format that uses an equation and device supplied coefficients.
     */
    direct,

    /**
     * Half-precision floating point format that follows the IEEE-754 standard
     * for representing magnitudes in 16 bits.
     */
    ieee
};

/**
 * Parse the one byte value of the VOUT_MODE command.
 *
 * VOUT_MODE contains a 'mode' field that indicates the data format used for
 * output voltage values.
 *
 * VOUT_MODE also contains a 'parameter' field whose value is dependent on the
 * data format:
 *  - Linear format: value is an exponent
 *  - VID format: value is a VID code
 *  - IEEE and Direct formats: value is not used
 *
 * @param voutModeValue one byte value of VOUT_MODE command
 * @param format data format from the 'mode' field
 * @param parameter parameter value from the 'parameter' field
 */
void parseVoutMode(uint8_t voutModeValue, VoutDataFormat& format,
                   int8_t& parameter);

/**
 * Converts the specified SensorDataFormat value to a string.
 *
 * @param format SensorDataFormat value
 * @return string corresponding to the enum value
 */
std::string toString(SensorDataFormat format);

/**
 * Converts the specified SensorValueType value to string.
 *
 * @param type SensorValueType type
 * @return string corresponding to the enum value
 */
std::string toString(SensorValueType type);

/**
 * Converts the specified VoutDataFormat value to string.
 *
 * @param format VoutDataFormat format
 * @return string corresponding to the enum value
 */
std::string toString(VoutDataFormat format);

/**
 * Converts a linear data format value to a double value.
 *
 * This data format consists of the following:
 *   - Two byte value
 *   - 11-bit two's complement mantissa value stored in the two bytes
 *   - 5-bit two's complement exponent value stored in the two bytes
 *
 * @param value linear data format value
 * @return double value
 */
inline double convertFromLinear(uint16_t value)
{
    // extract exponent from most significant 5 bits
    uint8_t exponentField = value >> 11;

    // extract mantissa from least significant 11 bits
    uint16_t mantissaField = value & 0x7FFu;

    // sign extend exponent
    if (exponentField > 0x0Fu)
    {
        exponentField |= 0xE0u;
    }

    // sign extend mantissa
    if (mantissaField > 0x03FFu)
    {
        mantissaField |= 0xF800u;
    }

    int8_t exponent = static_cast<int8_t>(exponentField);
    int16_t mantissa = static_cast<int16_t>(mantissaField);

    // compute value as mantissa * 2^(exponent)
    double decimal = mantissa * std::pow(2.0, exponent);
    return decimal;
}

/**
 * Converts a linear data format output voltage value to a volts value.
 *
 * This data format consists of the following:
 *   - Two byte value
 *   - 16-bit unsigned mantissa value stored in the two bytes
 *   - 5-bit signed exponent value that is not stored in the two bytes
 *
 * @param value linear data format output voltage value
 * @param exponent exponent value obtained from VOUT_MODE or device
 *        documentation
 * @return normal decimal number
 */
inline double convertFromVoutLinear(uint16_t value, int8_t exponent)
{
    // compute value as mantissa * 2^(exponent)
    double decimal = value * std::pow(2.0, exponent);
    return decimal;
}

/**
 * Converts a volts value to the linear data format for output voltage.
 *
 * This data format consists of the following:
 *   - Two byte value
 *   - 16-bit unsigned mantissa value stored in the two bytes
 *   - 5-bit signed exponent value that is not stored in the two bytes
 *
 * The exponent value is typically obtained from the PMBus VOUT_MODE command
 * or from the hardware device documentation (data sheet).
 *
 * Note that this format differs from the linear data format for values
 * unrelated to output voltage.
 *
 * @param volts volts value to convert; must not be negative
 * @param exponent 5-bit signed exponent used to convert value
 * @return linear data format value
 */
inline uint16_t convertToVoutLinear(double volts, int8_t exponent)
{
    // Obtain mantissa using equation 'mantissa = volts / 2^exponent'
    double mantissa = volts / std::pow(2.0, static_cast<double>(exponent));

    // Return the mantissa value after converting to a rounded uint16_t
    return static_cast<uint16_t>(std::lround(mantissa));
}

} // namespace phosphor::power::regulators::pmbus_utils
