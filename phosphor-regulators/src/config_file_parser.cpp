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
#include "json_parser_utils.hpp"
#include "pmbus_utils.hpp"

#include <cstdint>
#include <exception>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <utility>

using json = nlohmann::json;
using ConfigFileParserError = phosphor::power::util::ConfigFileParserError;
using namespace phosphor::power::json_parser_utils;

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

std::unique_ptr<Action> parseAction(
    const json& element, const std::map<std::string, std::string>& variables)
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
        action = parseAnd(element["and"], variables);
        ++propertyCount;
    }
    else if (element.contains("compare_presence"))
    {
        action = parseComparePresence(element["compare_presence"], variables);
        ++propertyCount;
    }
    else if (element.contains("compare_vpd"))
    {
        action = parseCompareVPD(element["compare_vpd"], variables);
        ++propertyCount;
    }
    else if (element.contains("i2c_capture_bytes"))
    {
        action = parseI2CCaptureBytes(element["i2c_capture_bytes"], variables);
        ++propertyCount;
    }
    else if (element.contains("i2c_compare_bit"))
    {
        action = parseI2CCompareBit(element["i2c_compare_bit"], variables);
        ++propertyCount;
    }
    else if (element.contains("i2c_compare_byte"))
    {
        action = parseI2CCompareByte(element["i2c_compare_byte"], variables);
        ++propertyCount;
    }
    else if (element.contains("i2c_compare_bytes"))
    {
        action = parseI2CCompareBytes(element["i2c_compare_bytes"], variables);
        ++propertyCount;
    }
    else if (element.contains("i2c_write_bit"))
    {
        action = parseI2CWriteBit(element["i2c_write_bit"], variables);
        ++propertyCount;
    }
    else if (element.contains("i2c_write_byte"))
    {
        action = parseI2CWriteByte(element["i2c_write_byte"], variables);
        ++propertyCount;
    }
    else if (element.contains("i2c_write_bytes"))
    {
        action = parseI2CWriteBytes(element["i2c_write_bytes"], variables);
        ++propertyCount;
    }
    else if (element.contains("if"))
    {
        action = parseIf(element["if"], variables);
        ++propertyCount;
    }
    else if (element.contains("log_phase_fault"))
    {
        action = parseLogPhaseFault(element["log_phase_fault"], variables);
        ++propertyCount;
    }
    else if (element.contains("not"))
    {
        action = parseNot(element["not"], variables);
        ++propertyCount;
    }
    else if (element.contains("or"))
    {
        action = parseOr(element["or"], variables);
        ++propertyCount;
    }
    else if (element.contains("pmbus_read_sensor"))
    {
        action = parsePMBusReadSensor(element["pmbus_read_sensor"], variables);
        ++propertyCount;
    }
    else if (element.contains("pmbus_write_vout_command"))
    {
        action = parsePMBusWriteVoutCommand(element["pmbus_write_vout_command"],
                                            variables);
        ++propertyCount;
    }
    else if (element.contains("run_rule"))
    {
        action = parseRunRule(element["run_rule"], variables);
        ++propertyCount;
    }
    else if (element.contains("set_device"))
    {
        action = parseSetDevice(element["set_device"], variables);
        ++propertyCount;
    }
    else
    {
        throw std::invalid_argument{"Required action type property missing"};
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return action;
}

std::vector<std::unique_ptr<Action>> parseActionArray(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Action>> actions;
    for (auto& actionElement : element)
    {
        actions.emplace_back(parseAction(actionElement, variables));
    }
    return actions;
}

std::unique_ptr<AndAction> parseAnd(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsArray(element);

    // Verify if array size less than 2
    if (element.size() < 2)
    {
        throw std::invalid_argument{"Array must contain two or more actions"};
    }
    // Array of two or more actions
    std::vector<std::unique_ptr<Action>> actions =
        parseActionArray(element, variables);

    return std::make_unique<AndAction>(std::move(actions));
}

