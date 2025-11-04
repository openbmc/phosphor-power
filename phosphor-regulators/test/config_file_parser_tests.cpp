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
#include "and_action.hpp"
#include "chassis.hpp"
#include "compare_presence_action.hpp"
#include "compare_vpd_action.hpp"
#include "config_file_parser.hpp"
#include "config_file_parser_error.hpp"
#include "configuration.hpp"
#include "device.hpp"
#include "i2c_capture_bytes_action.hpp"
#include "i2c_compare_bit_action.hpp"
#include "i2c_compare_byte_action.hpp"
#include "i2c_compare_bytes_action.hpp"
#include "i2c_interface.hpp"
#include "i2c_write_bit_action.hpp"
#include "i2c_write_byte_action.hpp"
#include "i2c_write_bytes_action.hpp"
#include "log_phase_fault_action.hpp"
#include "not_action.hpp"
#include "or_action.hpp"
#include "phase_fault.hpp"
#include "phase_fault_detection.hpp"
#include "pmbus_read_sensor_action.hpp"
#include "pmbus_utils.hpp"
#include "pmbus_write_vout_command_action.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"
#include "run_rule_action.hpp"
#include "sensor_monitoring.hpp"
#include "sensors.hpp"
#include "set_device_action.hpp"
#include "temporary_file.hpp"

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
using ConfigFileParserError = phosphor::power::util::ConfigFileParserError;
using json = nlohmann::json;
using TemporaryFile = phosphor::power::util::TemporaryFile;

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
                { "number": 1, "inventory_path": "system/chassis1" },
                { "number": 2, "inventory_path": "system/chassis2" },
                { "number": 3, "inventory_path": "system/chassis3" }
              ]
            }
        )"_json;

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassis{};
        std::tie(rules, chassis) = parse(pathName);

        EXPECT_EQ(rules.size(), 2);
        EXPECT_EQ(rules[0]->getID(), "set_voltage_rule1");
        EXPECT_EQ(rules[1]->getID(), "set_voltage_rule2");

        EXPECT_EQ(chassis.size(), 3);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[0]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis1");
        EXPECT_EQ(chassis[1]->getNumber(), 2);
        EXPECT_EQ(chassis[1]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
        EXPECT_EQ(chassis[2]->getNumber(), 3);
        EXPECT_EQ(chassis[2]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis3");
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
              "chassis": [
                { "number": 1, "inventory_path": "system/chassis1" }
              ]
            }
        )"_json;

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
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

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
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

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        parse(pathName);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ConfigFileParserError& e)
    {
        // Expected exception; what() message will vary
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
    {
        const json element = R"(
            {
              "and": [
                { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } },
                { "i2c_compare_byte": { "register": "0xA1", "value": "0x00" } }
              ]
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: compare_presence action type specified
    {
        const json element = R"(
            {
              "compare_presence":
              {
                "fru": "system/chassis/motherboard/cpu3",
                "value": true
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: compare_vpd action type specified
    {
        const json element = R"(
            {
              "compare_vpd":
              {
                "fru": "system/chassis/disk_backplane",
                "keyword": "CCIN",
                "value": "2D35"
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: i2c_capture_bytes action type specified
    {
        const json element = R"(
            {
              "i2c_capture_bytes": {
                "register": "0xA0",
                "count": 2
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: i2c_compare_bit action type specified
    {
        const json element = R"(
            {
              "i2c_compare_bit": {
                "register": "0xA0",
                "position": 3,
                "value": 0
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: i2c_compare_byte action type specified
    {
        const json element = R"(
            {
              "i2c_compare_byte": {
                "register": "0x0A",
                "value": "0xCC"
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: i2c_compare_bytes action type specified
    {
        const json element = R"(
            {
              "i2c_compare_bytes": {
                "register": "0x0A",
                "values": [ "0xCC", "0xFF" ]
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

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
    {
        const json element = R"(
            {
              "i2c_write_byte": {
                "register": "0x0A",
                "value": "0xCC"
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: i2c_write_bytes action type specified
    {
        const json element = R"(
            {
              "i2c_write_bytes": {
                "register": "0x0A",
                "values": [ "0xCC", "0xFF" ]
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: if action type specified
    {
        const json element = R"(
            {
              "if":
              {
                "condition": { "run_rule": "is_downlevel_regulator" },
                "then": [ { "run_rule": "configure_downlevel_regulator" } ],
                "else": [ { "run_rule": "configure_standard_regulator" } ]
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: log_phase_fault action type specified
    {
        const json element = R"(
            {
              "log_phase_fault": {
                "type": "n+1"
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: not action type specified
    {
        const json element = R"(
            {
              "not":
              { "i2c_compare_byte": { "register": "0xA0", "value": "0xFF" } }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: or action type specified
    {
        const json element = R"(
            {
              "or": [
                { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } },
                { "i2c_compare_byte": { "register": "0xA1", "value": "0x00" } }
              ]
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: pmbus_read_sensor action type specified
    {
        const json element = R"(
            {
              "pmbus_read_sensor": {
                "type": "iout",
                "command": "0x8C",
                "format": "linear_11"
              }
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

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
    {
        const json element = R"(
            {
              "run_rule": "set_voltage_rule"
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

    // Test where works: set_device action type specified
    {
        const json element = R"(
            {
              "set_device": "io_expander2"
            }
        )"_json;
        std::unique_ptr<Action> action = parseAction(element);
        EXPECT_NE(action.get(), nullptr);
    }

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
    try
    {
        const json element = R"(
            {
              "pmbus_write_vout_command": { "format": "linear" },
              "run_rule": "set_voltage_rule"
            }
        )"_json;
        parseAction(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

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

TEST(ConfigFileParserTests, ParseAnd)
{
    // Test where works: Element is an array with 2 actions
    {
        const json element = R"(
            [
              { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } },
              { "i2c_compare_byte": { "register": "0xA1", "value": "0x00" } }
            ]
        )"_json;
        std::unique_ptr<AndAction> action = parseAnd(element);
        EXPECT_EQ(action->getActions().size(), 2);
    }

    // Test where fails: Element is an array with 1 action
    try
    {
        const json element = R"(
            [
              { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } }
            ]
        )"_json;
        parseAnd(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Array must contain two or more actions");
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
              "foo": "bar"
            }
        )"_json;
        parseAnd(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParseChassis)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "number": 1,
              "inventory_path": "system/chassis1"
            }
        )"_json;
        std::unique_ptr<Chassis> chassis = parseChassis(element);
        EXPECT_EQ(chassis->getNumber(), 1);
        EXPECT_EQ(chassis->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis1");
        EXPECT_EQ(chassis->getDevices().size(), 0);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "comments": [ "comments property" ],
              "number": 2,
              "inventory_path": "system/chassis2",
              "devices": [
                {
                  "id": "vdd_regulator",
                  "is_regulator": true,
                  "fru": "system/chassis/motherboard/regulator2",
                  "i2c_interface":
                  {
                      "bus": 1,
                      "address": "0x70"
                  }
                }
              ]
            }
        )"_json;
        std::unique_ptr<Chassis> chassis = parseChassis(element);
        EXPECT_EQ(chassis->getNumber(), 2);
        EXPECT_EQ(chassis->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
        EXPECT_EQ(chassis->getDevices().size(), 1);
        EXPECT_EQ(chassis->getDevices()[0]->getID(), "vdd_regulator");
    }

    // Test where fails: number value is invalid
    try
    {
        const json element = R"(
            {
              "number": 0.5,
              "inventory_path": "system/chassis"
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: inventory_path is invalid: Not a string
    try
    {
        const json element = R"(
            {
              "number": 2,
              "inventory_path": true
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: inventory_path is invalid: Empty string
    try
    {
        const json element = R"(
            {
              "number": 2,
              "inventory_path": ""
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "number": 1,
              "inventory_path": "system/chassis",
              "foo": 2
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Required number property not specified
    try
    {
        const json element = R"(
            {
              "inventory_path": "system/chassis"
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: number");
    }

    // Test where fails: Required inventory_path property not specified
    try
    {
        const json element = R"(
            {
              "number": 1
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: inventory_path");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: number value is < 1
    try
    {
        const json element = R"(
            {
              "number": 0,
              "inventory_path": "system/chassis"
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid chassis number: Must be > 0");
    }

    // Test where fails: devices value is invalid
    try
    {
        const json element = R"(
            {
              "number": 1,
              "inventory_path": "system/chassis",
              "devices": 2
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParseChassisArray)
{
    // Test where works
    {
        const json element = R"(
            [
              { "number": 1, "inventory_path": "system/chassis1" },
              { "number": 2, "inventory_path": "system/chassis2" }
            ]
        )"_json;
        std::vector<std::unique_ptr<Chassis>> chassis =
            parseChassisArray(element);
        EXPECT_EQ(chassis.size(), 2);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[0]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis1");
        EXPECT_EQ(chassis[1]->getNumber(), 2);
        EXPECT_EQ(chassis[1]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
              "foo": "bar"
            }
        )"_json;
        parseChassisArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParseComparePresence)
{
    // Test where works
    {
        const json element = R"(
            {
              "fru": "system/chassis/motherboard/cpu3",
              "value": true
            }
        )"_json;
        std::unique_ptr<ComparePresenceAction> action =
            parseComparePresence(element);
        EXPECT_EQ(
            action->getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu3");
        EXPECT_EQ(action->getValue(), true);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseComparePresence(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/motherboard/cpu3",
              "value": true,
              "foo" : true
            }
        )"_json;
        parseComparePresence(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Required fru property not specified
    try
    {
        const json element = R"(
            {
              "value": true
            }
        )"_json;
        parseComparePresence(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: fru");
    }

    // Test where fails: Required value property not specified
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/motherboard/cpu3"
            }
        )"_json;
        parseComparePresence(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: value");
    }

    // Test where fails: fru value is invalid
    try
    {
        const json element = R"(
            {
              "fru": 1,
              "value": true
            }
        )"_json;
        parseComparePresence(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: value value is invalid
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/motherboard/cpu3",
              "value": 1
            }
        )"_json;
        parseComparePresence(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }
}

TEST(ConfigFileParserTests, ParseCompareVPD)
{
    // Test where works: value property: Not empty
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN",
              "value": "2D35"
            }
        )"_json;
        std::unique_ptr<CompareVPDAction> action = parseCompareVPD(element);
        EXPECT_EQ(
            action->getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action->getKeyword(), "CCIN");
        EXPECT_EQ(action->getValue(),
                  (std::vector<uint8_t>{0x32, 0x44, 0x33, 0x35}));
    }

    // Test where works: value property: Empty
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN",
              "value": ""
            }
        )"_json;
        std::unique_ptr<CompareVPDAction> action = parseCompareVPD(element);
        EXPECT_EQ(
            action->getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action->getKeyword(), "CCIN");
        EXPECT_EQ(action->getValue(), (std::vector<uint8_t>{}));
    }

    // Test where works: byte_values property: Not empty
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN",
              "byte_values": ["0x11", "0x22", "0x33"]
            }
        )"_json;
        std::unique_ptr<CompareVPDAction> action = parseCompareVPD(element);
        EXPECT_EQ(
            action->getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action->getKeyword(), "CCIN");
        EXPECT_EQ(action->getValue(), (std::vector<uint8_t>{0x11, 0x22, 0x33}));
    }

    // Test where works: byte_values property: Empty
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN",
              "byte_values": []
            }
        )"_json;
        std::unique_ptr<CompareVPDAction> action = parseCompareVPD(element);
        EXPECT_EQ(
            action->getFRU(),
            "/xyz/openbmc_project/inventory/system/chassis/disk_backplane");
        EXPECT_EQ(action->getKeyword(), "CCIN");
        EXPECT_EQ(action->getValue(), (std::vector<uint8_t>{}));
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN",
              "value": "2D35",
              "foo" : true
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Required fru property not specified
    try
    {
        const json element = R"(
            {
              "keyword": "CCIN",
              "value": "2D35"
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: fru");
    }

    // Test where fails: Required keyword property not specified
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "value": "2D35"
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: keyword");
    }

    // Test where fails: Required value property not specified
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN"
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property: Must contain "
                               "either value or byte_values");
    }

    // Test where fails: both value and byte_values specified
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN",
              "value": "2D35",
              "byte_values": [ "0x01", "0x02" ]
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property: Must contain "
                               "either value or byte_values");
    }

    // Test where fails: fru value is invalid
    try
    {
        const json element = R"(
            {
              "fru": 1,
              "keyword": "CCIN",
              "value": "2D35"
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: keyword value is invalid
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": 1,
              "value": "2D35"
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: value value is invalid
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN",
              "value": 1
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: byte_values is wrong format
    try
    {
        const json element = R"(
            {
              "fru": "system/chassis/disk_backplane",
              "keyword": "CCIN",
              "byte_values": [1, 2, 3]
            }
        )"_json;
        parseCompareVPD(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }
}

TEST(ConfigFileParserTests, ParseConfiguration)
{
    // Test where works: actions required property specified
    {
        const json element = R"(
            {
              "actions": [
                {
                  "pmbus_write_vout_command": {
                    "format": "linear"
                  }
                }
              ]
            }
        )"_json;
        std::unique_ptr<Configuration> configuration =
            parseConfiguration(element);
        EXPECT_EQ(configuration->getActions().size(), 1);
        EXPECT_EQ(configuration->getVolts().has_value(), false);
    }

    // Test where works: volts and actions properties specified
    {
        const json element = R"(
            {
              "comments": [ "comments property" ],
              "volts": 1.03,
              "actions": [
                { "pmbus_write_vout_command": { "format": "linear" } },
                { "run_rule": "set_voltage_rule" }
              ]
            }
        )"_json;
        std::unique_ptr<Configuration> configuration =
            parseConfiguration(element);
        EXPECT_EQ(configuration->getVolts().has_value(), true);
        EXPECT_EQ(configuration->getVolts().value(), 1.03);
        EXPECT_EQ(configuration->getActions().size(), 2);
    }

    // Test where works: volts and rule_id properties specified
    {
        const json element = R"(
            {
              "volts": 1.05,
              "rule_id": "set_voltage_rule"
            }
        )"_json;
        std::unique_ptr<Configuration> configuration =
            parseConfiguration(element);
        EXPECT_EQ(configuration->getVolts().has_value(), true);
        EXPECT_EQ(configuration->getVolts().value(), 1.05);
        EXPECT_EQ(configuration->getActions().size(), 1);
    }

    // Test where fails: volts value is invalid
    try
    {
        const json element = R"(
            {
              "volts": "foo",
              "actions": [
                {
                  "pmbus_write_vout_command": {
                    "format": "linear"
                  }
                }
              ]
            }
        )"_json;
        parseConfiguration(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a double");
    }

    // Test where fails: actions object is invalid
    try
    {
        const json element = R"(
            {
              "volts": 1.03,
              "actions": 1
            }
        )"_json;
        parseConfiguration(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }

    // Test where fails: rule_id value is invalid
    try
    {
        const json element = R"(
            {
              "volts": 1.05,
              "rule_id": 1
            }
        )"_json;
        parseConfiguration(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Required actions or rule_id property not specified
    try
    {
        const json element = R"(
            {
              "volts": 1.03
            }
        )"_json;
        parseConfiguration(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Required actions or rule_id property both specified
    try
    {
        const json element = R"(
            {
              "volts": 1.03,
              "rule_id": "set_voltage_rule",
              "actions": [
                {
                  "pmbus_write_vout_command": {
                    "format": "linear"
                  }
                }
              ]
            }
        )"_json;
        parseConfiguration(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseConfiguration(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "volts": 1.03,
              "rule_id": "set_voltage_rule",
              "foo": 1
            }
        )"_json;
        parseConfiguration(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseDevice)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": true,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface": { "bus": 1, "address": "0x70" }
            }
        )"_json;
        std::unique_ptr<Device> device = parseDevice(element);
        EXPECT_EQ(device->getID(), "vdd_regulator");
        EXPECT_EQ(device->isRegulator(), true);
        EXPECT_EQ(device->getFRU(), "/xyz/openbmc_project/inventory/system/"
                                    "chassis/motherboard/regulator2");
        EXPECT_NE(&(device->getI2CInterface()), nullptr);
        EXPECT_EQ(device->getPresenceDetection(), nullptr);
        EXPECT_EQ(device->getConfiguration(), nullptr);
        EXPECT_EQ(device->getPhaseFaultDetection(), nullptr);
        EXPECT_EQ(device->getRails().size(), 0);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "comments": [ "VDD Regulator" ],
              "id": "vdd_regulator",
              "is_regulator": true,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              },
              "presence_detection":
              {
                  "rule_id": "is_foobar_backplane_installed_rule"
              },
              "configuration":
              {
                  "rule_id": "configure_ir35221_rule"
              },
              "phase_fault_detection":
              {
                  "rule_id": "detect_phase_fault_rule"
              },
              "rails":
              [
                {
                  "id": "vdd"
                }
              ]
            }
        )"_json;
        std::unique_ptr<Device> device = parseDevice(element);
        EXPECT_EQ(device->getID(), "vdd_regulator");
        EXPECT_EQ(device->isRegulator(), true);
        EXPECT_EQ(device->getFRU(), "/xyz/openbmc_project/inventory/system/"
                                    "chassis/motherboard/regulator2");
        EXPECT_NE(&(device->getI2CInterface()), nullptr);
        EXPECT_NE(device->getPresenceDetection(), nullptr);
        EXPECT_NE(device->getConfiguration(), nullptr);
        EXPECT_NE(device->getPhaseFaultDetection(), nullptr);
        EXPECT_EQ(device->getRails().size(), 1);
    }

    // Test where fails: phase_fault_detection property exists and is_regulator
    // is false
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": false,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              },
              "phase_fault_detection":
              {
                  "rule_id": "detect_phase_fault_rule"
              }
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid phase_fault_detection property when "
                               "is_regulator is false");
    }

    // Test where fails: rails property exists and is_regulator is false
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": false,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              },
              "configuration":
              {
                  "rule_id": "configure_ir35221_rule"
              },
              "rails":
              [
                {
                  "id": "vdd"
                }
              ]
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(),
                     "Invalid rails property when is_regulator is false");
    }

    // Test where fails: id value is invalid
    try
    {
        const json element = R"(
            {
              "id": 3,
              "is_regulator": true,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              }
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: is_regulator value is invalid
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": 3,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              }
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }

    // Test where fails: fru value is invalid
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": true,
              "fru": 2,
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              }
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: i2c_interface value is invalid
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": true,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface": 3
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required id property not specified
    try
    {
        const json element = R"(
            {
              "is_regulator": true,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              }
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: id");
    }

    // Test where fails: Required is_regulator property not specified
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              }
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: is_regulator");
    }

    // Test where fails: Required fru property not specified
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": true,
              "i2c_interface":
              {
                  "bus": 1,
                  "address": "0x70"
              }
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: fru");
    }

    // Test where fails: Required i2c_interface property not specified
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": true,
              "fru": "system/chassis/motherboard/regulator2"
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: i2c_interface");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "id": "vdd_regulator",
              "is_regulator": true,
              "fru": "system/chassis/motherboard/regulator2",
              "i2c_interface": { "bus": 1, "address": "0x70" },
              "foo" : true
            }
        )"_json;
        parseDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseDeviceArray)
{
    // Test where works
    {
        const json element = R"(
            [
              {
                "id": "vdd_regulator",
                "is_regulator": true,
                "fru": "system/chassis/motherboard/regulator2",
                "i2c_interface": { "bus": 1, "address": "0x70" }
              },
              {
                "id": "vio_regulator",
                "is_regulator": true,
                "fru": "system/chassis/motherboard/regulator2",
                "i2c_interface": { "bus": 1, "address": "0x71" }
              }
            ]
        )"_json;
        std::vector<std::unique_ptr<Device>> devices =
            parseDeviceArray(element);
        EXPECT_EQ(devices.size(), 2);
        EXPECT_EQ(devices[0]->getID(), "vdd_regulator");
        EXPECT_EQ(devices[1]->getID(), "vio_regulator");
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
              "foo": "bar"
            }
        )"_json;
        parseDeviceArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParseI2CCaptureBytes)
{
    // Test where works
    {
        const json element = R"(
            {
              "register": "0xA0",
              "count": 2
            }
        )"_json;
        std::unique_ptr<I2CCaptureBytesAction> action =
            parseI2CCaptureBytes(element);
        EXPECT_EQ(action->getRegister(), 0xA0);
        EXPECT_EQ(action->getCount(), 2);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseI2CCaptureBytes(element);
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
              "register": "0x0Z",
              "count": 2
            }
        )"_json;
        parseI2CCaptureBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: count value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0xA0",
              "count": 0
            }
        )"_json;
        parseI2CCaptureBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid byte count: Must be > 0");
    }

    // Test where fails: Required register property not specified
    try
    {
        const json element = R"(
            {
              "count": 2
            }
        )"_json;
        parseI2CCaptureBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: register");
    }

    // Test where fails: Required count property not specified
    try
    {
        const json element = R"(
            {
              "register": "0xA0"
            }
        )"_json;
        parseI2CCaptureBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: count");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "register": "0xA0",
              "count": 2,
              "foo": 3
            }
        )"_json;
        parseI2CCaptureBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseI2CCompareBit)
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
        std::unique_ptr<I2CCompareBitAction> action =
            parseI2CCompareBit(element);
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
        parseI2CCompareBit(element);
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
        parseI2CCompareBit(element);
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
        parseI2CCompareBit(element);
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
        parseI2CCompareBit(element);
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
        parseI2CCompareBit(element);
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
        parseI2CCompareBit(element);
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
        parseI2CCompareBit(element);
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
        parseI2CCompareBit(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: value");
    }
}

