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

#include "chassis.hpp"
#include "chassis_status_monitor.hpp"
#include "power_sequencer_device.hpp"
#include "rail.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <functional> // for reference_wrapper
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

using json = nlohmann::json;

namespace phosphor::power::sequencer::config_file_parser
{

/**
 * Standard JSON configuration file directory on the BMC.
 */
extern const std::filesystem::path standardConfigFileDirectory;

/**
 * Default JSON configuration file name.
 */
extern const std::string defaultConfigFileName;

/**
 * Returns the path to the default JSON configuration file.
 *
 * @return default config file path
 */
std::filesystem::path getDefaultConfigFilePath();

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
 * Returns the corresponding C++ Chassis objects.
 *
 * Throws a ConfigFileParserError if an error occurs.
 *
 * @param pathName configuration file path name
 * @return vector of Chassis objects
 */
std::vector<std::unique_ptr<Chassis>> parse(
    const std::filesystem::path& pathName);

/*
 * Internal implementation details for parse()
 */
namespace internal
{

using JSONRefWrapper = std::reference_wrapper<const json>;

/**
 * Parses a JSON element containing a chassis object.
 *
 * Returns the corresponding C++ Chassis object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param chassisTemplates chassis templates map
 * @return Chassis object
 */
std::unique_ptr<Chassis> parseChassis(
    const json& element,
    const std::map<std::string, JSONRefWrapper>& chassisTemplates);

/**
 * Parses a JSON element containing an array of chassis objects.
 *
 * Returns the corresponding C++ Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param chassisTemplates chassis templates map
 * @return vector of Chassis objects
 */
std::vector<std::unique_ptr<Chassis>> parseChassisArray(
    const json& element,
    const std::map<std::string, JSONRefWrapper>& chassisTemplates);

/**
 * Parses a JSON element containing the properties of a chassis.
 *
 * The JSON element may be a chassis object or chassis_template object.
 *
 * Returns the corresponding C++ Chassis object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param isChassisTemplate specifies whether element is a chassis_template
 * @param variables variables map used to expand variables in element value
 * @return Chassis object
 */
std::unique_ptr<Chassis> parseChassisProperties(
    const json& element, bool isChassisTemplate,
    const std::map<std::string, std::string>& variables);

/**
 * Parses a JSON element containing a chassis_template object.
 *
 * Returns the template ID and a C++ reference_wrapper to the JSON element.
 *
 * A chassis_template object cannot be fully parsed in isolation. It is a
 * template that contains variables.
 *
 * The chassis_template object is used by one or more chassis objects to avoid
 * duplicate JSON. The chassis objects define chassis-specific values for the
 * template variables.
 *
 * When the chassis object is parsed, the chassis_template JSON will be
 * re-parsed, and the template variables will be replaced with the
 * chassis-specific values.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return template ID and reference_wrapper to JSON element
 */
std::tuple<std::string, JSONRefWrapper> parseChassisTemplate(
    const json& element);

/**
 * Parses a JSON element containing an array of chassis_template objects.
 *
 * Returns a map of template IDs to chassis_template JSON elements.
 *
 * Note that chassis_template objects cannot be fully parsed in isolation. See
 * parseChassisTemplate() for more information.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return chassis templates map
 */
std::map<std::string, JSONRefWrapper> parseChassisTemplateArray(
    const json& element);

/**
 * Parses a JSON element containing chassis status monitoring options.
 *
 * Returns the corresponding C++ ChassisStatusMonitorOptions object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return ChassisStatusMonitorOptions object
 */
ChassisStatusMonitorOptions parseStatusMonitoring(
    const json& element, const std::map<std::string, std::string>& variables);

/**
 * Parses a JSON element containing a gpio object.
 *
 * Returns the corresponding C++ PgoodGPIO object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return PgoodGPIO object
 */
PgoodGPIO parseGPIO(const json& element,
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
    const json& element, const std::map<std::string, std::string>& variables);

/**
 * Parses a JSON element containing a power_sequencer object.
 *
 * Returns the corresponding C++ PowerSequencerDevice object.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return PowerSequencerDevice object
 */
std::unique_ptr<PowerSequencerDevice> parsePowerSequencer(
    const json& element, const std::map<std::string, std::string>& variables);

/**
 * Parses a JSON element containing an array of power_sequencer objects.
 *
 * Returns the corresponding C++ PowerSequencerDevice objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param variables variables map used to expand variables in element value
 * @return vector of PowerSequencerDevice objects
 */
std::vector<std::unique_ptr<PowerSequencerDevice>> parsePowerSequencerArray(
    const json& element, const std::map<std::string, std::string>& variables);

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
    const json& element, const std::map<std::string, std::string>& variables);

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
    const json& element, const std::map<std::string, std::string>& variables);

/**
 * Parses the JSON root element of the entire configuration file.
 *
 * Returns the corresponding C++ Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return vector of Chassis objects
 */
std::vector<std::unique_ptr<Chassis>> parseRoot(const json& element);

/**
 * Parses a JSON element containing an object with variable names and values.
 *
 * Returns the corresponding C++ map of variable names and values.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return map of variable names and values
 */
std::map<std::string, std::string> parseVariables(const json& element);

} // namespace internal

} // namespace phosphor::power::sequencer::config_file_parser
