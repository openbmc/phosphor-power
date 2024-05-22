/**
 * Copyright © 2024 IBM Corporation
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

#include "rail.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace phosphor::power::sequencer::config_file_parser
{

/**
 * Standard JSON configuration file directory on the BMC.
 */
extern const std::filesystem::path standardConfigFileDirectory;

/**
 * Finds the JSON configuration file for the current system based on the
 * specified compatible system types.
 *
 * This is required when a single BMC firmware image supports multiple system
 * types and some system types require different configuration files.
 *
 * The compatible system types must be ordered from most to least specific.
 * Example:
 *   - com.acme.Hardware.Chassis.Model.MegaServer4CPU
 *   - com.acme.Hardware.Chassis.Model.MegaServer
 *   - com.acme.Hardware.Chassis.Model.Server
 *
 * Throws an exception if an error occurs.
 *
 * @param compatibleSystemTypes compatible system types for the current system
 *                              ordered from most to least specific
 * @param configFileDir directory containing configuration files
 * @return path to the JSON configuration file, or an empty path if none was
 *         found
 */
std::filesystem::path find(
    const std::vector<std::string>& compatibleSystemTypes,
    const std::filesystem::path& configFileDir = standardConfigFileDirectory);

/**
 * Parses the specified JSON configuration file.
 *
 * Returns the corresponding C++ Rail objects.
 *
 * Throws a ConfigFileParserError if an error occurs.
 *
 * @param pathName configuration file path name
 * @return vector of Rail objects
 */
std::vector<std::unique_ptr<Rail>> parse(const std::filesystem::path& pathName);

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
#pragma GCC diagnostic push
#if __GNUC__ == 13
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
 * Parses a JSON element containing a GPIO.
 *
 * Returns the corresponding C++ GPIO object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return GPIO object
 */
GPIO parseGPIO(const nlohmann::json& element);

/**
 * Parses a JSON element containing a rail.
 *
 * Returns the corresponding C++ Rail object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return Rail object
 */
std::unique_ptr<Rail> parseRail(const nlohmann::json& element);

/**
 * Parses a JSON element containing an array of rails.
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
 * Returns the corresponding C++ Rail objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Rail objects
 */
std::vector<std::unique_ptr<Rail>> parseRoot(const nlohmann::json& element);

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

} // namespace internal

} // namespace phosphor::power::sequencer::config_file_parser