std::unique_ptr<Chassis> parseChassis(
    const json& element,
    const std::map<std::string, JSONRefWrapper>& chassisTemplates)
{
    verifyIsObject(element);

    // If chassis object is not using a template, parse properties normally
    if (!element.contains("template_id"))
    {
        bool isChassisTemplate{false};
        return parseChassisProperties(element, isChassisTemplate, NO_VARIABLES);
    }

    // Parse chassis object that is using a template
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Required template_id property
    const json& templateIDElement = getRequiredProperty(element, "template_id");
    std::string templateID = parseString(templateIDElement);
    ++propertyCount;

    // Required template_variable_values property
    const json& variablesElement =
        getRequiredProperty(element, "template_variable_values");
    std::map<std::string, std::string> variables =
        parseVariables(variablesElement);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    // Get reference to chassis template JSON
    auto it = chassisTemplates.find(templateID);
    if (it == chassisTemplates.end())
    {
        throw std::invalid_argument{
            "Invalid chassis template id: " + templateID};
    }
    const json& templateElement = it->second.get();

    // Parse properties in template using variable values for this chassis
    bool isChassisTemplate{true};
    return parseChassisProperties(templateElement, isChassisTemplate,
                                  variables);
}

std::vector<std::unique_ptr<Chassis>> parseChassisArray(
    const json& element,
    const std::map<std::string, JSONRefWrapper>& chassisTemplates)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Chassis>> chassis;
    for (auto& chassisElement : element)
    {
        chassis.emplace_back(parseChassis(chassisElement, chassisTemplates));
    }
    return chassis;
}

