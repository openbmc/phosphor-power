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
#include "json_parser_utils.hpp"
#include "ucd90160_device.hpp"
#include "ucd90320_device.hpp"

#include <cstdint>
#include <exception>
#include <fstream>
#include <optional>
#include <stdexcept>

using namespace phosphor::power::json_parser_utils;
using ConfigFileParserError = phosphor::power::util::ConfigFileParserError;
namespace fs = std::filesystem;

namespace phosphor::power::sequencer::config_file_parser
{

const std::filesystem::path standardConfigFileDirectory{
    "/usr/share/phosphor-power-sequencer"};

std::filesystem::path find(
    const std::vector<std::string>& compatibleSystemTypes,
    const std::filesystem::path& configFileDir)
{
    fs::path pathName, possiblePath;
    std::string fileName;

    for (const std::string& systemType : compatibleSystemTypes)
    {
        // Look for file name that is entire system type + ".json"
        // Example: com.acme.Hardware.Chassis.Model.MegaServer.json
        fileName = systemType + ".json";
        possiblePath = configFileDir / fileName;
        if (fs::is_regular_file(possiblePath))
        {
            pathName = possiblePath;
            break;
        }

        // Look for file name that is last node of system type + ".json"
        // Example: MegaServer.json
        std::string::size_type pos = systemType.rfind('.');
        if ((pos != std::string::npos) && ((systemType.size() - pos) > 1))
        {
            fileName = systemType.substr(pos + 1) + ".json";
            possiblePath = configFileDir / fileName;
            if (fs::is_regular_file(possiblePath))
            {
                pathName = possiblePath;
                break;
            }
        }
    }

    return pathName;
}

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

std::tuple<std::string, JSONRefWrapper> parseChassisTemplate(
    const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Required id property
    const json& idElement = getRequiredProperty(element, "id");
    std::string id = parseString(idElement);
    ++propertyCount;

    // Required number property
    // Just verify it exists; cannot be parsed without variable values
    getRequiredProperty(element, "number");
    ++propertyCount;

    // Required inventory_path property
    // Just verify it exists; cannot be parsed without variable values
    getRequiredProperty(element, "inventory_path");
    ++propertyCount;

    // Required power_sequencers property
    // Just verify it exists; cannot be parsed without variable values
    getRequiredProperty(element, "power_sequencers");
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return {id, JSONRefWrapper{element}};
}

GPIO parseGPIO(const json& element,
               const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required line property
    const json& lineElement = getRequiredProperty(element, "line");
    unsigned int line = parseUnsignedInteger(lineElement, variables);
    ++propertyCount;

    // Optional active_low property
    bool activeLow{false};
    auto activeLowIt = element.find("active_low");
    if (activeLowIt != element.end())
    {
        activeLow = parseBoolean(*activeLowIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return GPIO(line, activeLow);
}

std::tuple<uint8_t, uint16_t> parseI2CInterface(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required bus property
    const json& busElement = getRequiredProperty(element, "bus");
    uint8_t bus = parseUint8(busElement, variables);
    ++propertyCount;

    // Required address property
    const json& addressElement = getRequiredProperty(element, "address");
    uint16_t address = parseHexByte(addressElement, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return {bus, address};
}

std::unique_ptr<PowerSequencerDevice> parsePowerSequencer(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables, Services& services)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Required type property
    const json& typeElement = getRequiredProperty(element, "type");
    std::string type = parseString(typeElement, false, variables);
    ++propertyCount;

    // Required i2c_interface property
    const json& i2cInterfaceElement =
        getRequiredProperty(element, "i2c_interface");
    auto [bus, address] = parseI2CInterface(i2cInterfaceElement, variables);
    ++propertyCount;

    // Required power_control_gpio_name property
    const json& powerControlGPIONameElement =
        getRequiredProperty(element, "power_control_gpio_name");
    std::string powerControlGPIOName =
        parseString(powerControlGPIONameElement, false, variables);
    ++propertyCount;

    // Required power_good_gpio_name property
    const json& powerGoodGPIONameElement =
        getRequiredProperty(element, "power_good_gpio_name");
    std::string powerGoodGPIOName =
        parseString(powerGoodGPIONameElement, false, variables);
    ++propertyCount;

    // Required rails property
    const json& railsElement = getRequiredProperty(element, "rails");
    std::vector<std::unique_ptr<Rail>> rails =
        parseRailArray(railsElement, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    if (type == UCD90160Device::deviceName)
    {
        return std::make_unique<UCD90160Device>(
            bus, address, powerControlGPIOName, powerGoodGPIOName,
            std::move(rails), services);
    }
    else if (type == UCD90320Device::deviceName)
    {
        return std::make_unique<UCD90320Device>(
            bus, address, powerControlGPIOName, powerGoodGPIOName,
            std::move(rails), services);
    }
    else
    {
        throw std::invalid_argument{"Invalid power sequencer type: " + type};
    }
}

std::vector<std::unique_ptr<PowerSequencerDevice>> parsePowerSequencerArray(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables, Services& services)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers;
    for (auto& powerSequencerElement : element)
    {
        powerSequencers.emplace_back(
            parsePowerSequencer(powerSequencerElement, variables, services));
    }
    return powerSequencers;
}

std::unique_ptr<Rail> parseRail(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required name property
    const json& nameElement = getRequiredProperty(element, "name");
    std::string name = parseString(nameElement, false, variables);
    ++propertyCount;

    // Optional presence property
    std::optional<std::string> presence{};
    auto presenceIt = element.find("presence");
    if (presenceIt != element.end())
    {
        presence = parseString(*presenceIt, false, variables);
        ++propertyCount;
    }

    // Optional page property
    std::optional<uint8_t> page{};
    auto pageIt = element.find("page");
    if (pageIt != element.end())
    {
        page = parseUint8(*pageIt, variables);
        ++propertyCount;
    }

    // Optional is_power_supply_rail property
    bool isPowerSupplyRail{false};
    auto isPowerSupplyRailIt = element.find("is_power_supply_rail");
    if (isPowerSupplyRailIt != element.end())
    {
        isPowerSupplyRail = parseBoolean(*isPowerSupplyRailIt, variables);
        ++propertyCount;
    }

    // Optional check_status_vout property
    bool checkStatusVout{false};
    auto checkStatusVoutIt = element.find("check_status_vout");
    if (checkStatusVoutIt != element.end())
    {
        checkStatusVout = parseBoolean(*checkStatusVoutIt, variables);
        ++propertyCount;
    }

    // Optional compare_voltage_to_limit property
    bool compareVoltageToLimit{false};
    auto compareVoltageToLimitIt = element.find("compare_voltage_to_limit");
    if (compareVoltageToLimitIt != element.end())
    {
        compareVoltageToLimit =
            parseBoolean(*compareVoltageToLimitIt, variables);
        ++propertyCount;
    }

    // Optional gpio property
    std::optional<GPIO> gpio{};
    auto gpioIt = element.find("gpio");
    if (gpioIt != element.end())
    {
        gpio = parseGPIO(*gpioIt, variables);
        ++propertyCount;
    }

    // If check_status_vout or compare_voltage_to_limit property is true, the
    // page property is required; verify page was specified
    if ((checkStatusVout || compareVoltageToLimit) && !page.has_value())
    {
        throw std::invalid_argument{"Required property missing: page"};
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Rail>(name, presence, page, isPowerSupplyRail,
                                  checkStatusVout, compareVoltageToLimit, gpio);
}

std::vector<std::unique_ptr<Rail>> parseRailArray(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Rail>> rails;
    for (auto& railElement : element)
    {
        rails.emplace_back(parseRail(railElement, variables));
    }
    return rails;
}

std::vector<std::unique_ptr<Rail>> parseRoot(const json& element)
{
    std::map<std::string, std::string> variables{};

    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required rails property
    const json& railsElement = getRequiredProperty(element, "rails");
    std::vector<std::unique_ptr<Rail>> rails =
        parseRailArray(railsElement, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return rails;
}

} // namespace internal

} // namespace phosphor::power::sequencer::config_file_parser
