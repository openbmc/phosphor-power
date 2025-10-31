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

#include <filesystem>
#include <memory>
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
std::vector<std::unique_ptr<Rail>> parseRailArray(
    const nlohmann::json& element);

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

} // namespace internal

} // namespace phosphor::power::sequencer::config_file_parser
