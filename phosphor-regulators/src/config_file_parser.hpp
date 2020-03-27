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
#include "pmbus_write_vout_command_action.hpp"
#include "rule.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class ConfigFileParser
 *
 * Parser for the JSON configuration file that controls the phosphor-regulators
 * application.
 *
 * This class parses the JSON elements in the file and creates the corresponding
 * C++ objects.
 */
class ConfigFileParser
{
  public:
    // Specify which compiler-generated methods we want
    ConfigFileParser() = delete;
    ConfigFileParser(const ConfigFileParser&) = delete;
    ConfigFileParser(ConfigFileParser&&) = delete;
    ConfigFileParser& operator=(const ConfigFileParser&) = delete;
    ConfigFileParser& operator=(ConfigFileParser&&) = delete;
    ~ConfigFileParser() = default;

    /**
     * Constructor.
     *
     * Parses the JSON elements in the file and creates the corresponding C++
     * objects.
     *
     * Throws ConfigFileParserError if an error occurs.
     *
     * @param pathName configuration file path name
     */
    explicit ConfigFileParser(const std::filesystem::path& pathName);

    /**
     * Returns the chassis in the configuration file.
     *
     * Returns a non-const reference to the vector so the caller can std::move()
     * the vector if needed.
     *
     * @return chassis in configuration file
     */
    std::vector<std::unique_ptr<Chassis>>& getChassis()
    {
        return chassis;
    }

    /**
     * Returns the configuration file path name.
     *
     * @return path name
     */
    const std::filesystem::path& getPathName()
    {
        return pathName;
    }

    /**
     * Returns the rules in the configuration file, if any.
     *
     * Returns a non-const reference to the vector so the caller can std::move()
     * the vector if needed.
     *
     * @return rules in configuration file
     */
    std::vector<std::unique_ptr<Rule>>& getRules()
    {
        return rules;
    }

  private:
    /**
     * Configuration file path name.
     */
    const std::filesystem::path pathName;

    /**
     * Rules in the configuration file.
     */
    std::vector<std::unique_ptr<Rule>> rules{};

    /**
     * Chassis in the configuration file.
     */
    std::vector<std::unique_ptr<Chassis>> chassis{};
};

/*
 * Internal utility functions used to implement ConfigFileParser.
 *
 * Defined as internal functions instead of private methods to simplify testing.
 */
namespace config_file_parser::internal
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
 * Returns the corresponding vector of C++ Action objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Action objects
 */
std::vector<std::unique_ptr<Action>>
    parseActionArray(const nlohmann::json& element);

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
 * Parses a JSON element containing an array of chassis.
 *
 * Returns the corresponding vector of C++ Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Chassis objects
 */
std::vector<std::unique_ptr<Chassis>>
    parseChassisArray(const nlohmann::json& element);

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
 * Parses the JSON root element of the entire configuration file.
 *
 * Returns the corresponding C++ Rule and Chassis objects in the specified
 * vectors.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param rules vector where Rule objects will be stored
 * @param chassis vector where Chassis objects will be stored
 */
void parseRoot(const nlohmann::json& element,
               std::vector<std::unique_ptr<Rule>>& rules,
               std::vector<std::unique_ptr<Chassis>>& chassis);

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
 * Returns the corresponding vector of C++ Rule objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Rule objects
 */
std::vector<std::unique_ptr<Rule>>
    parseRuleArray(const nlohmann::json& element);

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

} // namespace config_file_parser::internal

} // namespace phosphor::power::regulators
