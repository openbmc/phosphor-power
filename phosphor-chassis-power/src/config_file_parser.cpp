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

#include "config_file_parser.hpp"

#include "config_file_parser_error.hpp"
#include "json_parser_utils.hpp"

#include <cstdint>
#include <exception>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <utility>

using json = nlohmann::json;
using ConfigFileParserError = phosphor::power::util::ConfigFileParserError;
using namespace phosphor::power::json_parser_utils;

namespace phosphor::power::chassis::config_file_parser
{

std::vector<std::unique_ptr<Chassis>> parse(
    const std::filesystem::path& pathName)
{
    try
    {
        // Use standard JSON parser to create tree of JSON elements
        std::ifstream file{pathName};
        json rootElement = json::parse(file);

        // Parse tree of JSON elements and return corresponding C++ objects
        return internal::parseRoot(rootElement);
    }
    catch (const std::exception& e)
    {
        throw ConfigFileParserError{pathName, e.what()};
    }
}

namespace internal
{

std::unique_ptr<Chassis> parseChassis(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required number property
    const json& numberElement = getRequiredProperty(element, "ChassisNumber");
    unsigned int number = parseUnsignedInteger(numberElement);
    if (number < 1)
    {
        throw std::invalid_argument{"Invalid chassis number: Must be > 0"};
    }
    ++propertyCount;

    // Optional presence_path property
    std::optional<std::string> inventoryPath{};
    auto inventoryPathElement = element.find("PresencePath");
    if (inventoryPathElement != element.end())
    {
        inventoryPath = parseInventoryPath(*inventoryPathElement);
        ++propertyCount;
    }

    // Optional GPIO
    std::vector<std::unique_ptr<Gpio>> gpios{};
    // Optional PresenceGpio
    auto PresenceGpioDeviceIt = element.find("PresenceGpio");
    if (PresenceGpioDeviceIt != element.end())
    {
        gpios.emplace_back(parseGpio(*PresenceGpioDeviceIt));
        ++propertyCount;
    }

    // Optional FaultUnlatchedGpio
    auto FaultUnlatchedGpioDeviceIt = element.find("FaultUnlatchedGpio");
    if (FaultUnlatchedGpioDeviceIt != element.end())
    {
        gpios.emplace_back(parseGpio(*FaultUnlatchedGpioDeviceIt));
        ++propertyCount;
    }

    // Optional FaultLatchedGpio
    auto FaultLatchedGpioDeviceIt = element.find("FaultLatchedGpio");
    if (FaultLatchedGpioDeviceIt != element.end())
    {
        gpios.emplace_back(parseGpio(*FaultLatchedGpioDeviceIt));
        ++propertyCount;
    }

    // Optional FaultLatchResetGpio
    auto FaultLatchResetGpioDeviceIt = element.find("FaultLatchResetGpio");
    if (FaultLatchResetGpioDeviceIt != element.end())
    {
        gpios.emplace_back(parseGpio(*FaultLatchResetGpioDeviceIt));
        ++propertyCount;
    }

    // Optional EnableSystemResetGpio
    auto EnableSystemResetGpioDeviceIt = element.find("EnableSystemResetGpio");
    if (EnableSystemResetGpioDeviceIt != element.end())
    {
        gpios.emplace_back(parseGpio(*EnableSystemResetGpioDeviceIt));
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Chassis>(number, std::move(inventoryPath),
                                     std::move(gpios));
}

std::vector<std::unique_ptr<Chassis>> parseChassisArray(const json& element)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Chassis>> chassis;
    for (auto& chassisElement : element)
    {
        chassis.emplace_back(parseChassis(chassisElement));
    }
    return chassis;
}

std::unique_ptr<Gpio> parseGpio(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required Name property
    const json& nameElement = getRequiredProperty(element, "Name");
    std::string name = parseString(nameElement);
    ++propertyCount;

    // Required Direction property
    const json& directionElement = getRequiredProperty(element, "Direction");
    std::string directionStr = parseString(directionElement);
    gpioDirection direction = parseDirection(directionStr);
    ++propertyCount;

    // Required Polarity property
    const json& polarityElement = getRequiredProperty(element, "Polarity");
    std::string polarityStr = parseString(polarityElement);
    gpioPolarity polarity = parsePolarity(polarityStr);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Gpio>(name, direction, polarity);
}

std::string parseInventoryPath(const json& element)
{
    std::string inventoryPath = parseString(element);
    std::string absPath = "/xyz/openbmc_project/inventory";
    if (inventoryPath.front() != '/')
    {
        absPath += '/';
    }
    absPath += inventoryPath;
    return absPath;
}

std::vector<std::unique_ptr<Chassis>> parseRoot(const json& element)
{
    verifyIsArray(element);

    // Parse the array of chassis objects
    return parseChassisArray(element);
}

} // namespace internal

} // namespace phosphor::power::chassis::config_file_parser
