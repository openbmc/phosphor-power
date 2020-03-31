/**
 * Copyright © 2020 IBM Corporation
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
#include "i2c_interface.hpp"
#include "pmbus_utils.hpp"

#include <exception>
#include <fstream>
#include <optional>
#include <utility>

using json = nlohmann::json;

namespace phosphor::power::regulators::config_file_parser
{

std::tuple<std::vector<std::unique_ptr<Rule>>,
           std::vector<std::unique_ptr<Chassis>>>
    parse(const std::filesystem::path& pathName)
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

std::unique_ptr<Action> parseAction(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Required action type property; there must be exactly one specified
    std::unique_ptr<Action> action{};
    if (element.contains("and"))
    {
        // TODO: Not implemented yet
        // action = parseAnd(element["and"]);
        // ++propertyCount;
    }
    else if (element.contains("compare_presence"))
    {
        // TODO: Not implemented yet
        // action = parseComparePresence(element["compare_presence"]);
        // ++propertyCount;
    }
    else if (element.contains("compare_vpd"))
    {
        // TODO: Not implemented yet
        // action = parseCompareVPD(element["compare_vpd"]);
        // ++propertyCount;
    }
    else if (element.contains("i2c_compare_bit"))
    {
        // TODO: Not implemented yet
        // action = parseI2CCompareBit(element["i2c_compare_bit"]);
        // ++propertyCount;
    }
    else if (element.contains("i2c_compare_byte"))
    {
        // TODO: Not implemented yet
        // action = parseI2CCompareByte(element["i2c_compare_byte"]);
        // ++propertyCount;
    }
    else if (element.contains("i2c_compare_bytes"))
    {
        // TODO: Not implemented yet
        // action = parseI2CCompareBytes(element["i2c_compare_bytes"]);
        // ++propertyCount;
    }
    else if (element.contains("i2c_write_bit"))
    {
        action = parseI2CWriteBit(element["i2c_write_bit"]);
        ++propertyCount;
    }
    else if (element.contains("i2c_write_byte"))
    {
        action = parseI2CWriteByte(element["i2c_write_byte"]);
        ++propertyCount;
    }
    else if (element.contains("i2c_write_bytes"))
    {
        // TODO: Not implemented yet
        // action = parseI2CWriteBytes(element["i2c_write_bytes"]);
        // ++propertyCount;
    }
    else if (element.contains("if"))
    {
        // TODO: Not implemented yet
        // action = parseIf(element["if"]);
        // ++propertyCount;
    }
    else if (element.contains("not"))
    {
        // TODO: Not implemented yet
        // action = parseNot(element["not"]);
        // ++propertyCount;
    }
    else if (element.contains("or"))
    {
        // TODO: Not implemented yet
        // action = parseOr(element["or"]);
        // ++propertyCount;
    }
    else if (element.contains("pmbus_read_sensor"))
    {
        // TODO: Not implemented yet
        // action = parsePMBusReadSensor(element["pmbus_read_sensor"]);
        // ++propertyCount;
    }
    else if (element.contains("pmbus_write_vout_command"))
    {
        action =
            parsePMBusWriteVoutCommand(element["pmbus_write_vout_command"]);
        ++propertyCount;
    }
    else if (element.contains("run_rule"))
    {
        // TODO: Not implemented yet
        // action = parseRunRule(element["run_rule"]);
        // ++propertyCount;
    }
    else if (element.contains("set_device"))
    {
        // TODO: Not implemented yet
        // action = parseSetDevice(element["set_device"]);
        // ++propertyCount;
    }
    else
    {
        throw std::invalid_argument{"Required action type property missing"};
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return action;
}

std::vector<std::unique_ptr<Action>> parseActionArray(const json& element)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Action>> actions;
    for (auto& actionElement : element)
    {
        actions.emplace_back(parseAction(actionElement));
    }
    return actions;
}

std::vector<std::unique_ptr<Chassis>> parseChassisArray(const json& element)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Chassis>> chassis;
    // TODO: Not implemented yet
    // for (auto& chassisElement : element)
    // {
    //     chassis.emplace_back(parseChassis(chassisElement));
    // }
    return chassis;
}

std::unique_ptr<I2CWriteBitAction> parseI2CWriteBit(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseStringToUint8(regElement);
    ++propertyCount;

    // Required position property
    const json& positionElement = getRequiredProperty(element, "position");
    uint8_t position = parseBitPosition(positionElement);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    uint8_t value = parseBitValue(valueElement);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CWriteBitAction>(reg, position, value);
}

std::unique_ptr<I2CWriteByteAction> parseI2CWriteByte(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseStringToUint8(regElement);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    uint8_t value = parseStringToUint8(valueElement);
    ++propertyCount;

    // Optional mask property
    uint8_t mask = 0xff;
    auto maskIt = element.find("mask");
    if (maskIt != element.end())
    {
        mask = parseStringToUint8(*maskIt);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CWriteByteAction>(reg, value, mask);
}

std::unique_ptr<PMBusWriteVoutCommandAction>
    parsePMBusWriteVoutCommand(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional volts property
    std::optional<double> volts{};
    auto voltsIt = element.find("volts");
    if (voltsIt != element.end())
    {
        volts = parseDouble(*voltsIt);
        ++propertyCount;
    }

    // Required format property
    const json& formatElement = getRequiredProperty(element, "format");
    std::string formatString = parseString(formatElement);
    if (formatString != "linear")
    {
        throw std::invalid_argument{"Invalid format value: " + formatString};
    }
    pmbus_utils::VoutDataFormat format = pmbus_utils::VoutDataFormat::linear;
    ++propertyCount;

    // Optional exponent property
    std::optional<int8_t> exponent{};
    auto exponentIt = element.find("exponent");
    if (exponentIt != element.end())
    {
        exponent = parseInt8(*exponentIt);
        ++propertyCount;
    }

    // Optional is_verified property
    bool isVerified = false;
    auto isVerifiedIt = element.find("is_verified");
    if (isVerifiedIt != element.end())
    {
        isVerified = parseBoolean(*isVerifiedIt);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<PMBusWriteVoutCommandAction>(volts, format,
                                                         exponent, isVerified);
}

std::tuple<std::vector<std::unique_ptr<Rule>>,
           std::vector<std::unique_ptr<Chassis>>>
    parseRoot(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Optional rules property
    std::vector<std::unique_ptr<Rule>> rules{};
    auto rulesIt = element.find("rules");
    if (rulesIt != element.end())
    {
        rules = parseRuleArray(*rulesIt);
        ++propertyCount;
    }

    // Required chassis property
    const json& chassisElement = getRequiredProperty(element, "chassis");
    std::vector<std::unique_ptr<Chassis>> chassis =
        parseChassisArray(chassisElement);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_tuple(std::move(rules), std::move(chassis));
}

std::unique_ptr<Rule> parseRule(const json& element)
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

    // Required actions property
    const json& actionsElement = getRequiredProperty(element, "actions");
    std::vector<std::unique_ptr<Action>> actions =
        parseActionArray(actionsElement);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Rule>(id, std::move(actions));
}

std::vector<std::unique_ptr<Rule>> parseRuleArray(const json& element)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Rule>> rules;
    for (auto& ruleElement : element)
    {
        rules.emplace_back(parseRule(ruleElement));
    }
    return rules;
}

} // namespace internal

} // namespace phosphor::power::regulators::config_file_parser