std::unique_ptr<Chassis> parseChassisProperties(
    const json& element, bool isChassisTemplate,
    const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Required id property if this is a chassis template
    // Don't parse again; this was already parsed by parseChassisTemplate()
    if (isChassisTemplate)
    {
        getRequiredProperty(element, "id");
        ++propertyCount;
    }

    // Required number property
    const json& numberElement = getRequiredProperty(element, "number");
    unsigned int number = parseUnsignedInteger(numberElement, variables);
    if (number < 1)
    {
        throw std::invalid_argument{"Invalid chassis number: Must be > 0"};
    }
    ++propertyCount;

    // Required inventory_path property
    const json& inventoryPathElement =
        getRequiredProperty(element, "inventory_path");
    std::string inventoryPath =
        parseInventoryPath(inventoryPathElement, variables);
    ++propertyCount;

    // Optional devices property
    std::vector<std::unique_ptr<Device>> devices{};
    auto devicesIt = element.find("devices");
    if (devicesIt != element.end())
    {
        devices = parseDeviceArray(*devicesIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Chassis>(number, inventoryPath, std::move(devices));
}

std::tuple<std::string, JSONRefWrapper> parseChassisTemplate(
    const json& element)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not parsed
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

    // Optional devices property
    // Cannot be parsed without variable values
    if (element.contains("devices"))
    {
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return {id, JSONRefWrapper{element}};
}

std::map<std::string, JSONRefWrapper> parseChassisTemplateArray(
    const json& element)
{
    verifyIsArray(element);
    std::map<std::string, JSONRefWrapper> chassisTemplates;
    for (auto& chassisTemplateElement : element)
    {
        chassisTemplates.emplace(parseChassisTemplate(chassisTemplateElement));
    }
    return chassisTemplates;
}

std::unique_ptr<ComparePresenceAction> parseComparePresence(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required fru property
    const json& fruElement = getRequiredProperty(element, "fru");
    std::string fru = parseInventoryPath(fruElement, variables);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    bool value = parseBoolean(valueElement, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<ComparePresenceAction>(fru, value);
}

std::unique_ptr<CompareVPDAction> parseCompareVPD(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required fru property
    const json& fruElement = getRequiredProperty(element, "fru");
    std::string fru = parseInventoryPath(fruElement, variables);
    ++propertyCount;

    // Required keyword property
    const json& keywordElement = getRequiredProperty(element, "keyword");
    std::string keyword = parseString(keywordElement, false, variables);
    ++propertyCount;

    // Either value or byte_values required property
    auto valueIt = element.find("value");
    std::vector<uint8_t> value{};
    auto byteValuesIt = element.find("byte_values");
    if ((valueIt != element.end()) && (byteValuesIt == element.end()))
    {
        std::string stringValue = parseString(*valueIt, true, variables);
        value.insert(value.begin(), stringValue.begin(), stringValue.end());
        ++propertyCount;
    }
    else if ((valueIt == element.end()) && (byteValuesIt != element.end()))
    {
        value = parseHexByteArray(*byteValuesIt, variables);
        ++propertyCount;
    }
    else
    {
        throw std::invalid_argument{
            "Invalid property: Must contain either value or byte_values"};
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<CompareVPDAction>(fru, keyword, value);
}

std::unique_ptr<Configuration> parseConfiguration(
    const json& element, const std::map<std::string, std::string>& variables)
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
        volts = parseDouble(*voltsIt, variables);
        ++propertyCount;
    }

    // Required rule_id or actions property
    std::vector<std::unique_ptr<Action>> actions{};
    actions = parseRuleIDOrActionsProperty(element, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Configuration>(volts, std::move(actions));
}

std::unique_ptr<Device> parseDevice(
    const json& element, const std::map<std::string, std::string>& variables)
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
    std::string id = parseString(idElement, false, variables);
    ++propertyCount;

    // Required is_regulator property
    const json& isRegulatorElement =
        getRequiredProperty(element, "is_regulator");
    bool isRegulator = parseBoolean(isRegulatorElement, variables);
    ++propertyCount;

    // Required fru property
    const json& fruElement = getRequiredProperty(element, "fru");
    std::string fru = parseInventoryPath(fruElement, variables);
    ++propertyCount;

    // Required i2c_interface property
    const json& i2cInterfaceElement =
        getRequiredProperty(element, "i2c_interface");
    std::unique_ptr<i2c::I2CInterface> i2cInterface =
        parseI2CInterface(i2cInterfaceElement, variables);
    ++propertyCount;

    // Optional presence_detection property
    std::unique_ptr<PresenceDetection> presenceDetection{};
    auto presenceDetectionIt = element.find("presence_detection");
    if (presenceDetectionIt != element.end())
    {
        presenceDetection =
            parsePresenceDetection(*presenceDetectionIt, variables);
        ++propertyCount;
    }

    // Optional configuration property
    std::unique_ptr<Configuration> configuration{};
    auto configurationIt = element.find("configuration");
    if (configurationIt != element.end())
    {
        configuration = parseConfiguration(*configurationIt, variables);
        ++propertyCount;
    }

    // Optional phase_fault_detection property
    std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
    auto phaseFaultDetectionIt = element.find("phase_fault_detection");
    if (phaseFaultDetectionIt != element.end())
    {
        if (!isRegulator)
        {
            throw std::invalid_argument{"Invalid phase_fault_detection "
                                        "property when is_regulator is false"};
        }
        phaseFaultDetection =
            parsePhaseFaultDetection(*phaseFaultDetectionIt, variables);
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
        rails = parseRailArray(*railsIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Device>(
        id, isRegulator, fru, std::move(i2cInterface),
        std::move(presenceDetection), std::move(configuration),
        std::move(phaseFaultDetection), std::move(rails));
}

std::vector<std::unique_ptr<Device>> parseDeviceArray(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsArray(element);
    std::vector<std::unique_ptr<Device>> devices;
    for (auto& deviceElement : element)
    {
        devices.emplace_back(parseDevice(deviceElement, variables));
    }
    return devices;
}

std::unique_ptr<I2CCaptureBytesAction> parseI2CCaptureBytes(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement, variables);
    ++propertyCount;

    // Required count property
    const json& countElement = getRequiredProperty(element, "count");
    uint8_t count = parseUint8(countElement, variables);
    if (count < 1)
    {
        throw std::invalid_argument{"Invalid byte count: Must be > 0"};
    }
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CCaptureBytesAction>(reg, count);
}

std::unique_ptr<I2CCompareBitAction> parseI2CCompareBit(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement, variables);
    ++propertyCount;

    // Required position property
    const json& positionElement = getRequiredProperty(element, "position");
    uint8_t position = parseBitPosition(positionElement, variables);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    uint8_t value = parseBitValue(valueElement, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CCompareBitAction>(reg, position, value);
}

std::unique_ptr<I2CCompareByteAction> parseI2CCompareByte(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement, variables);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    uint8_t value = parseHexByte(valueElement, variables);
    ++propertyCount;

    // Optional mask property
    uint8_t mask = 0xff;
    auto maskIt = element.find("mask");
    if (maskIt != element.end())
    {
        mask = parseHexByte(*maskIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CCompareByteAction>(reg, value, mask);
}

std::unique_ptr<I2CCompareBytesAction> parseI2CCompareBytes(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement, variables);
    ++propertyCount;

    // Required values property
    const json& valueElement = getRequiredProperty(element, "values");
    std::vector<uint8_t> values = parseHexByteArray(valueElement, variables);
    ++propertyCount;

    // Optional masks property
    std::vector<uint8_t> masks{};
    auto masksIt = element.find("masks");
    if (masksIt != element.end())
    {
        masks = parseHexByteArray(*masksIt, variables);
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

std::unique_ptr<i2c::I2CInterface> parseI2CInterface(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required bus property
    const json& busElement = getRequiredProperty(element, "bus");
    uint16_t bus = parseUint16(busElement, variables);
    ++propertyCount;

    // Required address property
    const json& addressElement = getRequiredProperty(element, "address");
    uint8_t address = parseHexByte(addressElement, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    // Create I2CInterface object; retry failed I2C operations a max of 3 times.
    int maxRetries{3};
    return i2c::create(bus, address, i2c::I2CInterface::InitialState::CLOSED,
                       maxRetries);
}

std::unique_ptr<I2CWriteBitAction> parseI2CWriteBit(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement, variables);
    ++propertyCount;

    // Required position property
    const json& positionElement = getRequiredProperty(element, "position");
    uint8_t position = parseBitPosition(positionElement, variables);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    uint8_t value = parseBitValue(valueElement, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CWriteBitAction>(reg, position, value);
}

std::unique_ptr<I2CWriteByteAction> parseI2CWriteByte(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement, variables);
    ++propertyCount;

    // Required value property
    const json& valueElement = getRequiredProperty(element, "value");
    uint8_t value = parseHexByte(valueElement, variables);
    ++propertyCount;

    // Optional mask property
    uint8_t mask = 0xff;
    auto maskIt = element.find("mask");
    if (maskIt != element.end())
    {
        mask = parseHexByte(*maskIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<I2CWriteByteAction>(reg, value, mask);
}

std::unique_ptr<I2CWriteBytesAction> parseI2CWriteBytes(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required register property
    const json& regElement = getRequiredProperty(element, "register");
    uint8_t reg = parseHexByte(regElement, variables);
    ++propertyCount;

    // Required values property
    const json& valueElement = getRequiredProperty(element, "values");
    std::vector<uint8_t> values = parseHexByteArray(valueElement, variables);
    ++propertyCount;

    // Optional masks property
    std::vector<uint8_t> masks{};
    auto masksIt = element.find("masks");
    if (masksIt != element.end())
    {
        masks = parseHexByteArray(*masksIt, variables);
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

std::unique_ptr<IfAction> parseIf(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required condition property
    const json& conditionElement = getRequiredProperty(element, "condition");
    std::unique_ptr<Action> conditionAction =
        parseAction(conditionElement, variables);
    ++propertyCount;

    // Required then property
    const json& thenElement = getRequiredProperty(element, "then");
    std::vector<std::unique_ptr<Action>> thenActions =
        parseActionArray(thenElement, variables);
    ++propertyCount;

    // Optional else property
    std::vector<std::unique_ptr<Action>> elseActions{};
    auto elseIt = element.find("else");
    if (elseIt != element.end())
    {
        elseActions = parseActionArray(*elseIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<IfAction>(std::move(conditionAction),
                                      std::move(thenActions),
                                      std::move(elseActions));
}

std::string parseInventoryPath(
    const json& element, const std::map<std::string, std::string>& variables)
{
    std::string inventoryPath = parseString(element, false, variables);
    std::string absPath = "/xyz/openbmc_project/inventory";
    if (inventoryPath.front() != '/')
    {
        absPath += '/';
    }
    absPath += inventoryPath;
    return absPath;
}

std::unique_ptr<LogPhaseFaultAction> parseLogPhaseFault(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required type property
    const json& typeElement = getRequiredProperty(element, "type");
    PhaseFaultType type = parsePhaseFaultType(typeElement, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<LogPhaseFaultAction>(type);
}

std::unique_ptr<NotAction> parseNot(
    const json& element, const std::map<std::string, std::string>& variables)
{
    // Required action to execute
    std::unique_ptr<Action> action = parseAction(element, variables);

    return std::make_unique<NotAction>(std::move(action));
}

std::unique_ptr<OrAction> parseOr(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsArray(element);

    // Verify if array size less than 2
    if (element.size() < 2)
    {
        throw std::invalid_argument{"Array must contain two or more actions"};
    }
    // Array of two or more actions
    std::vector<std::unique_ptr<Action>> actions =
        parseActionArray(element, variables);

    return std::make_unique<OrAction>(std::move(actions));
}

std::unique_ptr<PhaseFaultDetection> parsePhaseFaultDetection(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional comments property; value not stored
    if (element.contains("comments"))
    {
        ++propertyCount;
    }

    // Optional device_id property
    std::string deviceID{};
    auto deviceIDIt = element.find("device_id");
    if (deviceIDIt != element.end())
    {
        deviceID = parseString(*deviceIDIt, false, variables);
        ++propertyCount;
    }

    // Required rule_id or actions property
    std::vector<std::unique_ptr<Action>> actions{};
    actions = parseRuleIDOrActionsProperty(element, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<PhaseFaultDetection>(std::move(actions), deviceID);
}

PhaseFaultType parsePhaseFaultType(
    const json& element, const std::map<std::string, std::string>& variables)
{
    std::string value = parseString(element, false, variables);
    PhaseFaultType type{};

    if (value == "n")
    {
        type = PhaseFaultType::n;
    }
    else if (value == "n+1")
    {
        type = PhaseFaultType::n_plus_1;
    }
    else
    {
        throw std::invalid_argument{"Element is not a phase fault type"};
    }

    return type;
}

std::unique_ptr<PMBusReadSensorAction> parsePMBusReadSensor(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Required type property
    const json& typeElement = getRequiredProperty(element, "type");
    SensorType type = parseSensorType(typeElement, variables);
    ++propertyCount;

    // Required command property
    const json& commandElement = getRequiredProperty(element, "command");
    uint8_t command = parseHexByte(commandElement, variables);
    ++propertyCount;

    // Required format property
    const json& formatElement = getRequiredProperty(element, "format");
    pmbus_utils::SensorDataFormat format =
        parseSensorDataFormat(formatElement, variables);
    ++propertyCount;

    // Optional exponent property
    std::optional<int8_t> exponent{};
    auto exponentIt = element.find("exponent");
    if (exponentIt != element.end())
    {
        exponent = parseInt8(*exponentIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<PMBusReadSensorAction>(type, command, format,
                                                   exponent);
}

std::unique_ptr<PMBusWriteVoutCommandAction> parsePMBusWriteVoutCommand(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    unsigned int propertyCount{0};

    // Optional volts property
    std::optional<double> volts{};
    auto voltsIt = element.find("volts");
    if (voltsIt != element.end())
    {
        volts = parseDouble(*voltsIt, variables);
        ++propertyCount;
    }

    // Required format property
    const json& formatElement = getRequiredProperty(element, "format");
    std::string formatString = parseString(formatElement, false, variables);
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
        exponent = parseInt8(*exponentIt, variables);
        ++propertyCount;
    }

    // Optional is_verified property
    bool isVerified = false;
    auto isVerifiedIt = element.find("is_verified");
    if (isVerifiedIt != element.end())
    {
        isVerified = parseBoolean(*isVerifiedIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<PMBusWriteVoutCommandAction>(volts, format,
                                                         exponent, isVerified);
}

std::unique_ptr<PresenceDetection> parsePresenceDetection(
    const json& element, const std::map<std::string, std::string>& variables)
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
    actions = parseRuleIDOrActionsProperty(element, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<PresenceDetection>(std::move(actions));
}

std::unique_ptr<Rail> parseRail(
    const json& element, const std::map<std::string, std::string>& variables)
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
    std::string id = parseString(idElement, false, variables);
    ++propertyCount;

    // Optional configuration property
    std::unique_ptr<Configuration> configuration{};
    auto configurationIt = element.find("configuration");
    if (configurationIt != element.end())
    {
        configuration = parseConfiguration(*configurationIt, variables);
        ++propertyCount;
    }

    // Optional sensor_monitoring property
    std::unique_ptr<SensorMonitoring> sensorMonitoring{};
    auto sensorMonitoringIt = element.find("sensor_monitoring");
    if (sensorMonitoringIt != element.end())
    {
        sensorMonitoring =
            parseSensorMonitoring(*sensorMonitoringIt, variables);
        ++propertyCount;
    }

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<Rail>(id, std::move(configuration),
                                  std::move(sensorMonitoring));
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

    // Optional chassis_templates property
    std::map<std::string, JSONRefWrapper> chassisTemplates{};
    auto chassisTemplatesIt = element.find("chassis_templates");
    if (chassisTemplatesIt != element.end())
    {
        chassisTemplates = parseChassisTemplateArray(*chassisTemplatesIt);
        ++propertyCount;
    }

    // Required chassis property
    const json& chassisElement = getRequiredProperty(element, "chassis");
    std::vector<std::unique_ptr<Chassis>> chassis =
        parseChassisArray(chassisElement, chassisTemplates);
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
        parseActionArray(actionsElement, NO_VARIABLES);
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

std::vector<std::unique_ptr<Action>> parseRuleIDOrActionsProperty(
    const json& element, const std::map<std::string, std::string>& variables)
{
    verifyIsObject(element);
    // Required rule_id or actions property
    std::vector<std::unique_ptr<Action>> actions{};
    auto ruleIDIt = element.find("rule_id");
    auto actionsIt = element.find("actions");
    if ((actionsIt == element.end()) && (ruleIDIt != element.end()))
    {
        std::string ruleID = parseString(*ruleIDIt, false, variables);
        actions.emplace_back(std::make_unique<RunRuleAction>(ruleID));
    }
    else if ((actionsIt != element.end()) && (ruleIDIt == element.end()))
    {
        actions = parseActionArray(*actionsIt, variables);
    }
    else
    {
        throw std::invalid_argument{"Invalid property combination: Must "
                                    "contain either rule_id or actions"};
    }

    return actions;
}

std::unique_ptr<RunRuleAction> parseRunRule(
    const json& element, const std::map<std::string, std::string>& variables)
{
    // String ruleID
    std::string ruleID = parseString(element, false, variables);

    return std::make_unique<RunRuleAction>(ruleID);
}

pmbus_utils::SensorDataFormat parseSensorDataFormat(
    const json& element, const std::map<std::string, std::string>& variables)
{
    std::string value = parseString(element, false, variables);
    pmbus_utils::SensorDataFormat format{};

    if (value == "linear_11")
    {
        format = pmbus_utils::SensorDataFormat::linear_11;
    }
    else if (value == "linear_16")
    {
        format = pmbus_utils::SensorDataFormat::linear_16;
    }
    else
    {
        throw std::invalid_argument{"Element is not a sensor data format"};
    }

    return format;
}

std::unique_ptr<SensorMonitoring> parseSensorMonitoring(
    const json& element, const std::map<std::string, std::string>& variables)
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
    actions = parseRuleIDOrActionsProperty(element, variables);
    ++propertyCount;

    // Verify no invalid properties exist
    verifyPropertyCount(element, propertyCount);

    return std::make_unique<SensorMonitoring>(std::move(actions));
}

SensorType parseSensorType(const json& element,
                           const std::map<std::string, std::string>& variables)
{
    std::string value = parseString(element, false, variables);
    SensorType type{};

    if (value == "iout")
    {
        type = SensorType::iout;
    }
    else if (value == "iout_peak")
    {
        type = SensorType::iout_peak;
    }
    else if (value == "iout_valley")
    {
        type = SensorType::iout_valley;
    }
    else if (value == "pout")
    {
        type = SensorType::pout;
    }
    else if (value == "temperature")
    {
        type = SensorType::temperature;
    }
    else if (value == "temperature_peak")
    {
        type = SensorType::temperature_peak;
    }
    else if (value == "vout")
    {
        type = SensorType::vout;
    }
    else if (value == "vout_peak")
    {
        type = SensorType::vout_peak;
    }
    else if (value == "vout_valley")
    {
        type = SensorType::vout_valley;
    }
    else
    {
        throw std::invalid_argument{"Element is not a sensor type"};
    }

    return type;
}

std::unique_ptr<SetDeviceAction> parseSetDevice(
    const json& element, const std::map<std::string, std::string>& variables)
{
    // String deviceID
    std::string deviceID = parseString(element, false, variables);

    return std::make_unique<SetDeviceAction>(deviceID);
}

std::map<std::string, std::string> parseVariables(const json& element)
{
    verifyIsObject(element);

    std::map<std::string, std::string> variables;
    std::string name, value;
    for (const auto& [nameElement, valueElement] : element.items())
    {
        name = parseString(nameElement);
        value = parseString(valueElement);
        variables.emplace(name, value);
    }
    return variables;
}

pmbus_utils::VoutDataFormat parseVoutDataFormat(
    const json& element, const std::map<std::string, std::string>& variables)
{
    std::string value = parseString(element, false, variables);
    pmbus_utils::VoutDataFormat format{};

    if (value == "linear")
    {
        format = pmbus_utils::VoutDataFormat::linear;
    }
    else if (value == "vid")
    {
        format = pmbus_utils::VoutDataFormat::vid;
    }
    else if (value == "direct")
    {
        format = pmbus_utils::VoutDataFormat::direct;
    }
    else if (value == "ieee")
    {
        format = pmbus_utils::VoutDataFormat::ieee;
    }
    else
    {
        throw std::invalid_argument{"Element is not a vout data format"};
    }

    return format;
}

} // namespace internal

} // namespace phosphor::power::regulators::config_file_parser