TEST(ConfigFileParserTests, ParseI2CCompareByte)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCC"
            }
        )"_json;
        std::unique_ptr<I2CCompareByteAction> action =
            parseI2CCompareByte(element);
        EXPECT_EQ(action->getRegister(), 0x0A);
        EXPECT_EQ(action->getValue(), 0xCC);
        EXPECT_EQ(action->getMask(), 0xFF);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCC",
              "mask": "0xF7"
            }
        )"_json;
        std::unique_ptr<I2CCompareByteAction> action =
            parseI2CCompareByte(element);
        EXPECT_EQ(action->getRegister(), 0x0A);
        EXPECT_EQ(action->getValue(), 0xCC);
        EXPECT_EQ(action->getMask(), 0xF7);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseI2CCompareByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCC",
              "mask": "0xF7",
              "foo": 1
            }
        )"_json;
        parseI2CCompareByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: register value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0Z",
              "value": "0xCC",
              "mask": "0xF7"
            }
        )"_json;
        parseI2CCompareByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: value value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCCC",
              "mask": "0xF7"
            }
        )"_json;
        parseI2CCompareByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: mask value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCC",
              "mask": "F7"
            }
        )"_json;
        parseI2CCompareByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: Required register property not specified
    try
    {
        const json element = R"(
            {
              "value": "0xCC",
              "mask": "0xF7"
            }
        )"_json;
        parseI2CCompareByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: register");
    }

    // Test where fails: Required value property not specified
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "mask": "0xF7"
            }
        )"_json;
        parseI2CCompareByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: value");
    }
}

