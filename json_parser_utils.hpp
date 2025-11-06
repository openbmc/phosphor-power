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
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @namespace json_parser_utils
 *
 * Contains utility functions for parsing JSON data.
 *
 * ## Variables
 * The parsing functions support optional usage of variables. JSON string values
 * can contain one or more variables. A variable is specified using the format
 * `${variable_name}`. A variables map is specified to parsing functions that
 * provides the variable values. Any variable in a JSON string value will be
 * replaced by the variable value.
 *
 * Variables can only appear in a JSON string value. The parsing functions for
 * other data types, such as integer and double, support a string value if it
 * contains a variable. After variable expansion, the string is converted to the
 * expected data type.
 */
namespace phosphor::power::json_parser_utils
{

/**
 * Empty variables map used as a default value for parsing functions.
 */
extern const std::map<std::string, std::string> NO_VARIABLES;

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
 * @param variables variables map used to expand variables in element value
 * @return uint8_t value
 */
uint8_t parseBitPosition(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing a bit value (0 or 1).
 *
 * Returns the corresponding C++ uint8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return uint8_t value
 */
uint8_t parseBitValue(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing a boolean.
 *
 * Returns the corresponding C++ boolean value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return boolean value
 */
bool parseBoolean(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing a double (floating point number).
 *
 * Returns the corresponding C++ double value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return double value
 */
double parseDouble(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing a byte value expressed as a hexadecimal
 * string.
 *
 * The JSON number data type does not support the hexadecimal format.  For this
 * reason, a hexadecimal byte value is stored in a JSON string.
 *
 * Returns the corresponding C++ uint8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return uint8_t value
 */
uint8_t parseHexByte(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing an array of byte values expressed as a
 * hexadecimal strings.
 *
 * Returns the corresponding C++ uint8_t values.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return vector of uint8_t
 */
std::vector<uint8_t> parseHexByteArray(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing an 8-bit signed integer.
 *
 * Returns the corresponding C++ int8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return int8_t value
 */
int8_t parseInt8(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing an integer.
 *
 * Returns the corresponding C++ int value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return integer value
 */
int parseInteger(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing a string.
 *
 * Returns the corresponding C++ string.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param isEmptyValid indicates whether an empty string value is valid
 * @param variables variables map used to expand variables in element value
 * @return string value
 */
std::string parseString(
    const nlohmann::json& element, bool isEmptyValid = false,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing an 8-bit unsigned integer.
 *
 * Returns the corresponding C++ uint8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return uint8_t value
 */
uint8_t parseUint8(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing a 16-bit unsigned integer.
 *
 * Returns the corresponding C++ uint16_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return uint16_t value
 */
uint16_t parseUint16(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

/**
 * Parses a JSON element containing an unsigned integer.
 *
 * Returns the corresponding C++ unsigned int value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return unsigned int value
 */
unsigned int parseUnsignedInteger(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables = NO_VARIABLES);

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

namespace internal
{

/**
 * Expands any variables that appear in the specified string value.
 *
 * Does nothing if the variables map is empty or the value contains no
 * variables.
 *
 * Throws an invalid_argument exception if a variable occurs in the value that
 * does not exist in the variables map.
 *
 * @param value string value in which to perform variable expansion
 * @param variables variables map containing variable values
 */
void expandVariables(std::string& value,
                     const std::map<std::string, std::string>& variables);

} // namespace internal

} // namespace phosphor::power::json_parser_utils
