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
        action = parseI2CCompareBit(element["i2c_compare_bit"]);
        ++propertyCount;
    }
    else if (element.contains("i2c_compare_byte"))
    {
        action = parseI2CCompareByte(element["i2c_compare_byte"]);
        ++propertyCount;
    }
    else if (element.contains("i2c_compare_bytes"))
    {
        action = parseI2CCompareBytes(element["i2c_compare_bytes"]);
        ++propertyCount;
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
        action = parseI2CWriteBytes(element["i2c_write_bytes"]);
        ++propertyCount;
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
        action = parseRunRule(element["run_rule"]);
        ++propertyCount;
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

std::unique_ptr<Chassis> parseChassis(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Required number property
    const json& numberElement = getRequiredProperty(element, "number");
    unsigned int number = parseUnsignedInteger(numberElement);
    if (number < 1)
    {
        throw std::invalid_argument{"Invalid chassis number: Must be > 0"};
    }
    ++propertyCount;

    // Optional devices property
    std::vector<std::unique_ptr<Device>> devices{};
    auto devicesIt = element.find("devices");
    if (devicesIt != element.end())
    {
        devices = parseDeviceArray(*devicesIt);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Chassis>(number, std::move(devices));
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

std::unique_ptr<Configuration> parseConfiguration(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Optional volts property
    std::optional<double> volts{};
    auto voltsIt = element.find("volts");
    if (voltsIt != element.end())
    {
        volts = parseDouble(*voltsIt);
        ++propertyCount;
    }

    // Required rule_id or actions property
    std::vector<std::unique_ptr<Action>> actions{};
    actions = parseRuleIDOrActionsProperty(element);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Configuration>(volts, std::move(actions));
}

std::unique_ptr<Device> parseDevice(const json& element)
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

    // Required is_regulator property
    const json& isRegulatorElement =
        getRequiredProperty(element, "is_regulator");
    bool isRegulator = parseBoolean(isRegulatorElement);
    ++propertyCount;

    // Required fru property
    const json& fruElement = getRequiredProperty(element, "fru");
    std::string fru = parseString(fruElement);
    ++propertyCount;

    // Required i2c_interface property
    const json& i2cInterfaceElement =
        getRequiredProperty(element, "i2c_interface");
    std::unique_ptr<i2c::I2CInterface> i2cInterface =
        parseI2CInterface(i2cInterfaceElement);
    ++propertyCount;

    // Optional presence_detection property
    // TODO: Not implemented yet
    std::unique_ptr<PresenceDetection> presenceDetection{};
    // auto presenceDetectionIt = element.find("presence_detection");
    // if (presenceDetectionIt != element.end())
    // {
    //     presenceDetection = parsePresenceDetection(*presenceDetectionIt);
    //     ++propertyCount;
    // }

    // Optional configuration property
    std::unique_ptr<Configuration> configuration{};
    auto configurationIt = element.find("configuration");
    if (configurationIt != element.end())
    {
        configuration = parseConfiguration(*configurationIt);
        ++propertyCount;
    }

    // Optional rails property
    std::vector<std::unique_ptr<Rail>> rails{};
    auto railsIt = element.find("rails");
    if (railsIt != element.end())
    {
        if (!isRegulator)
        {
            throw std::invalid_argument{
                "Invalid rails property when is_regulator is false"};
        }
        rails = parseRailArray(*railsIt);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Device>(id, isRegulator, fru,
                                    std::move(i2cInterface),
                                    std::move(presenceDetection),
                                    std::move(configuration), std::move(rails));
}

std::vector<std::unique_ptr<Device>> parseDeviceArray(const json& element)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Device>> devices;
    for (auto& deviceElement : element)
    {
        devices.emplace_back(parseDevice(deviceElement));
    }
    return devices;
}

std::vector<uint8_t> parseHexByteArray(const json& element)
{
    verifyIsArray(element);
    std::vector<uint8_t> values;
    for (auto& valueElement : element)
    {
        values.emplace_back(parseHexByte(valueElement));
    }
    return values;
}

std::unique_ptr<I2CCompareBitAction> parseI2CCompareBit(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement);
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

    return std::make_unique<I2CCompareBitAction>(reg, position, value);
}

std::unique_ptr<I2CCompareByteAction> parseI2CCompareByte(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    uint8_t value = parseHexByte(valueElement);
    ++propertyCount;

    // Optional mask property
    uint8_t mask = 0xff;
    auto maskIt = element.find("mask");
    if (maskIt != element.end())
    {
        mask = parseHexByte(*maskIt);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CCompareByteAction>(reg, value, mask);
}

std::unique_ptr<I2CCompareBytesAction> parseI2CCompareBytes(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement);
    ++propertyCount;

    // Required values property
    const json& valueElement = getRequiredProperty(element, "values");
    std::vector<uint8_t> values = parseHexByteArray(valueElement);
    ++propertyCount;

    // Optional masks property
    std::vector<uint8_t> masks{};
    auto masksIt = element.find("masks");
    if (masksIt != element.end())
    {
        masks = parseHexByteArray(*masksIt);
        ++propertyCount;
    }

    // Verify masks array (if specified) was same size as values array
    if ((!masks.empty()) && (masks.size() != values.size()))
    {
        throw std::invalid_argument{"Invalid number of elements in masks"};
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    if (masks.empty())
    {
        return std::make_unique<I2CCompareBytesAction>(reg, values);
    }
    return std::make_unique<I2CCompareBytesAction>(reg, values, masks);
}

std::unique_ptr<i2c::I2CInterface> parseI2CInterface(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required bus property
    const json& busElement = getRequiredProperty(element, "bus");
    uint8_t bus = parseUint8(busElement);
    ++propertyCount;

    // Required address property
    const json& addressElement = getRequiredProperty(element, "address");
    uint8_t address = parseHexByte(addressElement);
    ++propertyCount;

    verifyPropertyCount(element, propertyCount);
    return i2c::create(bus, address, i2c::I2CInterface::InitialState::CLOSED);
}

std::unique_ptr<I2CWriteBitAction> parseI2CWriteBit(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement);
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
    uint8_t reg = parseHexByte(regElement);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    uint8_t value = parseHexByte(valueElement);
    ++propertyCount;

    // Optional mask property
    uint8_t mask = 0xff;
    auto maskIt = element.find("mask");
    if (maskIt != element.end())
    {
        mask = parseHexByte(*maskIt);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CWriteByteAction>(reg, value, mask);
}

std::unique_ptr<I2CWriteBytesAction> parseI2CWriteBytes(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement);
    ++propertyCount;

    // Required values property
    const json& valueElement = getRequiredProperty(element, "values");
    std::vector<uint8_t> values = parseHexByteArray(valueElement);
    ++propertyCount;

    // Optional masks property
    std::vector<uint8_t> masks{};
    auto masksIt = element.find("masks");
    if (masksIt != element.end())
    {
        masks = parseHexByteArray(*masksIt);
        ++propertyCount;
    }

    // Verify masks array (if specified) was same size as values array
    if ((!masks.empty()) && (masks.size() != values.size()))
    {
        throw std::invalid_argument{"Invalid number of elements in masks"};
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    if (masks.empty())
    {
        return std::make_unique<I2CWriteBytesAction>(reg, values);
    }
    return std::make_unique<I2CWriteBytesAction>(reg, values, masks);
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

std::unique_ptr<Rail> parseRail(const json& element)
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

    // Optional configuration property
    std::unique_ptr<Configuration> configuration{};
    auto configurationIt = element.find("configuration");
    if (configurationIt != element.end())
    {
        configuration = parseConfiguration(*configurationIt);
        ++propertyCount;
    }

    // Optional sensor_monitoring property
    std::unique_ptr<SensorMonitoring> sensorMonitoring{};
    auto sensorMonitoringIt = element.find("sensor_monitoring");
    if (sensorMonitoringIt != element.end())
    {
        sensorMonitoring = parseSensorMonitoring(*sensorMonitoringIt);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Rail>(id, std::move(configuration),
                                  std::move(sensorMonitoring));
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

std::vector<std::unique_ptr<Action>>
    parseRuleIDOrActionsProperty(const json& element)
{
    verifyIsObject(element);
    // Required rule_id or actions property
    std::vector<std::unique_ptr<Action>> actions{};
    auto ruleIDIt = element.find("rule_id");
    auto actionsIt = element.find("actions");
    if ((actionsIt == element.end()) && (ruleIDIt != element.end()))
    {
        std::string ruleID = parseString(*ruleIDIt);
        actions.emplace_back(std::make_unique<RunRuleAction>(ruleID));
    }
    else if ((actionsIt != element.end()) && (ruleIDIt == element.end()))
    {
        actions = parseActionArray(*actionsIt);
    }
    else
    {
        throw std::invalid_argument{"Invalid property combination: Must "
                                    "contain either rule_id or actions"};
    }

    return actions;
}

std::unique_ptr<RunRuleAction> parseRunRule(const json& element)
{
    // String ruleID
    std::string ruleID = parseString(element);

    return std::make_unique<RunRuleAction>(ruleID);
}

std::unique_ptr<SensorMonitoring> parseSensorMonitoring(const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Required rule_id or actions property
    std::vector<std::unique_ptr<Action>> actions{};
    actions = parseRuleIDOrActionsProperty(element);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<SensorMonitoring>(std::move(actions));
}

} // namespace internal

} // namespace phosphor::power::regulators::config_file_parser