TEST(ConfigFileParserTests, ParseI2CCompareBytes)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ]
            }
        )"_json;
        std::unique_ptr<I2CCompareBytesAction> action =
            parseI2CCompareBytes(element);
        EXPECT_EQ(action->getRegister(), 0x0A);
        EXPECT_EQ(action->getValues().size(), 2);
        EXPECT_EQ(action->getValues()[0], 0xCC);
        EXPECT_EQ(action->getValues()[1], 0xFF);
        EXPECT_EQ(action->getMasks().size(), 2);
        EXPECT_EQ(action->getMasks()[0], 0xFF);
        EXPECT_EQ(action->getMasks()[1], 0xFF);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "0x7F", "0x77" ]
            }
        )"_json;
        std::unique_ptr<I2CCompareBytesAction> action =
            parseI2CCompareBytes(element);
        EXPECT_EQ(action->getRegister(), 0x0A);
        EXPECT_EQ(action->getValues().size(), 2);
        EXPECT_EQ(action->getValues()[0], 0xCC);
        EXPECT_EQ(action->getValues()[1], 0xFF);
        EXPECT_EQ(action->getMasks().size(), 2);
        EXPECT_EQ(action->getMasks()[0], 0x7F);
        EXPECT_EQ(action->getMasks()[1], 0x77);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseI2CCompareBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "0x7F", "0x7F" ],
              "foo": 1
            }
        )"_json;
        parseI2CCompareBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: register value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0Z",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "0x7F", "0x7F" ]
            }
        )"_json;
        parseI2CCompareBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: values value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCCC", "0xFF" ],
              "masks":  [ "0x7F", "0x7F" ]
            }
        )"_json;
        parseI2CCompareBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: masks value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "F", "0x7F" ]
            }
        )"_json;
        parseI2CCompareBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: number of elements in masks is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "0x7F" ]
            }
        )"_json;
        parseI2CCompareBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid number of elements in masks");
    }

    // Test where fails: Required register property not specified
    try
    {
        const json element = R"(
            {
              "values": [ "0xCC", "0xFF" ]
            }
        )"_json;
        parseI2CCompareBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: register");
    }

    // Test where fails: Required values property not specified
    try
    {
        const json element = R"(
            {
              "register": "0x0A"
            }
        )"_json;
        parseI2CCompareBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: values");
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

