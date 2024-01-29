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

#include "config_file_parser.hpp"

#include "config_file_parser_error.hpp"

#include <exception>
#include <fstream>
#include <optional>

using json = nlohmann::json;

namespace phosphor::power::sequencer::config_file_parser
{

std::vector<std::unique_ptr<Rail>> parse(const std::filesystem::path& pathName)
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

GPIO parseGPIO(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required line property
    const json& lineElement = getRequiredProperty(element, "line");
    unsigned int line = parseUnsignedInteger(lineElement);
    ++propertyCount;

    // Optional active_low property
    bool activeLow{false};
    auto activeLowIt = element.find("active_low");
    if (activeLowIt != element.end())
    {
        activeLow = parseBoolean(*activeLowIt);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return GPIO(line, activeLow);
}

std::unique_ptr<Rail> parseRail(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required name property
    const json& nameElement = getRequiredProperty(element, "name");
    std::string name = parseString(nameElement);
    ++propertyCount;

    // Optional presence property
    std::optional<std::string> presence{};
    auto presenceIt = element.find("presence");
    if (presenceIt != element.end())
    {
        presence = parseString(*presenceIt);
        ++propertyCount;
    }

    // Optional page property
    std::optional<uint8_t> page{};
    auto pageIt = element.find("page");
    if (pageIt != element.end())
    {
        page = parseUint8(*pageIt);
        ++propertyCount;
    }

    // Optional is_power_supply_rail property
    bool isPowerSupplyRail{false};
    auto isPowerSupplyRailIt = element.find("is_power_supply_rail");
    if (isPowerSupplyRailIt != element.end())
    {
        isPowerSupplyRail = parseBoolean(*isPowerSupplyRailIt);
        ++propertyCount;
    }

    // Optional check_status_vout property
    bool checkStatusVout{false};
    auto checkStatusVoutIt = element.find("check_status_vout");
    if (checkStatusVoutIt != element.end())
    {
        checkStatusVout = parseBoolean(*checkStatusVoutIt);
        ++propertyCount;
    }

    // Optional compare_voltage_to_limits property
    bool compareVoltageToLimits{false};
    auto compareVoltageToLimitsIt = element.find("compare_voltage_to_limits");
    if (compareVoltageToLimitsIt != element.end())
    {
        compareVoltageToLimits = parseBoolean(*compareVoltageToLimitsIt);
        ++propertyCount;
    }

    // Optional gpio property
    std::optional<GPIO> gpio{};
    auto gpioIt = element.find("gpio");
    if (gpioIt != element.end())
    {
        gpio = parseGPIO(*gpioIt);
        ++propertyCount;
    }

    // If check_status_vout or compare_voltage_to_limits property is true,
    // the page property is required; verify page was specified
    if ((checkStatusVout || compareVoltageToLimits) && !page.has_value())
    {
        throw std::invalid_argument{"Required property missing: page"};
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Rail>(name, presence, page, isPowerSupplyRail,
                                  checkStatusVout, compareVoltageToLimits,
                                  gpio);
}

std::vector<std::unique_ptr<Rail>> parseRailArray(const json& element)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Rail>> rails;
    for (auto& railElement : element)
    {
        rails.emplace_back(parseRail(railElement));
    }
    return rails;
}

std::vector<std::unique_ptr<Rail>> parseRoot(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required rails property
    const json& railsElement = getRequiredProperty(element, "rails");
    std::vector<std::unique_ptr<Rail>> rails = parseRailArray(railsElement);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return rails;
}

} // namespace internal

} // namespace phosphor::power::sequencer::config_file_parser
