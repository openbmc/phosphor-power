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
    const std::filesystem::path& pathName, Services& services)
{
    try
    {
        // Use standard JSON parser to create tree of JSON elements
        std::ifstream file{pathName};
        json rootElement = json::parse(file);

        // Parse tree of JSON elements and return corresponding C++ objects
        return internal::parseRoot(rootElement, services);
    }
    catch (const std::exception& e)
    {
        throw ConfigFileParserError{pathName, e.what()};
    }
}

namespace internal
{

std::unique_ptr<Chassis> parseChassis(const json& element, Services& services)
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

    // Optional GPIO
    std::vector<std::unique_ptr<Gpio>> gpios{};
    // Optional PresenceGpio
    auto presenceGpioIt = element.find("PresenceGpio");
    if (presenceGpioIt != element.end())
    {
        gpios.emplace_back(parseGpio(*presenceGpioIt, services));
        ++propertyCount;
    }

    // Optional FaultUnlatchedGpio
    auto faultUnlatchedGpioIt = element.find("FaultUnlatchedGpio");
    if (faultUnlatchedGpioIt != element.end())
    {
        gpios.emplace_back(parseGpio(*faultUnlatchedGpioIt, services));
        ++propertyCount;
    }

    // Optional FaultLatchedGpio
    auto faultLatchedGpioIt = element.find("FaultLatchedGpio");
    if (faultLatchedGpioIt != element.end())
    {
        gpios.emplace_back(parseGpio(*faultLatchedGpioIt, services));
        ++propertyCount;
    }

    // Optional FaultLatchResetGpio
    auto faultLatchResetGpioIt = element.find("FaultLatchResetGpio");
    if (faultLatchResetGpioIt != element.end())
    {
        gpios.emplace_back(parseGpio(*faultLatchResetGpioIt, services));
        ++propertyCount;
    }

    // Optional EnableSystemResetGpio
    auto enableSystemResetGpioIt = element.find("EnableSystemResetGpio");
    if (enableSystemResetGpioIt != element.end())
    {
        gpios.emplace_back(parseGpio(*enableSystemResetGpioIt, services));
        ++propertyCount;
    }

    // Optional PresencePath property
    std::optional<std::string> presencePath{};
    auto presencePathElement = element.find("PresencePath");
    if (presencePathElement != element.end())
    {
        presencePath = parsePresencePath(*presencePathElement);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Chassis>(number, services, std::move(presencePath),
                                     std::move(gpios));
}

std::vector<std::unique_ptr<Chassis>> parseChassisArray(const json& element,
                                                        Services& services)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Chassis>> chassis;
    for (auto& chassisElement : element)
    {
        chassis.emplace_back(parseChassis(chassisElement, services));
    }
    return chassis;
}

std::unique_ptr<Gpio> parseGpio(const json& element, Services& services)
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
    GpioDirection direction = parseDirection(directionStr);
    ++propertyCount;

    // Required Polarity property
    const json& polarityElement = getRequiredProperty(element, "Polarity");
    std::string polarityStr = parseString(polarityElement);
    GpioPolarity polarity = parsePolarity(polarityStr);
    ++propertyCount;

    // Optional Default property
    std::optional<uint8_t> defaultValue{};
    auto defaultElement = element.find("DefaultValue");
    if (defaultElement != element.end())
    {
        // Perform check to confirm only read default value on input polarity
        if (direction == GpioDirection::Input)
        {
            defaultValue = parseBitValue(*defaultElement, NO_VARIABLES);
            ++propertyCount;
        }
        else
        {
            throw std::invalid_argument{
                "default value not valid with output direction"};
        }
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return services.createGPIO(name, direction, polarity, defaultValue);
}

std::string parsePresencePath(const json& element)
{
    std::string presencePath = parseString(element);
    // if presence path is absolute path (leading with '/')
    //  then return else throw exception.
    if (presencePath.front() == '/')
    {
        return presencePath;
    }
    else
    {
        throw std::invalid_argument{"Invalid absolute path"};
    }
}

GpioDirection parseDirection(const std::string& directionStr)
{
    if (directionStr == "Input")
    {
        return GpioDirection::Input;
    }
    else if (directionStr == "Output")
    {
        return GpioDirection::Output;
    }
    else
    {
        throw std::invalid_argument{"Invalid direction value: " + directionStr};
    }
}

GpioPolarity parsePolarity(const std::string& polarityStr)
{
    if (polarityStr == "Low")
    {
        return GpioPolarity::Low;
    }
    else if (polarityStr == "High")
    {
        return GpioPolarity::High;
    }
    else
    {
        throw std::invalid_argument{"Invalid polarity value: " + polarityStr};
    }
}

std::vector<std::unique_ptr<Chassis>> parseRoot(const json& element,
                                                Services& services)
{
    verifyIsArray(element);

    // Parse the array of chassis objects
    return parseChassisArray(element, services);
}

} // namespace internal

} // namespace phosphor::power::chassis::config_file_parser