TEST(ConfigFileParserTests, ParseI2CWriteByte)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCC"
            }
        )"_json;
        std::unique_ptr<I2CWriteByteAction> action = parseI2CWriteByte(element);
        EXPECT_EQ(action->getRegister(), 0x0A);
        EXPECT_EQ(action->getValue(), 0xCC);
        EXPECT_EQ(action->getMask(), 0xFF);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCC",
              "mask": "0xF7"
            }
        )"_json;
        std::unique_ptr<I2CWriteByteAction> action = parseI2CWriteByte(element);
        EXPECT_EQ(action->getRegister(), 0x0A);
        EXPECT_EQ(action->getValue(), 0xCC);
        EXPECT_EQ(action->getMask(), 0xF7);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseI2CWriteByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCC",
              "mask": "0xF7",
              "foo": 1
            }
        )"_json;
        parseI2CWriteByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: register value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0Z",
              "value": "0xCC",
              "mask": "0xF7"
            }
        )"_json;
        parseI2CWriteByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: value value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCCC",
              "mask": "0xF7"
            }
        )"_json;
        parseI2CWriteByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: mask value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "value": "0xCC",
              "mask": "F7"
            }
        )"_json;
        parseI2CWriteByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: Required register property not specified
    try
    {
        const json element = R"(
            {
              "value": "0xCC",
              "mask": "0xF7"
            }
        )"_json;
        parseI2CWriteByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: register");
    }

    // Test where fails: Required value property not specified
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "mask": "0xF7"
            }
        )"_json;
        parseI2CWriteByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: value");
    }
}

