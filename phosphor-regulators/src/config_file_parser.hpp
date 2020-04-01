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

#include "action.hpp"
#include "chassis.hpp"
#include "configuration.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "i2c_write_bit_action.hpp"
#include "i2c_write_byte_action.hpp"
#include "i2c_write_bytes_action.hpp"
#include "pmbus_write_vout_command_action.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "run_rule_action.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::power::regulators::config_file_parser
{

/**
 * Parses the specified JSON configuration file.
 *
 * Returns the corresponding C++ Rule and Chassis objects.
 *
 * Throws a ConfigFileParserError if an error occurs.
 *
 * @param pathName configuration file path name
 * @return tuple containing vectors of Rule and Chassis objects
 */
std::tuple<std::vector<std::unique_ptr<Rule>>,
           std::vector<std::unique_ptr<Chassis>>>
    parse(const std::filesystem::path& pathName);

/*
 * Internal implementation details for parse()
 */
namespace internal
{

/**
 * Returns the specified property of the specified JSON element.
 *
 * Throws an invalid_argument exception if the property does not exist.
 *
 * @param element JSON element
 * @param property property name
 */
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

/**
 * Parses a JSON element containing an action.
 *
 * Returns the corresponding C++ Action object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return Action object
 */
std::unique_ptr<Action> parseAction(const nlohmann::json& element);

/**
 * Parses a JSON element containing an array of actions.
 *
 * Returns the corresponding C++ Action objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Action objects
 */
std::vector<std::unique_ptr<Action>>
    parseActionArray(const nlohmann::json& element);

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
    int value = element;
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
    int value = element;
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
 * Parses a JSON element containing a chassis.
 *
 * Returns the corresponding C++ Chassis object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return Chassis object
 */
std::unique_ptr<Chassis> parseChassis(const nlohmann::json& element);

/**
 * Parses a JSON element containing an array of chassis.
 *
 * Returns the corresponding C++ Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Chassis objects
 */
std::vector<std::unique_ptr<Chassis>>
    parseChassisArray(const nlohmann::json& element);

/**
 * Parses a JSON element containing a configuration.
 *
 * Returns the corresponding C++ Configuration object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return Configuration object
 */
std::unique_ptr<Configuration>
    parseConfiguration(const nlohmann::json& element);

/**
 * Parses a JSON element containing a device.
 *
 * Returns the corresponding C++ Device object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return Device object
 */
std::unique_ptr<Device> parseDevice(const nlohmann::json& element);

/**
 * Parses a JSON element containing an array of device.
 *
 * Returns the corresponding C++ Device objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Device objects
 */
std::vector<std::unique_ptr<Device>>
    parseDeviceArray(const nlohmann::json& element);

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
 * Parses a JSON element containing an i2c_interface action.
 *
 * Returns the corresponding C++ i2c::I2CInterface object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return i2c::I2CInterface object
 */
std::unique_ptr<i2c::I2CInterface>
    parseI2CInterface(const nlohmann::json& element);

/**
 * Parses a JSON element containing an i2c_write_bit action.
 *
 * Returns the corresponding C++ I2CWriteBitAction object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return I2CWriteBitAction object
 */
std::unique_ptr<I2CWriteBitAction>
    parseI2CWriteBit(const nlohmann::json& element);

/**
 * Parses a JSON element containing an i2c_write_byte action.
 *
 * Returns the corresponding C++ I2CWriteByteAction object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return I2CWriteByteAction object
 */
std::unique_ptr<I2CWriteByteAction>
    parseI2CWriteByte(const nlohmann::json& element);

/**
 * Parses a JSON element containing an i2c_write_bytes action.
 *
 * Returns the corresponding C++ I2CWriteBytesAction object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return I2CWriteBytesAction object
 */
std::unique_ptr<I2CWriteBytesAction>
    parseI2CWriteBytes(const nlohmann::json& element);

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
    int value = element;
    if ((value < INT8_MIN) || (value > INT8_MAX))
    {
        throw std::invalid_argument{"Element is not an 8-bit signed integer"};
    }
    return static_cast<int8_t>(value);
}

/**
 * Parses a JSON element containing an array of masks.
 *
 * Returns the corresponding C++ Mask objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Mask objects
 */
std::vector<uint8_t> parseMaskArray(const nlohmann::json& element);

/**
 * Parses a JSON element containing an unsigned integer for number.
 *
 * Returns the corresponding C++ unsigned int value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return unsigned int value
 */
inline unsigned int parseNumber(const nlohmann::json& element)
{
    // Verify element contains an integer
    if (!element.is_number_integer())
    {
        throw std::invalid_argument{"Element is not an integer"};
    }
    int value = element;
    if ((value < 1) || (value > 65535))
    {
        throw std::invalid_argument{"Element is not a number"};
    }
    return static_cast<unsigned int>(value);
}

/**
 * Parses a JSON element containing a pmbus_write_vout_command action.
 *
 * Returns the corresponding C++ PMBusWriteVoutCommandAction object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return PMBusWriteVoutCommandAction object
 */
std::unique_ptr<PMBusWriteVoutCommandAction>
    parsePMBusWriteVoutCommand(const nlohmann::json& element);

/**
 * Parses a JSON element containing an array of rail.
 *
 * Returns the corresponding C++ Rail objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Rail objects
 */
std::vector<std::unique_ptr<Rail>>
    parseRailArray(const nlohmann::json& element);

/**
 * Parses the JSON root element of the entire configuration file.
 *
 * Returns the corresponding C++ Rule and Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return tuple containing vectors of Rule and Chassis objects
 */
std::tuple<std::vector<std::unique_ptr<Rule>>,
           std::vector<std::unique_ptr<Chassis>>>
    parseRoot(const nlohmann::json& element);

/**
 * Parses a JSON element containing a rule.
 *
 * Returns the corresponding C++ Rule object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return Rule object
 */
std::unique_ptr<Rule> parseRule(const nlohmann::json& element);

/**
 * Parses a JSON element containing an array of rules.
 *
 * Returns the corresponding C++ Rule objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Rule objects
 */
std::vector<std::unique_ptr<Rule>>
    parseRuleArray(const nlohmann::json& element);

/**
 * Parses a JSON element containing an run_rule action.
 *
 * Returns the corresponding C++ RunRuleAction object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return RunRuleAction object
 */
std::unique_ptr<RunRuleAction> parseRunRule(const nlohmann::json& element);

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
    std::string value = element;
    if (value.empty() && !isEmptyValid)
    {
        throw std::invalid_argument{"Element contains an empty string"};
    }
    return value;
}

/**
 * Parses a JSON element containing a hexadecimal string.
 *
 * Returns the corresponding C++ uint8_t value.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return uint8_t value
 */
inline uint8_t parseStringToUint8(const nlohmann::json& element)
{
    std::string value = parseString(element);

    bool isHex = (value.compare(0, 2, "0x") == 0) && (value.size() > 2) &&
                 (value.size() < 5) &&
                 (value.find_first_not_of("0123456789abcdefABCDEF", 2) ==
                  std::string::npos);
    if (!isHex)
    {
        throw std::invalid_argument{"Element is not hexadecimal string"};
    }
    return static_cast<uint8_t>(std::stoul(value, 0, 0));
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
    int value = element;
    if ((value < 0) || (value > UINT8_MAX))
    {
        throw std::invalid_argument{"Element is not an 8-bit unsigned integer"};
    }
    return static_cast<uint8_t>(value);
}

/**
 * Parses a JSON element containing an array of values.
 *
 * Returns the corresponding C++ Value objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Value objects
 */
std::vector<uint8_t> parseValueArray(const nlohmann::json& element);

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

} // namespace internal

} // namespace phosphor::power::regulators::config_file_parser
