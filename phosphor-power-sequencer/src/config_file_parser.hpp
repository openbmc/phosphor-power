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

#include "power_sequencer_device.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <tuple>
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
 * @param variables variables map used to expand variables in element value
 * @return GPIO object
 */
GPIO parseGPIO(const nlohmann::json& element,
               const std::map<std::string, std::string>& variables);

/**
 * Parses a JSON element containing an i2c_interface object.
 *
 * Returns the corresponding I2C bus and address.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return tuple containing bus and address
 */
std::tuple<uint8_t, uint16_t> parseI2CInterface(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables);

/**
 * Parses a JSON element containing a power_sequencer object.
 *
 * Returns the corresponding C++ PowerSequencerDevice object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @param services System services like hardware presence and the journal
 * @return PowerSequencerDevice object
 */
std::unique_ptr<PowerSequencerDevice> parsePowerSequencer(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables, Services& services);

/**
 * Parses a JSON element containing an array of power_sequencer objects.
 *
 * Returns the corresponding C++ PowerSequencerDevice objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @param services System services like hardware presence and the journal
 * @return vector of PowerSequencerDevice objects
 */
std::vector<std::unique_ptr<PowerSequencerDevice>> parsePowerSequencerArray(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables, Services& services);

/**
 * Parses a JSON element containing a rail.
 *
 * Returns the corresponding C++ Rail object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return Rail object
 */
std::unique_ptr<Rail> parseRail(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables);

/**
 * Parses a JSON element containing an array of rails.
 *
 * Returns the corresponding C++ Rail objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return vector of Rail objects
 */
std::vector<std::unique_ptr<Rail>> parseRailArray(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables);

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