TEST(ConfigFileParserTests, ParseI2CWriteBytes)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ]
            }
        )"_json;
        std::unique_ptr<I2CWriteBytesAction> action =
            parseI2CWriteBytes(element);
        EXPECT_EQ(action->getRegister(), 0x0A);
        EXPECT_EQ(action->getValues().size(), 2);
        EXPECT_EQ(action->getValues()[0], 0xCC);
        EXPECT_EQ(action->getValues()[1], 0xFF);
        EXPECT_EQ(action->getMasks().size(), 0);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "0x7F", "0x77" ]
            }
        )"_json;
        std::unique_ptr<I2CWriteBytesAction> action =
            parseI2CWriteBytes(element);
        EXPECT_EQ(action->getRegister(), 0x0A);
        EXPECT_EQ(action->getValues().size(), 2);
        EXPECT_EQ(action->getValues()[0], 0xCC);
        EXPECT_EQ(action->getValues()[1], 0xFF);
        EXPECT_EQ(action->getMasks().size(), 2);
        EXPECT_EQ(action->getMasks()[0], 0x7F);
        EXPECT_EQ(action->getMasks()[1], 0x77);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseI2CWriteBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "0x7F", "0x7F" ],
              "foo": 1
            }
        )"_json;
        parseI2CWriteBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: register value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0Z",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "0x7F", "0x7F" ]
            }
        )"_json;
        parseI2CWriteBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: values value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCCC", "0xFF" ],
              "masks":  [ "0x7F", "0x7F" ]
            }
        )"_json;
        parseI2CWriteBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: masks value is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "F", "0x7F" ]
            }
        )"_json;
        parseI2CWriteBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: number of elements in masks is invalid
    try
    {
        const json element = R"(
            {
              "register": "0x0A",
              "values": [ "0xCC", "0xFF" ],
              "masks":  [ "0x7F" ]
            }
        )"_json;
        parseI2CWriteBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid number of elements in masks");
    }

    // Test where fails: Required register property not specified
    try
    {
        const json element = R"(
            {
              "values": [ "0xCC", "0xFF" ]
            }
        )"_json;
        parseI2CWriteBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: register");
    }

    // Test where fails: Required values property not specified
    try
    {
        const json element = R"(
            {
              "register": "0x0A"
            }
        )"_json;
        parseI2CWriteBytes(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: values");
    }
}

TEST(ConfigFileParserTests, ParseIf)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "condition": { "run_rule": "is_downlevel_regulator" },
              "then": [ { "run_rule": "configure_downlevel_regulator" },
                        { "run_rule": "configure_standard_regulator" } ]
            }
        )"_json;
        std::unique_ptr<IfAction> action = parseIf(element);
        EXPECT_NE(action->getConditionAction().get(), nullptr);
        EXPECT_EQ(action->getThenActions().size(), 2);
        EXPECT_EQ(action->getElseActions().size(), 0);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "condition": { "run_rule": "is_downlevel_regulator" },
              "then": [ { "run_rule": "configure_downlevel_regulator" } ],
              "else": [ { "run_rule": "configure_standard_regulator" } ]
            }
        )"_json;
        std::unique_ptr<IfAction> action = parseIf(element);
        EXPECT_NE(action->getConditionAction().get(), nullptr);
        EXPECT_EQ(action->getThenActions().size(), 1);
        EXPECT_EQ(action->getElseActions().size(), 1);
    }

    // Test where fails: Required condition property not specified
    try
    {
        const json element = R"(
            {
              "then": [ { "run_rule": "configure_downlevel_regulator" } ],
              "else": [ { "run_rule": "configure_standard_regulator" } ]
            }
        )"_json;
        parseIf(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: condition");
    }

    // Test where fails: Required then property not specified
    try
    {
        const json element = R"(
            {
              "condition": { "run_rule": "is_downlevel_regulator" },
              "else": [ { "run_rule": "configure_standard_regulator" } ]
            }
        )"_json;
        parseIf(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: then");
    }

    // Test where fails: condition value is invalid
    try
    {
        const json element = R"(
            {
              "condition": 1,
              "then": [ { "run_rule": "configure_downlevel_regulator" } ],
              "else": [ { "run_rule": "configure_standard_regulator" } ]
            }
        )"_json;
        parseIf(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: then value is invalid
    try
    {
        const json element = R"(
            {
              "condition": { "run_rule": "is_downlevel_regulator" },
              "then": "foo",
              "else": [ { "run_rule": "configure_standard_regulator" } ]
            }
        )"_json;
        parseIf(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }

    // Test where fails: else value is invalid
    try
    {
        const json element = R"(
            {
              "condition": { "run_rule": "is_downlevel_regulator" },
              "then": [ { "run_rule": "configure_downlevel_regulator" } ],
              "else": 1
            }
        )"_json;
        parseIf(element);
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
              "condition": { "run_rule": "is_downlevel_regulator" },
              "then": [ { "run_rule": "configure_downlevel_regulator" } ],
              "foo": "bar"
            }
        )"_json;
        parseIf(element);
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
        parseIf(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }
}

