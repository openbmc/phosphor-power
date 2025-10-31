/**
 * Copyright © 2025 IBM Corporation
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

#include <nlohmann/json.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @namespace json_parser_utils
 *
 * Contains utility functions for parsing JSON data.
 */
namespace phosphor::power::json_parser_utils
{

/**
 * Returns the specified property of the specified JSON element.
 *
 * Throws an invalid_argument exception if the property does not exist.
 *
 * @param element JSON element
 * @param property property name
 */
#pragma GCC diagnostic push
#if __GNUC__ >= 13
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif
inline const nlohmann::json& getRequiredProperty(const nlohmann::json& element,
                                                 const std::string& property)
{
    auto it = element.find(property);
    if (it == element.end())
    {
        throw std::invalid_argument{"Required property missing: " + property};
    }
    return *it;
}
#pragma GCC diagnostic pop

/**
 * Parses a JSON element containing a bit position (from 0-7).
 *
 * Returns the corresponding C++ uint8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return uint8_t value
 */
inline uint8_t parseBitPosition(const nlohmann::json& element)
{
    // Verify element contains an integer
    if (!element.is_number_integer())
    {
        throw std::invalid_argument{"Element is not an integer"};
    }
    int value = element.get<int>();
    if ((value < 0) || (value > 7))
    {
        throw std::invalid_argument{"Element is not a bit position"};
    }
    return static_cast<uint8_t>(value);
}

/**
 * Parses a JSON element containing a bit value (0 or 1).
 *
 * Returns the corresponding C++ uint8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return uint8_t value
 */
inline uint8_t parseBitValue(const nlohmann::json& element)
{
    // Verify element contains an integer
    if (!element.is_number_integer())
    {
        throw std::invalid_argument{"Element is not an integer"};
    }
    int value = element.get<int>();
    if ((value < 0) || (value > 1))
    {
        throw std::invalid_argument{"Element is not a bit value"};
    }
    return static_cast<uint8_t>(value);
}

/**
 * Parses a JSON element containing a boolean.
 *
 * Returns the corresponding C++ boolean value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return boolean value
 */
inline bool parseBoolean(const nlohmann::json& element)
{
    // Verify element contains a boolean
    if (!element.is_boolean())
    {
        throw std::invalid_argument{"Element is not a boolean"};
    }
    return element.get<bool>();
}

/**
 * Parses a JSON element containing a double (floating point number).
 *
 * Returns the corresponding C++ double value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return double value
 */
inline double parseDouble(const nlohmann::json& element)
{
    // Verify element contains a number (integer or floating point)
    if (!element.is_number())
    {
        throw std::invalid_argument{"Element is not a number"};
    }
    return element.get<double>();
}

/**
 * Parses a JSON element containing a byte value expressed as a hexadecimal
 * string.
 *
 * The JSON number data type does not support the hexadecimal format.  For this
 * reason, hexadecimal byte values are stored as strings in the configuration
 * file.
 *
 * Returns the corresponding C++ uint8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return uint8_t value
 */
inline uint8_t parseHexByte(const nlohmann::json& element)
{
    if (!element.is_string())
    {
        throw std::invalid_argument{"Element is not a string"};
    }
    std::string value = element.get<std::string>();

    bool isHex = (value.compare(0, 2, "0x") == 0) && (value.size() > 2) &&
                 (value.size() < 5) &&
                 (value.find_first_not_of("0123456789abcdefABCDEF", 2) ==
                  std::string::npos);
    if (!isHex)
    {
        throw std::invalid_argument{"Element is not hexadecimal string"};
    }
    return static_cast<uint8_t>(std::stoul(value, nullptr, 0));
}

/**
 * Parses a JSON element containing an array of byte values expressed as a
 * hexadecimal strings.
 *
 * Returns the corresponding C++ uint8_t values.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of uint8_t
 */
std::vector<uint8_t> parseHexByteArray(const nlohmann::json& element);

/**
 * Parses a JSON element containing an 8-bit signed integer.
 *
 * Returns the corresponding C++ int8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return int8_t value
 */
inline int8_t parseInt8(const nlohmann::json& element)
{
    // Verify element contains an integer
    if (!element.is_number_integer())
    {
        throw std::invalid_argument{"Element is not an integer"};
    }
    int value = element.get<int>();
    if ((value < INT8_MIN) || (value > INT8_MAX))
    {
        throw std::invalid_argument{"Element is not an 8-bit signed integer"};
    }
    return static_cast<int8_t>(value);
}

/**
 * Parses a JSON element containing a string.
 *
 * Returns the corresponding C++ string.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param isEmptyValid indicates whether an empty string value is valid
 * @return string value
 */
inline std::string parseString(const nlohmann::json& element,
                               bool isEmptyValid = false)
{
    if (!element.is_string())
    {
        throw std::invalid_argument{"Element is not a string"};
    }
    std::string value = element.get<std::string>();
    if (value.empty() && !isEmptyValid)
    {
        throw std::invalid_argument{"Element contains an empty string"};
    }
    return value;
}

/**
 * Parses a JSON element containing an 8-bit unsigned integer.
 *
 * Returns the corresponding C++ uint8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return uint8_t value
 */
inline uint8_t parseUint8(const nlohmann::json& element)
{
    // Verify element contains an integer
    if (!element.is_number_integer())
    {
        throw std::invalid_argument{"Element is not an integer"};
    }
    int value = element.get<int>();
    if ((value < 0) || (value > UINT8_MAX))
    {
        throw std::invalid_argument{"Element is not an 8-bit unsigned integer"};
    }
    return static_cast<uint8_t>(value);
}

/**
 * Parses a JSON element containing an unsigned integer.
 *
 * Returns the corresponding C++ unsigned int value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return unsigned int value
 */
inline unsigned int parseUnsignedInteger(const nlohmann::json& element)
{
    // Verify element contains an unsigned integer
    if (!element.is_number_unsigned())
    {
        throw std::invalid_argument{"Element is not an unsigned integer"};
    }
    return element.get<unsigned int>();
}

/**
 * Verifies that the specified JSON element is a JSON array.
 *
 * Throws an invalid_argument exception if the element is not an array.
 *
 * @param element JSON element
 */
inline void verifyIsArray(const nlohmann::json& element)
{
    if (!element.is_array())
    {
        throw std::invalid_argument{"Element is not an array"};
    }
}

/**
 * Verifies that the specified JSON element is a JSON object.
 *
 * Throws an invalid_argument exception if the element is not an object.
 *
 * @param element JSON element
 */
inline void verifyIsObject(const nlohmann::json& element)
{
    if (!element.is_object())
    {
        throw std::invalid_argument{"Element is not an object"};
    }
}

/**
 * Verifies that the specified JSON element contains the expected number of
 * properties.
 *
 * Throws an invalid_argument exception if the element contains a different
 * number of properties.  This indicates the element contains an invalid
 * property.
 *
 * @param element JSON element
 * @param expectedCount expected number of properties in element
 */
inline void verifyPropertyCount(const nlohmann::json& element,
                                unsigned int expectedCount)
{
    if (element.size() != expectedCount)
    {
        throw std::invalid_argument{"Element contains an invalid property"};
    }
}

} // namespace phosphor::power::json_parser_utils
