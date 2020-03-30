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
#include "action.hpp"
#include "chassis.hpp"
#include "config_file_parser.hpp"
#include "config_file_parser_error.hpp"
#include "i2c_interface.hpp"
#include "i2c_write_bit_action.hpp"
#include "pmbus_utils.hpp"
#include "pmbus_write_vout_command_action.hpp"
#include "rule.hpp"
#include "tmp_file.hpp"

#include <sys/stat.h> // for chmod()

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::config_file_parser;
using namespace phosphor::power::regulators::config_file_parser::internal;
using json = nlohmann::json;

void writeConfigFile(const std::filesystem::path& pathName,
                     const std::string& contents)
{
    std::ofstream file{pathName};
    file << contents;
}

void writeConfigFile(const std::filesystem::path& pathName,
                     const json& contents)
{
    std::ofstream file{pathName};
    file << contents;
}

TEST(ConfigFileParserTests, Parse)
{
    // Test where works
    {
        const json configFileContents = R"(
            {
              "rules": [
                {
                  "id": "set_voltage_rule1",
                  "actions": [
                    { "pmbus_write_vout_command": { "volts": 1.03, "format": "linear" } }
                  ]
                },
                {
                  "id": "set_voltage_rule2",
                  "actions": [
                    { "pmbus_write_vout_command": { "volts": 1.33, "format": "linear" } }
                  ]
                }
              ],
              "chassis": [
                { "number": 1 },
                { "number": 2 },
                { "number": 3 }
              ]
            }
        )"_json;

        TmpFile configFile;
        std::filesystem::path pathName{configFile.getName()};
        writeConfigFile(pathName, configFileContents);

        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassis{};
        std::tie(rules, chassis) = parse(pathName);

        EXPECT_EQ(rules.size(), 2);
        EXPECT_EQ(rules[0]->getID(), "set_voltage_rule1");
        EXPECT_EQ(rules[1]->getID(), "set_voltage_rule2");

        // TODO: Not implemented yet
        // EXPECT_EQ(chassis.size(), 3);
        // EXPECT_EQ(chassis[0]->getNumber(), 1);
        // EXPECT_EQ(chassis[1]->getNumber(), 2);
        // EXPECT_EQ(chassis[2]->getNumber(), 3);
    }

    // Test where fails: File does not exist
    try
    {
        std::filesystem::path pathName{"/tmp/non_existent_file"};
        parse(pathName);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ConfigFileParserError& e)
    {
        // Expected exception; what() message will vary
    }

    // Test where fails: File is not readable
    try
    {
        const json configFileContents = R"(
            {
              "chassis": [ { "number": 1 } ]
            }
        )"_json;

        TmpFile configFile;
        std::filesystem::path pathName{configFile.getName()};
        writeConfigFile(pathName, configFileContents);

        chmod(pathName.c_str(), 0222);

        parse(pathName);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ConfigFileParserError& e)
    {
        // Expected exception; what() message will vary
    }

    // Test where fails: File is not valid JSON
    try
    {
        const std::string configFileContents = "] foo [";

        TmpFile configFile;
        std::filesystem::path pathName{configFile.getName()};
        writeConfigFile(pathName, configFileContents);

        parse(pathName);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ConfigFileParserError& e)
    {
        // Expected exception; what() message will vary
    }

    // Test where fails: Error when parsing JSON elements
    try
    {
        const json configFileContents = R"( { "foo": "bar" } )"_json;

        TmpFile configFile;
        std::filesystem::path pathName{configFile.getName()};
        writeConfigFile(pathName, configFileContents);

        parse(pathName);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ConfigFileParserError& e)
    {
        // Expected exception; what() message will vary
    }
}

TEST(ConfigFileParserTests, GetRequiredProperty)
{
    // Test where property exists
    {
        const json element = R"( { "format": "linear" } )"_json;
        const json& propertyElement = getRequiredProperty(element, "format");
        EXPECT_EQ(propertyElement.get<std::string>(), "linear");
    }

    // Test where property does not exist
    try
    {
        const json element = R"( { "volts": 1.03 } )"_json;
        getRequiredProperty(element, "format");
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: format");
    }
}