TEST(ConfigFileParserTests, ParseInventoryPath)
{
    // Test where works: Inventory path has a leading '/'
    {
        const json element = "/system/chassis/motherboard/cpu3";
        std::string value = parseInventoryPath(element);
        EXPECT_EQ(
            value,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu3");
    }

    // Test where works: Inventory path does not have a leading '/'
    {
        const json element = "system/chassis/motherboard/cpu1";
        std::string value = parseInventoryPath(element);
        EXPECT_EQ(
            value,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1");
    }

    // Test where fails: JSON element is not a string
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        parseInventoryPath(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: JSON element contains an empty string
    try
    {
        const json element = "";
        parseInventoryPath(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }
}

TEST(ConfigFileParserTests, ParseLogPhaseFault)
{
    // Test where works
    {
        const json element = R"(
            {
              "type": "n+1"
            }
        )"_json;
        std::unique_ptr<LogPhaseFaultAction> action =
            parseLogPhaseFault(element);
        EXPECT_EQ(action->getType(), PhaseFaultType::n_plus_1);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseLogPhaseFault(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required type property not specified
    try
    {
        const json element = R"(
            {
            }
        )"_json;
        parseLogPhaseFault(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: type");
    }

    // Test where fails: type value is invalid
    try
    {
        const json element = R"(
            {
              "type": "n+2"
            }
        )"_json;
        parseLogPhaseFault(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a phase fault type");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "type": "n+1",
              "foo": 1
            }
        )"_json;
        parseLogPhaseFault(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseNot)
{
    // Test where works
    {
        const json element = R"(
            { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } }
        )"_json;
        std::unique_ptr<NotAction> action = parseNot(element);
        EXPECT_NE(action->getAction().get(), nullptr);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseNot(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }
}

TEST(ConfigFileParserTests, ParseOr)
{
    // Test where works: Element is an array with 2 actions
    {
        const json element = R"(
            [
              { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } },
              { "i2c_compare_byte": { "register": "0xA1", "value": "0x00" } }
            ]
        )"_json;
        std::unique_ptr<OrAction> action = parseOr(element);
        EXPECT_EQ(action->getActions().size(), 2);
    }

    // Test where fails: Element is an array with 1 action
    try
    {
        const json element = R"(
            [
              { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } }
            ]
        )"_json;
        parseOr(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Array must contain two or more actions");
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
              "foo": "bar"
            }
        )"_json;
        parseOr(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParsePhaseFaultDetection)
{
    // Test where works: actions specified: optional properties not specified
    {
        const json element = R"(
            {
              "actions": [
                { "run_rule": "detect_phase_fault_rule" }
              ]
            }
        )"_json;
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection =
            parsePhaseFaultDetection(element);
        EXPECT_EQ(phaseFaultDetection->getActions().size(), 1);
        EXPECT_EQ(phaseFaultDetection->getDeviceID(), "");
    }

    // Test where works: rule_id specified: optional properties specified
    {
        const json element = R"(
            {
              "comments": [ "Detect phase fault using I/O expander" ],
              "device_id": "io_expander",
              "rule_id": "detect_phase_fault_rule"
            }
        )"_json;
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection =
            parsePhaseFaultDetection(element);
        EXPECT_EQ(phaseFaultDetection->getActions().size(), 1);
        EXPECT_EQ(phaseFaultDetection->getDeviceID(), "io_expander");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        parsePhaseFaultDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: device_id value is invalid
    try
    {
        const json element = R"(
            {
              "device_id": 1,
              "rule_id": "detect_phase_fault_rule"
            }
        )"_json;
        parsePhaseFaultDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: rule_id value is invalid
    try
    {
        const json element = R"(
            {
              "rule_id": 1
            }
        )"_json;
        parsePhaseFaultDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: actions object is invalid
    try
    {
        const json element = R"(
            {
              "actions": 1
            }
        )"_json;
        parsePhaseFaultDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }

    // Test where fails: Required actions or rule_id property not specified
    try
    {
        const json element = R"(
            {
              "device_id": "io_expander"
            }
        )"_json;
        parsePhaseFaultDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Required actions or rule_id property both specified
    try
    {
        const json element = R"(
            {
              "rule_id": "detect_phase_fault_rule",
              "actions": [
                { "run_rule": "detect_phase_fault_rule" }
              ]
            }
        )"_json;
        parsePhaseFaultDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "foo": "bar",
              "actions": [
                { "run_rule": "detect_phase_fault_rule" }
              ]
            }
        )"_json;
        parsePhaseFaultDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParsePhaseFaultType)
{
    // Test where works: n
    {
        const json element = "n";
        PhaseFaultType type = parsePhaseFaultType(element);
        EXPECT_EQ(type, PhaseFaultType::n);
    }

    // Test where works: n+1
    {
        const json element = "n+1";
        PhaseFaultType type = parsePhaseFaultType(element);
        EXPECT_EQ(type, PhaseFaultType::n_plus_1);
    }

    // Test where fails: Element is not a phase fault type
    try
    {
        const json element = "n+2";
        parsePhaseFaultType(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a phase fault type");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        parsePhaseFaultType(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }
}

TEST(ConfigFileParserTests, ParsePMBusReadSensor)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "type": "iout",
              "command": "0x8C",
              "format": "linear_11"
            }
        )"_json;
        std::unique_ptr<PMBusReadSensorAction> action =
            parsePMBusReadSensor(element);
        EXPECT_EQ(action->getType(), SensorType::iout);
        EXPECT_EQ(action->getCommand(), 0x8C);
        EXPECT_EQ(action->getFormat(),
                  pmbus_utils::SensorDataFormat::linear_11);
        EXPECT_EQ(action->getExponent().has_value(), false);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "type": "temperature",
              "command": "0x7A",
              "format": "linear_16",
              "exponent": -8
            }
        )"_json;
        std::unique_ptr<PMBusReadSensorAction> action =
            parsePMBusReadSensor(element);
        EXPECT_EQ(action->getType(), SensorType::temperature);
        EXPECT_EQ(action->getCommand(), 0x7A);
        EXPECT_EQ(action->getFormat(),
                  pmbus_utils::SensorDataFormat::linear_16);
        EXPECT_EQ(action->getExponent().has_value(), true);
        EXPECT_EQ(action->getExponent().value(), -8);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "type": "iout",
              "command": "0x8C",
              "format": "linear_11",
              "foo": 1
            }
        )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Required type property not specified
    try
    {
        const json element = R"(
            {
              "command": "0x8C",
              "format": "linear_11"
            }
        )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: type");
    }

    // Test where fails: Required command property not specified
    try
    {
        const json element = R"(
            {
              "type": "iout",
              "format": "linear_11"
            }
        )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: command");
    }

    // Test where fails: Required format property not specified
    try
    {
        const json element = R"(
            {
              "type": "iout",
              "command": "0x8C"
            }
        )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: format");
    }

    // Test where fails: type value is invalid
    try
    {
        const json element = R"(
            {
              "type": 1,
              "command": "0x7A",
              "format": "linear_16"
            }
        )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: command value is invalid
    try
    {
        const json element = R"(
            {
              "type": "temperature",
              "command": 0,
              "format": "linear_16"
            }
        )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: format value is invalid
    try
    {
        const json element = R"(
            {
              "type": "temperature",
              "command": "0x7A",
              "format": 1
            }
        )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: exponent value is invalid
    try
    {
        const json element = R"(
            {
              "type": "temperature",
              "command": "0x7A",
              "format": "linear_16",
              "exponent": 1.3
            }
        )"_json;
        parsePMBusReadSensor(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
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
        EXPECT_STREQ(e.what(), "Element is not a double");
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

TEST(ConfigFileParserTests, ParsePresenceDetection)
{
    // Test where works: actions property specified
    {
        const json element = R"(
            {
              "actions": [
                { "run_rule": "read_sensors_rule" }
              ]
            }
        )"_json;
        std::unique_ptr<PresenceDetection> presenceDetection =
            parsePresenceDetection(element);
        EXPECT_EQ(presenceDetection->getActions().size(), 1);
    }

    // Test where works: rule_id property specified
    {
        const json element = R"(
            {
              "comments": [ "comments property" ],
              "rule_id": "set_voltage_rule"
            }
        )"_json;
        std::unique_ptr<PresenceDetection> presenceDetection =
            parsePresenceDetection(element);
        EXPECT_EQ(presenceDetection->getActions().size(), 1);
    }

    // Test where fails: actions object is invalid
    try
    {
        const json element = R"(
            {
              "actions": 1
            }
        )"_json;
        parsePresenceDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }

    // Test where fails: rule_id value is invalid
    try
    {
        const json element = R"(
            {
              "rule_id": 1
            }
        )"_json;
        parsePresenceDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Required actions or rule_id property not specified
    try
    {
        const json element = R"(
            {
              "comments": [ "comments property" ]
            }
        )"_json;
        parsePresenceDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Required actions or rule_id property both specified
    try
    {
        const json element = R"(
            {
              "rule_id": "set_voltage_rule",
              "actions": [
                { "run_rule": "read_sensors_rule" }
              ]
            }
        )"_json;
        parsePresenceDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        parsePresenceDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "foo": "bar",
              "actions": [
                { "run_rule": "read_sensors_rule" }
              ]
            }
        )"_json;
        parsePresenceDetection(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseRail)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "id": "vdd"
            }
        )"_json;
        std::unique_ptr<Rail> rail = parseRail(element);
        EXPECT_EQ(rail->getID(), "vdd");
        EXPECT_EQ(rail->getConfiguration(), nullptr);
        EXPECT_EQ(rail->getSensorMonitoring(), nullptr);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "comments": [ "comments property" ],
              "id": "vdd",
              "configuration": {
                "volts": 1.1,
                "actions": [
                  {
                    "pmbus_write_vout_command": {
                      "format": "linear"
                    }
                  }
                ]
              },
              "sensor_monitoring": {
                "actions": [
                  { "run_rule": "read_sensors_rule" }
                ]
              }
            }
        )"_json;
        std::unique_ptr<Rail> rail = parseRail(element);
        EXPECT_EQ(rail->getID(), "vdd");
        EXPECT_NE(rail->getConfiguration(), nullptr);
        EXPECT_NE(rail->getSensorMonitoring(), nullptr);
    }

    // Test where fails: id property not specified
    try
    {
        const json element = R"(
            {
              "configuration": {
                "volts": 1.1,
                "actions": [
                  {
                    "pmbus_write_vout_command": {
                      "format": "linear"
                    }
                  }
                ]
              }
            }
        )"_json;
        parseRail(element);
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
              "configuration": {
                "volts": 1.1,
                "actions": [
                  {
                    "pmbus_write_vout_command": {
                      "format": "linear"
                    }
                  }
                ]
              }
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: configuration value is invalid
    try
    {
        const json element = R"(
            {
              "id": "vdd",
              "configuration": "config"
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: sensor_monitoring value is invalid
    try
    {
        const json element = R"(
            {
              "comments": [ "comments property" ],
              "id": "vdd",
              "configuration": {
                "volts": 1.1,
                "actions": [
                  {
                    "pmbus_write_vout_command": {
                      "format": "linear"
                    }
                  }
                ]
              },
              "sensor_monitoring": 1
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "id": "vdd",
              "foo" : true
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseRailArray)
{
    // Test where works
    {
        const json element = R"(
            [
              { "id": "vdd" },
              { "id": "vio" }
            ]
        )"_json;
        std::vector<std::unique_ptr<Rail>> rails = parseRailArray(element);
        EXPECT_EQ(rails.size(), 2);
        EXPECT_EQ(rails[0]->getID(), "vdd");
        EXPECT_EQ(rails[1]->getID(), "vio");
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
              "foo": "bar"
            }
        )"_json;
        parseRailArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParseRoot)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "chassis": [
                { "number": 1, "inventory_path": "system/chassis" }
              ]
            }
        )"_json;
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassis{};
        std::tie(rules, chassis) = parseRoot(element);
        EXPECT_EQ(rules.size(), 0);
        EXPECT_EQ(chassis.size(), 1);
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
                { "number": 1, "inventory_path": "system/chassis1" },
                { "number": 3, "inventory_path": "system/chassis3" }
              ]
            }
        )"_json;
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassis{};
        std::tie(rules, chassis) = parseRoot(element);
        EXPECT_EQ(rules.size(), 1);
        EXPECT_EQ(chassis.size(), 2);
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
                { "number": 1, "inventory_path": "system/chassis" }
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

