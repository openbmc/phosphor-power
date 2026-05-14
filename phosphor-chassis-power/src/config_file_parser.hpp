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
#include "services.hpp"

#include <nlohmann/json.hpp>
#include <sdeventplus/event.hpp>

#include <filesystem>
#include <string>
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
 * @param services Services object
<<<<<<< HEAD
=======
 * @param event Event loop for timer operations
>>>>>>> 3d2bb96 (PCP enable/disable BMC POR circuitry from sled)
 * @return vector of C++ Chassis objects
 */
std::vector<std::unique_ptr<Chassis>> parse(
    const std::filesystem::path& pathName, Services& services,
    const sdeventplus::Event& event);

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
 * @param services Services object
<<<<<<< HEAD
=======
 * @param event Event loop for timer operations
>>>>>>> 3d2bb96 (PCP enable/disable BMC POR circuitry from sled)
 * @return Chassis object
 */
std::unique_ptr<Chassis> parseChassis(const nlohmann::json& element,
                                      Services& services,
                                      const sdeventplus::Event& event);

/**
 * Parses a JSON element containing an array of chassis.
 *
 * Returns vector of C++ Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param services Services object
<<<<<<< HEAD
=======
 * @param event Event loop for timer operations
>>>>>>> 3d2bb96 (PCP enable/disable BMC POR circuitry from sled)
 * @return Returns vector of C++ Chassis objects.
 */
std::vector<std::unique_ptr<Chassis>> parseChassisArray(
    const nlohmann::json& element, Services& services,
    const sdeventplus::Event& event);

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
std::unique_ptr<Gpio> parseGpio(const nlohmann::json& element,
                                Services& services);

/**
 * Parses a JSON element containing an absolute presence path.
 *
 * Returns the corresponding C++ string containing the absolute presence path.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @return absolute file presence path
 */
std::string parsePresencePath(const nlohmann::json& element);

/**
 * Parses a string to a gpioDirection enum value.
 *
 * @param directionStr Direction string ("Input" or "Output")
 * @return GpioDirection enum value
 * @throws std::invalid_argument if string is invalid
 */
GpioDirection parseDirection(const std::string& directionStr);

/**
 * Parses a string to a gpioPolarity enum value.
 *
 * @param polarityStr Polarity string ("Low" or "High")
 * @return GpioPolarity enum value
 * @throws std::invalid_argument if string is invalid
 */
GpioPolarity parsePolarity(const std::string& polarityStr);

/**
 * Parses the JSON root element of the entire configuration file.
 *
 * Returns the corresponding C++ Chassis objects.
 *
 * Throws an exception if parsing fails.
 *
 * @param element JSON element
 * @param services Services object
 * @param event Event loop for timer operations
 * @return vectors of Chassis objects
 */
std::vector<std::unique_ptr<Chassis>> parseRoot(const nlohmann::json& element,
                                                Services& services,
                                                const sdeventplus::Event& event);

} // namespace internal

} // namespace phosphor::power::chassis::config_file_parser