TEST(ConfigFileParserTests, ParseAction)
{
    // Test where works: comments property specified
    {
        const json element = R"(
            {
              "comments": [ "Set output voltage." ],
              "pmbus_write_vout_command": {
                "format": "linear"
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: comments property not specified
    {
        const json element = R"(
            {
              "pmbus_write_vout_command": {
                "format": "linear"
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: and action type specified
    // TODO: Not implemented yet

    // Test where works: compare_presence action type specified
    // TODO: Not implemented yet

    // Test where works: compare_vpd action type specified
    // TODO: Not implemented yet

    // Test where works: i2c_compare_bit action type specified
    // TODO: Not implemented yet

    // Test where works: i2c_compare_byte action type specified
    // TODO: Not implemented yet

    // Test where works: i2c_compare_bytes action type specified
    // TODO: Not implemented yet

    // Test where works: i2c_write_bit action type specified
    {
        const json element = R"(
            {
              "i2c_write_bit": {
                "register": "0xA0",
                "position": 3,
                "value": 0
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: i2c_write_byte action type specified
    // TODO: Not implemented yet

    // Test where works: i2c_write_bytes action type specified
    // TODO: Not implemented yet

    // Test where works: if action type specified
    // TODO: Not implemented yet

    // Test where works: not action type specified
    // TODO: Not implemented yet

    // Test where works: or action type specified
    // TODO: Not implemented yet

    // Test where works: pmbus_read_sensor action type specified
    // TODO: Not implemented yet

    // Test where works: pmbus_write_vout_command action type specified
    {
        const json element = R"(
            {
              "pmbus_write_vout_command": {
                "format": "linear"
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: run_rule action type specified
    // TODO: Not implemented yet

    // Test where works: set_device action type specified
    // TODO: Not implemented yet

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseAction(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: No action type specified
    try
    {
        const json element = R"(
            {
              "comments": [ "Set output voltage." ]
            }
        )"_json;
        parseAction(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required action type property missing");
    }

    // Test where fails: Multiple action types specified
    // TODO: Implement after another action type is supported

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "remarks": [ "Set output voltage." ],
              "pmbus_write_vout_command": {
                "format": "linear"
              }
            }
        )"_json;
        parseAction(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseActionArray)
{
    // Test where works
    {
        const json element = R"(
            [
              { "pmbus_write_vout_command": { "volts": 1.01, "format": "linear" } },
              { "pmbus_write_vout_command": { "volts": 1.03, "format": "linear" } }
            ]
        )"_json;
        std::vector<std::unique_ptr<Action>> actions =
            parseActionArray(element);
        EXPECT_EQ(actions.size(), 2);
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
              "foo": "bar"
            }
        )"_json;
        parseActionArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParseBitPosition)
{
    // Test where works: 0
    {
        const json element = R"( 0 )"_json;
        uint8_t value = parseBitPosition(element);
        EXPECT_EQ(value, 0);
    }

    // Test where works: 7
    {
        const json element = R"( 7 )"_json;
        uint8_t value = parseBitPosition(element);
        EXPECT_EQ(value, 7);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.03 )"_json;
        parseBitPosition(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Value < 0
    try
    {
        const json element = R"( -1 )"_json;
        parseBitPosition(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit position");
    }

    // Test where fails: Value > 7
    try
    {
        const json element = R"( 8 )"_json;
        parseBitPosition(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit position");
    }
}

TEST(ConfigFileParserTests, ParseBitValue)
{
    // Test where works: 0
    {
        const json element = R"( 0 )"_json;
        uint8_t value = parseBitValue(element);
        EXPECT_EQ(value, 0);
    }

    // Test where works: 1
    {
        const json element = R"( 1 )"_json;
        uint8_t value = parseBitValue(element);
        EXPECT_EQ(value, 1);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 0.5 )"_json;
        parseBitValue(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Value < 0
    try
    {
        const json element = R"( -1 )"_json;
        parseBitValue(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit value");
    }

    // Test where fails: Value > 1
    try
    {
        const json element = R"( 2 )"_json;
        parseBitValue(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit value");
    }
}

TEST(ConfigFileParserTests, ParseBoolean)
{
    // Test where works: true
    {
        const json element = R"( true )"_json;
        bool value = parseBoolean(element);
        EXPECT_EQ(value, true);
    }

    // Test where works: false
    {
        const json element = R"( false )"_json;
        bool value = parseBoolean(element);
        EXPECT_EQ(value, false);
    }

    // Test where fails: Element is not a boolean
    try
    {
        const json element = R"( 1 )"_json;
        parseBoolean(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }
}

TEST(ConfigFileParserTests, ParseChassisArray)
{
    // TODO: Not implemented yet
}

TEST(ConfigFileParserTests, ParseDouble)
{
    // Test where works: floating point value
    {
        const json element = R"( 1.03 )"_json;
        double value = parseDouble(element);
        EXPECT_EQ(value, 1.03);
    }

    // Test where works: integer value
    {
        const json element = R"( 24 )"_json;
        double value = parseDouble(element);
        EXPECT_EQ(value, 24.0);
    }

    // Test where fails: Element is not a number
    try
    {
        const json element = R"( true )"_json;
        parseDouble(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a number");
    }
}

TEST(ConfigFileParserTests, ParseInt8)
{
    // Test where works: INT8_MIN
    {
        const json element = R"( -128 )"_json;
        int8_t value = parseInt8(element);
        EXPECT_EQ(value, -128);
    }

    // Test where works: INT8_MAX
    {
        const json element = R"( 127 )"_json;
        int8_t value = parseInt8(element);
        EXPECT_EQ(value, 127);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.03 )"_json;
        parseInt8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Value < INT8_MIN
    try
    {
        const json element = R"( -129 )"_json;
        parseInt8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit signed integer");
    }

    // Test where fails: Value > INT8_MAX
    try
    {
        const json element = R"( 128 )"_json;
        parseInt8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit signed integer");
    }
}

TEST(ConfigFileParserTests, ParseI2CWriteBit)
{
    // Test where works
    {
        const json element = R"(
            {
              "register": "0xA0",
              "position": 3,
              "value": 0
            }
        )"_json;
        std::unique_ptr<I2CWriteBitAction> action = parseI2CWriteBit(element);
        EXPECT_EQ(action->getRegister(), 0xA0);
        EXPECT_EQ(action->getPosition(), 3);
        EXPECT_EQ(action->getValue(), 0);
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "register": "0xA0",
              "position": 3,
              "value": 0,
              "foo": 3
            }
        )"_json;
        parseI2CWriteBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseI2CWriteBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: register value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0xAG",
              "position": 3,
              "value": 0
            }
        )"_json;
        parseI2CWriteBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: position value is invalid
    try
    {
        const json element = R"(
                {
                  "register": "0xA0",
                  "position": 8,
                  "value": 0
                }
            )"_json;
        parseI2CWriteBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit position");
    }

    // Test where fails: value value is invalid
    try
    {
        const json element = R"(
                {
                  "register": "0xA0",
                  "position": 3,
                  "value": 2
                }
            )"_json;
        parseI2CWriteBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit value");
    }

    // Test where fails: Required register property not specified
    try
    {
        const json element = R"(
            {
              "position": 3,
              "value": 0
            }
        )"_json;
        parseI2CWriteBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: register");
    }

    // Test where fails: Required position property not specified
    try
    {
        const json element = R"(
            {
              "register": "0xA0",
              "value": 0
            }
        )"_json;
        parseI2CWriteBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: position");
    }

    // Test where fails: Required value property not specified
    try
    {
        const json element = R"(
            {
              "register": "0xA0",
              "position": 3
            }
        )"_json;
        parseI2CWriteBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: value");
    }
}

TEST(ConfigFileParserTests, ParsePMBusWriteVoutCommand)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "format": "linear"
            }
        )"_json;
        std::unique_ptr<PMBusWriteVoutCommandAction> action =
            parsePMBusWriteVoutCommand(element);
        EXPECT_EQ(action->getVolts().has_value(), false);
        EXPECT_EQ(action->getFormat(), pmbus_utils::VoutDataFormat::linear);
        EXPECT_EQ(action->getExponent().has_value(), false);
        EXPECT_EQ(action->isVerified(), false);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "volts": 1.03,
              "format": "linear",
              "exponent": -8,
              "is_verified": true
            }
        )"_json;
        std::unique_ptr<PMBusWriteVoutCommandAction> action =
            parsePMBusWriteVoutCommand(element);
        EXPECT_EQ(action->getVolts().has_value(), true);
        EXPECT_EQ(action->getVolts().value(), 1.03);
        EXPECT_EQ(action->getFormat(), pmbus_utils::VoutDataFormat::linear);
        EXPECT_EQ(action->getExponent().has_value(), true);
        EXPECT_EQ(action->getExponent().value(), -8);
        EXPECT_EQ(action->isVerified(), true);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parsePMBusWriteVoutCommand(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: volts value is invalid
    try
    {
        const json element = R"(
            {
              "volts": "foo",
              "format": "linear"
            }
        )"_json;
        parsePMBusWriteVoutCommand(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a number");
    }

    // Test where fails: Required format property not specified
    try
    {
        const json element = R"(
            {
              "volts": 1.03,
              "is_verified": true
            }
        )"_json;
        parsePMBusWriteVoutCommand(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: format");
    }

    // Test where fails: format value is invalid
    try
    {
        const json element = R"(
            {
              "format": "linear_11"
            }
        )"_json;
        parsePMBusWriteVoutCommand(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid format value: linear_11");
    }

    // Test where fails: exponent value is invalid
    try
    {
        const json element = R"(
            {
              "format": "linear",
              "exponent": 1.3
            }
        )"_json;
        parsePMBusWriteVoutCommand(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: is_verified value is invalid
    try
    {
        const json element = R"(
            {
              "format": "linear",
              "is_verified": "true"
            }
        )"_json;
        parsePMBusWriteVoutCommand(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "format": "linear",
              "foo": "bar"
            }
        )"_json;
        parsePMBusWriteVoutCommand(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseRoot)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "chassis": [
                { "number": 1 }
              ]
            }
        )"_json;
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassis{};
        std::tie(rules, chassis) = parseRoot(element);
        EXPECT_EQ(rules.size(), 0);
        // TODO: Not implemented yet
        // EXPECT_EQ(chassis.size(), 1);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "comments": [ "Config file for a FooBar one-chassis system" ],
              "rules": [
                {
                  "id": "set_voltage_rule",
                  "actions": [
                    { "pmbus_write_vout_command": { "format": "linear" } }
                  ]
                }
              ],
              "chassis": [
                { "number": 1 },
                { "number": 3 }
              ]
            }
        )"_json;
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassis{};
        std::tie(rules, chassis) = parseRoot(element);
        EXPECT_EQ(rules.size(), 1);
        // TODO: Not implemented yet
        // EXPECT_EQ(chassis.size(), 2);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseRoot(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: chassis property not specified
    try
    {
        const json element = R"(
            {
              "rules": [
                {
                  "id": "set_voltage_rule",
                  "actions": [
                    { "pmbus_write_vout_command": { "format": "linear" } }
                  ]
                }
              ]
            }
        )"_json;
        parseRoot(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: chassis");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "remarks": [ "Config file for a FooBar one-chassis system" ],
              "chassis": [
                { "number": 1 }
              ]
            }
        )"_json;
        parseRoot(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseRule)
{
    // Test where works: comments property specified
    {
        const json element = R"(
            {
              "comments": [ "Set voltage rule" ],
              "id": "set_voltage_rule",
              "actions": [
                { "pmbus_write_vout_command": { "volts": 1.01, "format": "linear" } },
                { "pmbus_write_vout_command": { "volts": 1.03, "format": "linear" } }
              ]
            }
        )"_json;
        std::unique_ptr<Rule> rule = parseRule(element);
        EXPECT_EQ(rule->getID(), "set_voltage_rule");
        EXPECT_EQ(rule->getActions().size(), 2);
    }

    // Test where works: comments property not specified
    {
        const json element = R"(
            {
              "id": "set_voltage_rule",
              "actions": [
                { "pmbus_write_vout_command": { "volts": 1.01, "format": "linear" } },
                { "pmbus_write_vout_command": { "volts": 1.03, "format": "linear" } },
                { "pmbus_write_vout_command": { "volts": 1.05, "format": "linear" } }
              ]
            }
        )"_json;
        std::unique_ptr<Rule> rule = parseRule(element);
        EXPECT_EQ(rule->getID(), "set_voltage_rule");
        EXPECT_EQ(rule->getActions().size(), 3);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseRule(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: id property not specified
    try
    {
        const json element = R"(
            {
              "actions": [
                { "pmbus_write_vout_command": { "volts": 1.01, "format": "linear" } }
              ]
            }
        )"_json;
        parseRule(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: id");
    }

    // Test where fails: id property is invalid
    try
    {
        const json element = R"(
            {
              "id": "",
              "actions": [
                { "pmbus_write_vout_command": { "volts": 1.01, "format": "linear" } }
              ]
            }
        )"_json;
        parseRule(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }

    // Test where fails: actions property not specified
    try
    {
        const json element = R"(
            {
              "comments": [ "Set voltage rule" ],
              "id": "set_voltage_rule"
            }
        )"_json;
        parseRule(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: actions");
    }

    // Test where fails: actions property is invalid
    try
    {
        const json element = R"(
            {
              "id": "set_voltage_rule",
              "actions": true
            }
        )"_json;
        parseRule(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "remarks": [ "Set voltage rule" ],
              "id": "set_voltage_rule",
              "actions": [
                { "pmbus_write_vout_command": { "volts": 1.01, "format": "linear" } }
              ]
            }
        )"_json;
        parseRule(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseRuleArray)
{
    // Test where works
    {
        const json element = R"(
            [
              {
                "id": "set_voltage_rule1",
                "actions": [
                  { "pmbus_write_vout_command": { "volts": 1.01, "format": "linear" } }
                ]
              },
              {
                "id": "set_voltage_rule2",
                "actions": [
                  { "pmbus_write_vout_command": { "volts": 1.01, "format": "linear" } },
                  { "pmbus_write_vout_command": { "volts": 1.11, "format": "linear" } }
                ]
              }
            ]
        )"_json;
        std::vector<std::unique_ptr<Rule>> rules = parseRuleArray(element);
        EXPECT_EQ(rules.size(), 2);
        EXPECT_EQ(rules[0]->getID(), "set_voltage_rule1");
        EXPECT_EQ(rules[0]->getActions().size(), 1);
        EXPECT_EQ(rules[1]->getID(), "set_voltage_rule2");
        EXPECT_EQ(rules[1]->getActions().size(), 2);
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"( { "id": "set_voltage_rule" } )"_json;
        parseRuleArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParseString)
{
    // Test where works: Empty string
    {
        const json element = "";
        std::string value = parseString(element, true);
        EXPECT_EQ(value, "");
    }

    // Test where works: Non-empty string
    {
        const json element = "vdd_regulator";
        std::string value = parseString(element, false);
        EXPECT_EQ(value, "vdd_regulator");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        parseString(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Empty string
    try
    {
        const json element = "";
        parseString(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }
}

TEST(ConfigFileParserTests, ParseStringToUint8)
{
    // Test where works: "0xFF"
    {
        const json element = R"( "0xFF" )"_json;
        uint8_t value = parseStringToUint8(element);
        EXPECT_EQ(value, 0xFF);
    }

    // Test where works: "0xff"
    {
        const json element = R"( "0xff" )"_json;
        uint8_t value = parseStringToUint8(element);
        EXPECT_EQ(value, 0xff);
    }

    // Test where works: "0xf"
    {
        const json element = R"( "0xf" )"_json;
        uint8_t value = parseStringToUint8(element);
        EXPECT_EQ(value, 0xf);
    }

    // Test where fails: "0xfff"
    try
    {
        const json element = R"( "0xfff" )"_json;
        parseStringToUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "0xAG"
    try
    {
        const json element = R"( "0xAG" )"_json;
        parseStringToUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "ff"
    try
    {
        const json element = R"( "ff" )"_json;
        parseStringToUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: ""
    try
    {
        const json element = "";
        parseStringToUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }

    // Test where fails: "f"
    try
    {
        const json element = R"( "f" )"_json;
        parseStringToUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "0x"
    try
    {
        const json element = R"( "0x" )"_json;
        parseStringToUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "0Xff"
    try
    {
        const json element = R"( "0XFF" )"_json;
        parseStringToUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }
}

TEST(ConfigFileParserTests, ParseUint8)
{
    // Test where works: 0
    {
        const json element = R"( 0 )"_json;
        uint8_t value = parseUint8(element);
        EXPECT_EQ(value, 0);
    }

    // Test where works: UINT8_MAX
    {
        const json element = R"( 255 )"_json;
        uint8_t value = parseUint8(element);
        EXPECT_EQ(value, 255);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.03 )"_json;
        parseUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Value < 0
    try
    {
        const json element = R"( -1 )"_json;
        parseUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit unsigned integer");
    }

    // Test where fails: Value > UINT8_MAX
    try
    {
        const json element = R"( 256 )"_json;
        parseUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit unsigned integer");
    }
}

TEST(ConfigFileParserTests, VerifyIsArray)
{
    // Test where element is an array
    try
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        verifyIsArray(element);
    }
    catch (const std::exception& e)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where element is not an array
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        verifyIsArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, VerifyIsObject)
{
    // Test where element is an object
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        verifyIsObject(element);
    }
    catch (const std::exception& e)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where element is not an object
    try
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        verifyIsObject(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }
}

TEST(ConfigFileParserTests, VerifyPropertyCount)
{
    // Test where element has expected number of properties
    try
    {
        const json element = R"(
            {
              "comments": [ "Set voltage rule" ],
              "id": "set_voltage_rule"
            }
        )"_json;
        verifyPropertyCount(element, 2);
    }
    catch (const std::exception& e)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where element has unexpected number of properties
    try
    {
        const json element = R"(
            {
              "comments": [ "Set voltage rule" ],
              "id": "set_voltage_rule",
              "foo": 1.3
            }
        )"_json;
        verifyPropertyCount(element, 2);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}
