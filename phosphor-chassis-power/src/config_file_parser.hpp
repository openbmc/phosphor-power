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
#pragma once

#include "chassis.hpp"
#include "gpio.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::power::chassis::config_file_parser
{

/**
 * Parses the specified JSON configuration file.
 *
 * Returns vector of C++ Chassis objects.
 *
 * Throws a ConfigFileParserError if an error occurs.
 *
 * @param pathName configuration file path name
 * @return vector of C++ Chassis objects
 */
std::vector<std::unique_ptr<Chassis>> parse(
    const std::filesystem::path& pathName);

/*
 * Internal implementation details for parse()
 */
namespace internal
{

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
 * Returns vector of C++ Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return Returns vector of C++ Chassis objects.
 */
std::vector<std::unique_ptr<Chassis>> parseChassisArray(
    const nlohmann::json& element);

/**
 * Parses a JSON element containing a GPIO.
 *
 * Returns the corresponding C++ Gpio object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return Gpio object
 */
std::unique_ptr<Gpio> parseGpio(const nlohmann::json& element);

/**
 * Parses a JSON element containing a relative inventory path.
 *
 * Returns the corresponding C++ string containing the absolute inventory path.
 *
 * Inventory paths in the JSON configuration file are relative.  Adds the
 * necessary prefix to make the path absolute.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return absolute D-Bus inventory path
 */
std::string parseInventoryPath(const nlohmann::json& element);

/**
 * Parses the JSON root element of the entire configuration file.
 *
 * Returns the corresponding C++ Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vectors of Chassis objects
 */
std::vector<std::unique_ptr<Chassis>> parseRoot(const nlohmann::json& element);

} // namespace internal

} // namespace phosphor::power::chassis::config_file_parser