TEST(ConfigFileParserTests, ParseRuleIDOrActionsProperty)
{
    // Test where works: actions specified
    {
        const json element = R"(
            {
              "actions": [
                { "pmbus_write_vout_command": { "format": "linear" } },
                { "run_rule": "set_voltage_rule" }
              ]
            }
        )"_json;
        std::vector<std::unique_ptr<Action>> actions =
            parseRuleIDOrActionsProperty(element);
        EXPECT_EQ(actions.size(), 2);
    }

    // Test where works: rule_id specified
    {
        const json element = R"(
            {
              "rule_id": "set_voltage_rule"
            }
        )"_json;
        std::vector<std::unique_ptr<Action>> actions =
            parseRuleIDOrActionsProperty(element);
        EXPECT_EQ(actions.size(), 1);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        parseRuleIDOrActionsProperty(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: rule_id is invalid
    try
    {
        const json element = R"(
            { "rule_id": 1 }
        )"_json;
        parseRuleIDOrActionsProperty(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: actions is invalid
    try
    {
        const json element = R"(
            { "actions": 1 }
        )"_json;
        parseRuleIDOrActionsProperty(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }

    // Test where fails: Neither rule_id nor actions specified
    try
    {
        const json element = R"(
            {
              "volts": 1.03
            }
        )"_json;
        parseRuleIDOrActionsProperty(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Both rule_id and actions specified
    try
    {
        const json element = R"(
            {
              "volts": 1.03,
              "rule_id": "set_voltage_rule",
              "actions": [
                {
                  "pmbus_write_vout_command": {
                    "format": "linear"
                  }
                }
              ]
            }
        )"_json;
        parseRuleIDOrActionsProperty(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }
}

TEST(ConfigFileParserTests, ParseRunRule)
{
    // Test where works
    {
        const json element = "vdd_regulator";
        std::unique_ptr<RunRuleAction> action = parseRunRule(element);
        EXPECT_EQ(action->getRuleID(), "vdd_regulator");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = 1;
        parseRunRule(element);
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
        parseRunRule(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }
}

TEST(ConfigFileParserTests, ParseSensorDataFormat)
{
    // Test where works: linear_11
    {
        const json element = "linear_11";
        pmbus_utils::SensorDataFormat value = parseSensorDataFormat(element);
        pmbus_utils::SensorDataFormat format =
            pmbus_utils::SensorDataFormat::linear_11;
        EXPECT_EQ(value, format);
    }

    // Test where works: linear_16
    {
        const json element = "linear_16";
        pmbus_utils::SensorDataFormat value = parseSensorDataFormat(element);
        pmbus_utils::SensorDataFormat format =
            pmbus_utils::SensorDataFormat::linear_16;
        EXPECT_EQ(value, format);
    }

    // Test where fails: Element is not a sensor data format
    try
    {
        const json element = "foo";
        parseSensorDataFormat(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a sensor data format");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        parseSensorDataFormat(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }
}

TEST(ConfigFileParserTests, ParseSensorMonitoring)
{
    // Test where works: actions property specified
    {
        const json element = R"(
            {
              "actions": [
                { "run_rule": "read_sensors_rule" }
              ]
            }
        )"_json;
        std::unique_ptr<SensorMonitoring> sensorMonitoring =
            parseSensorMonitoring(element);
        EXPECT_EQ(sensorMonitoring->getActions().size(), 1);
    }

    // Test where works: rule_id property specified
    {
        const json element = R"(
            {
              "comments": [ "comments property" ],
              "rule_id": "set_voltage_rule"
            }
        )"_json;
        std::unique_ptr<SensorMonitoring> sensorMonitoring =
            parseSensorMonitoring(element);
        EXPECT_EQ(sensorMonitoring->getActions().size(), 1);
    }

    // Test where fails: actions object is invalid
    try
    {
        const json element = R"(
            {
              "actions": 1
            }
        )"_json;
        parseSensorMonitoring(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }

    // Test where fails: rule_id value is invalid
    try
    {
        const json element = R"(
            {
              "rule_id": 1
            }
        )"_json;
        parseSensorMonitoring(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Required actions or rule_id property not specified
    try
    {
        const json element = R"(
            {
              "comments": [ "comments property" ]
            }
        )"_json;
        parseSensorMonitoring(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Required actions or rule_id property both specified
    try
    {
        const json element = R"(
            {
              "rule_id": "set_voltage_rule",
              "actions": [
                { "run_rule": "read_sensors_rule" }
              ]
            }
        )"_json;
        parseSensorMonitoring(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid property combination: Must contain "
                               "either rule_id or actions");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        parseSensorMonitoring(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "foo": "bar",
              "actions": [
                { "run_rule": "read_sensors_rule" }
              ]
            }
        )"_json;
        parseSensorMonitoring(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseSensorType)
{
    // Test where works: iout
    {
        const json element = "iout";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::iout);
    }

    // Test where works: iout_peak
    {
        const json element = "iout_peak";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::iout_peak);
    }

    // Test where works: iout_valley
    {
        const json element = "iout_valley";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::iout_valley);
    }

    // Test where works: pout
    {
        const json element = "pout";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::pout);
    }

    // Test where works: temperature
    {
        const json element = "temperature";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::temperature);
    }

    // Test where works: temperature_peak
    {
        const json element = "temperature_peak";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::temperature_peak);
    }

    // Test where works: vout
    {
        const json element = "vout";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::vout);
    }

    // Test where works: vout_peak
    {
        const json element = "vout_peak";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::vout_peak);
    }

    // Test where works: vout_valley
    {
        const json element = "vout_valley";
        SensorType type = parseSensorType(element);
        EXPECT_EQ(type, SensorType::vout_valley);
    }

    // Test where fails: Element is not a sensor type
    try
    {
        const json element = "foo";
        parseSensorType(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a sensor type");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        parseSensorType(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }
}

TEST(ConfigFileParserTests, ParseSetDevice)
{
    // Test where works
    {
        const json element = "regulator1";
        std::unique_ptr<SetDeviceAction> action = parseSetDevice(element);
        EXPECT_EQ(action->getDeviceID(), "regulator1");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = 1;
        parseSetDevice(element);
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
        parseSetDevice(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }
}

TEST(ConfigFileParserTests, ParseVoutDataFormat)
{
    // Test where works: linear
    {
        const json element = "linear";
        pmbus_utils::VoutDataFormat value = parseVoutDataFormat(element);
        pmbus_utils::VoutDataFormat format =
            pmbus_utils::VoutDataFormat::linear;
        EXPECT_EQ(value, format);
    }

    // Test where works: vid
    {
        const json element = "vid";
        pmbus_utils::VoutDataFormat value = parseVoutDataFormat(element);
        pmbus_utils::VoutDataFormat format = pmbus_utils::VoutDataFormat::vid;
        EXPECT_EQ(value, format);
    }

    // Test where works: direct
    {
        const json element = "direct";
        pmbus_utils::VoutDataFormat value = parseVoutDataFormat(element);
        pmbus_utils::VoutDataFormat format =
            pmbus_utils::VoutDataFormat::direct;
        EXPECT_EQ(value, format);
    }

    // Test where works: ieee
    {
        const json element = "ieee";
        pmbus_utils::VoutDataFormat value = parseVoutDataFormat(element);
        pmbus_utils::VoutDataFormat format = pmbus_utils::VoutDataFormat::ieee;
        EXPECT_EQ(value, format);
    }

    // Test where fails: Element is not a vout data format
    try
    {
        const json element = "foo";
        parseVoutDataFormat(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a vout data format");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        parseVoutDataFormat(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }
}
