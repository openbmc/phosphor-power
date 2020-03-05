/**
 * Copyright c 2020 IBM Corporation
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <nlohmann/json.hpp>

#include <fstream>

#include <gtest/gtest.h>

#define EXPECT_FILE_VALID(configFile) expectFileValid(configFile)
#define EXPECT_FILE_INVALID(configFile, expectedErrorMessage,                  \
                            expectedOutputMessage)                             \
    expectFileInvalid(configFile, expectedErrorMessage, expectedOutputMessage)
#define EXPECT_JSON_VALID(configFileJson) expectJsonValid(configFileJson)
#define EXPECT_JSON_INVALID(configFileJson, expectedErrorMessage,              \
                            expectedOutputMessage)                             \
    expectJsonInvalid(configFileJson, expectedErrorMessage,                    \
                      expectedOutputMessage)

using json = nlohmann::json;

const json validConfigFile = R"(
    {
      "comments": [ "Config file for a FooBar one-chassis system" ],

      "rules": [
        {
          "comments": [ "Sets output voltage for a PMBus regulator rail" ],
          "id": "set_voltage_rule",
          "actions": [
            {
              "pmbus_write_vout_command": {
                "format": "linear"
              }
            }
          ]
        },
        {
          "comments": [ "Reads sensors from a PMBus regulator rail" ],
          "id": "read_sensors_rule",
          "actions": [
            {
              "comments": [ "Read output voltage from READ_VOUT." ],
              "pmbus_read_sensor": {
                "type": "vout",
                "command": "0x8B",
                "format": "linear_16"
              }
            }
          ]
        }
      ],

      "chassis": [
        {
          "comments": [ "Chassis number 1 containing CPUs and memory" ],
          "number": 1,
          "devices": [
            {
              "comments": [ "IR35221 regulator producing the Vdd rail" ],
              "id": "vdd_regulator",
              "is_regulator": true,
              "fru": "/system/chassis/motherboard/regulator1",
              "i2c_interface": {
                "bus": 1,
                "address": "0x70"
              },
              "rails": [
                {
                  "comments": [ "Vdd rail" ],
                  "id": "vdd",
                  "configuration": {
                    "volts": 1.03,
                    "rule_id": "set_voltage_rule"
                  },
                  "sensor_monitoring": {
                    "rule_id": "read_sensors_rule"
                  }
                }
              ]
            }
          ]
        }
      ]
    }
)"_json;

std::string createTmpFile()
{
    // create temporary file using mkstemp under /tmp/. random name for XXXXXX
    char fileName[] = "/tmp/temp-XXXXXX";
    int fd = mkstemp(fileName);
    if (fd == -1)
    {
        perror("Can't create temporary file");
    }
    close(fd);
    return fileName;
}

std::string getValidationToolCommand(const std::string& configFileName)
{
    std::string command = "../tools/validate-regulators-config.py -s \
                           ../schema/config_schema.json -c ";
    command += configFileName;
    return command;
}

int runToolForOutput(const std::string& configFileName, std::string& output,
                     std::string command, bool isReadingStderr = false)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    // reading the stderr while isReadingStderr is true.
    if (isReadingStderr == true)
    {
        command += " 2>&1 >/dev/null";
    }
    // get the jsonschema print from validation tool.
    char buffer[256];
    std::string result = "";
    // to get the stdout from the validation tool.
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (!std::feof(pipe))
    {
        if (fgets(buffer, sizeof buffer, pipe) != NULL)
        {
            result += buffer;
        }
    }
    int returnValue = pclose(pipe);
    // Check if pclose() failed
    if (returnValue == -1)
    {
        // unable to close pipe.  Print error and exit function.
        throw std::runtime_error("pclose() failed!");
    }
    std::string firstLine = result.substr(0, result.find('\n'));
    output = firstLine;
    // Get command exit status from return value
    int exitStatus = WEXITSTATUS(returnValue);
    return exitStatus;
}

int runToolForOutput(const std::string& configFileName, std::string& output,
                     bool isReadingStderr = false)
{
    std::string command = getValidationToolCommand(configFileName);
    return runToolForOutput(configFileName, output, command, isReadingStderr);
}

void expectFileValid(const std::string& configFileName)
{
    std::string errorMessage = "";
    std::string outputMessage = "";
    EXPECT_EQ(runToolForOutput(configFileName, errorMessage, true), 0);
    EXPECT_EQ(runToolForOutput(configFileName, outputMessage), 0);
    EXPECT_EQ(errorMessage, "");
    EXPECT_EQ(outputMessage, "");
}

void expectFileInvalid(const std::string& configFileName,
                       const std::string& expectedErrorMessage,
                       const std::string& expectedOutputMessage)
{
    std::string errorMessage = "";
    std::string outputMessage = "";
    EXPECT_EQ(runToolForOutput(configFileName, errorMessage, true), 1);
    EXPECT_EQ(runToolForOutput(configFileName, outputMessage), 1);
    EXPECT_EQ(errorMessage, expectedErrorMessage);
    EXPECT_EQ(outputMessage, expectedOutputMessage);
}

std::string writeDataToTmpFile(const json configFileJson)
{
    std::string fileName;
    fileName = createTmpFile();
    std::string jsonData = configFileJson.dump();
    std::ofstream out(fileName);
    out << jsonData;
    out.close();

    return fileName;
}

void expectJsonValid(const json configFileJson)
{
    std::string fileName;
    fileName = writeDataToTmpFile(configFileJson);

    EXPECT_FILE_VALID(fileName);
    unlink(fileName.c_str());
}

void expectJsonInvalid(const json configFileJson,
                       const std::string& expectedErrorMessage,
                       const std::string& expectedOutputMessage)
{
    std::string fileName;
    fileName = writeDataToTmpFile(configFileJson);

    EXPECT_FILE_INVALID(fileName, expectedErrorMessage, expectedOutputMessage);
    unlink(fileName.c_str());
}

void expectCommandLineSyntax(const std::string& configFileName,
                             const std::string& expectedErrorMessage,
                             const std::string& expectedOutputMessage,
                             std::string command, int status)
{
    std::string errorMessage = "";
    std::string outputMessage = "";
    EXPECT_EQ(runToolForOutput(configFileName, errorMessage, command, true),
              status);
    EXPECT_EQ(runToolForOutput(configFileName, outputMessage, command), status);
    EXPECT_EQ(errorMessage, expectedErrorMessage);
    EXPECT_EQ(outputMessage, expectedOutputMessage);
}

TEST(ValidateRegulatorsConfigTest, And)
{
    // Valid.
    {
        json configFile = validConfigFile;
        json andAction =
            R"(
                {
                 "and": [
                    { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } },
                    { "i2c_compare_byte": { "register": "0xA1", "value": "0x00" } }
                  ]
                }
            )"_json;
        configFile["rules"][0]["actions"].push_back(andAction);
        EXPECT_JSON_VALID(configFile);
    }

    // Invalid: actions property value is an empty array.
    {
        json configFile = validConfigFile;
        json andAction =
            R"(
                {
                 "and": []
                }
            )"_json;
        configFile["rules"][0]["actions"].push_back(andAction);
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }

    // Invalid: actions property has incorrect value data type.
    {
        json configFile = validConfigFile;
        json andAction =
            R"(
                {
                 "and": true
                }
            )"_json;
        configFile["rules"][0]["actions"].push_back(andAction);
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }

    // Invalid: actions property value contains wrong element type
    {
        json configFile = validConfigFile;
        json andAction =
            R"(
                {
                 "and": ["foo"]
                }
            )"_json;
        configFile["rules"][0]["actions"].push_back(andAction);
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'foo' is valid under each of {u'required': "
            "[u'compare_presence']}, {u'required': [u'compare_vpd']}, "
            "{u'required': [u'i2c_compare_bit']}, {u'required': "
            "[u'i2c_compare_byte']}, {u'required': [u'i2c_compare_bytes']}, "
            "{u'required': [u'i2c_write_bit']}, {u'required': "
            "[u'i2c_write_byte']}, {u'required': [u'i2c_write_bytes']}, "
            "{u'required': [u'if']}, {u'required': [u'not']}, {u'required': "
            "[u'or']}, {u'required': [u'pmbus_write_vout_command']}, "
            "{u'required': [u'pmbus_read_sensor']}, {u'required': "
            "[u'run_rule']}, {u'required': [u'set_device']}, {u'required': "
            "[u'and']}");
    }
}
TEST(ValidateRegulatorsConfigTest, Chassis)
{
    // Valid: test chassis.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis with required properties.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0].erase("comments");
        configFile["chassis"][0].erase("devices");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis with no number.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0].erase("number");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'number' is a required property");
    }
    // Invalid: test chassis with property comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test chassis with property number wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["number"] = 1.3;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1.3 is not of type u'integer'");
    }
    // Invalid: test chassis with property devices wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test chassis with property comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test chassis with property devices empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test chassis with property number less than 1.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["number"] = 0;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "0 is less than the minimum of 1");
    }
}
TEST(ValidateRegulatorsConfigTest, ComparePresence)
{
    json comparePresenceFile = validConfigFile;
    comparePresenceFile["rules"][0]["actions"][1]["compare_presence"]["fru"] =
        "/system/chassis/motherboard/regulator2";
    comparePresenceFile["rules"][0]["actions"][1]["compare_presence"]["value"] =
        true;
    // Valid.
    {
        json configFile = comparePresenceFile;
        EXPECT_JSON_VALID(configFile);
    }

    // Invalid: no FRU property.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"].erase("fru");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'fru' is a required property");
    }

    // Invalid: FRU property length is string less than 1.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'' is too short");
    }

    // Invalid: no value property.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }

    // Invalid: value property type is not boolean.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'1' is not of type u'boolean'");
    }

    // Invalid: FRU property type is not string.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
}
TEST(ValidateRegulatorsConfigTest, CompareVpd)
{
    json compareVpdFile = validConfigFile;
    compareVpdFile["rules"][0]["actions"][1]["compare_vpd"]["fru"] =
        "/system/chassis/motherboard/regulator2";
    compareVpdFile["rules"][0]["actions"][1]["compare_vpd"]["keyword"] = "CCIN";
    compareVpdFile["rules"][0]["actions"][1]["compare_vpd"]["value"] = "2D35";

    // Valid.
    {
        json configFile = compareVpdFile;
        EXPECT_JSON_VALID(configFile);
    }

    // Invalid: no FRU property.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"].erase("fru");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'fru' is a required property");
    }

    // Invalid: no keyword property.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"].erase("keyword");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'keyword' is a required property");
    }

    // Invalid: no value property.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }

    // Invalid: property FRU wrong type.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["fru"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }

    // Invalid: property FRU is string less than 1.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'' is too short");
    }

    // Invalid: property keyword is not "CCIN", "Manufacturer", "Model",
    // "PartNumber"
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["keyword"] =
            "Number";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'Number' is not one of [u'CCIN', "
                            "u'Manufacturer', u'Model', u'PartNumber']");
    }

    // Invalid: property value wrong type.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
}
TEST(ValidateRegulatorsConfigTest, Configuration)
{
    json configurationFile = validConfigFile;
    configurationFile["chassis"][0]["devices"][0]["configuration"]["comments"]
                     [0] = "Set rail to 1.25V using standard rule";
    configurationFile["chassis"][0]["devices"][0]["configuration"]["volts"] =
        1.25;
    configurationFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
        "set_voltage_rule";
    // Valid: test configuration with property rule_id and with no actions.
    {
        json configFile = configurationFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test configuration with property actions and with no rule_id.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"][0]
                  ["compare_presence"]["fru"] =
                      "/system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"][0]
                  ["compare_presence"]["value"] = true;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: comments not specified (optional property).
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "comments");
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: volts not specified (optional property).
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase("volts");
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: configuration is property of a rail (vs. a device).
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["configuration"]
                  ["comments"][0] = "Set rail to 1.25V using standard rule";
        configFile["chassis"][0]["devices"][0]["rails"][0]["configuration"]
                  ["volts"] = 1.25;
        configFile["chassis"][0]["devices"][0]["rails"][0]["configuration"]
                  ["rule_id"] = "set_voltage_rule";
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: comments property has wrong data type (not an array).
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["comments"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // Invalid: test configuration with both actions and rule_id properties.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"][0]
                  ["compare_presence"]["fru"] =
                      "/system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"][0]
                  ["compare_presence"]["value"] = true;
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "{u'volts': 1.25, u'comments': [u'Set rail to 1.25V using standard "
            "rule'], u'actions': [{u'compare_presence': {u'value': True, "
            "u'fru': u'/system/chassis/motherboard/cpu3'}}], u'rule_id': "
            "u'set_voltage_rule'} is valid under each of {u'required': "
            "[u'actions']}, {u'required': [u'rule_id']}");
    }
    // Invalid: test configuration with no rule_id and actions.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "{u'volts': 1.25, u'comments': [u'Set rail to 1.25V using standard "
            "rule']} is not valid under any of the given schemas");
    }
    // Invalid: test configuration with property volts wrong type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["volts"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'number'");
    }
    // Invalid: test configuration with property rule_id wrong type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test configuration with property actions wrong type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test configuration with property comments empty array.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["comments"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test configuration with property rule_id wrong format.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
            "id!";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id!' does not match u'^[A-Za-z0-9_]+$'");
    }
    // Invalid: test configuration with property actions empty array.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
}
TEST(ValidateRegulatorsConfigTest, Device)
{

    // Valid: test devices.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test devices with required properties.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("comments");
        configFile["chassis"][0]["devices"][0].erase("presence_detection");
        configFile["chassis"][0]["devices"][0].erase("configuration");
        configFile["chassis"][0]["devices"][0].erase("rails");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test devices with no id.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id' is a required property");
    }
    // Invalid: test devices with no is_regulator.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("is_regulator");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'is_regulator' is a required property");
    }
    // Invalid: test devices with no fru.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("fru");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'fru' is a required property");
    }
    // Invalid: test devices with no i2c_interface.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("i2c_interface");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'i2c_interface' is a required property");
    }
    // Invalid: test devices with property comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test devices with property id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test devices with property is_regulator wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["is_regulator"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'boolean'");
    }
    // Invalid: test devices with property fru wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["fru"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test devices with property i2c_interface wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'object'");
    }
    // Invalid: test devices with property presence_detection wrong
    // type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is valid under each of {u'required': "
                            "[u'actions']}, {u'required': [u'rule_id']}");
    }
    // Invalid: test devices with property configuration wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["configuration"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is valid under each of {u'required': "
                            "[u'actions']}, {u'required': [u'rule_id']}");
    }
    // Invalid: test devices with property rails wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test devices with property comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test devices with property fru length less than 1.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'' is too short");
    }
    // Invalid: test devices with property id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["id"] = "id#";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id#' does not match u'^[A-Za-z0-9_]+$'");
    }
    // Invalid: test devices with property rails empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
}
TEST(ValidateRegulatorsConfigTest, I2CCompareBit)
{
    json i2cCompareBitFile = validConfigFile;
    i2cCompareBitFile["rules"][0]["actions"][1]["i2c_compare_bit"]["register"] =
        "0xA0";
    i2cCompareBitFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] =
        3;
    i2cCompareBitFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = 1;
    // Valid: test rule actions i2c_compare_bit.
    {
        json configFile = i2cCompareBitFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test i2c_compare_bit with no register.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // Invalid: test i2c_compare_bit with no position.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"].erase(
            "position");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'position' is a required property");
    }
    // Invalid: test i2c_compare_bit with no value.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }
    // Invalid: test i2c_compare_bit with register wrong type.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_compare_bit with register wrong format.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["register"] =
            "0xA00";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xA00' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bit with position wrong type.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] =
            3.1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "3.1 is not of type u'integer'");
    }
    // Invalid: test i2c_compare_bit with position greater than 7.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] = 8;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "8 is greater than the maximum of 7");
    }
    // Invalid: test i2c_compare_bit with position less than 0.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] =
            -1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "-1 is less than the minimum of 0");
    }
    // Invalid: test i2c_compare_bit with value wrong type.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'1' is not of type u'integer'");
    }
    // Invalid: test i2c_compare_bit with value greater than 1.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = 2;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "2 is greater than the maximum of 1");
    }
    // Invalid: test i2c_compare_bit with value less than 0.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = -1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "-1 is less than the minimum of 0");
    }
}
TEST(ValidateRegulatorsConfigTest, I2CCompareByte)
{
    json i2cCompareByteFile = validConfigFile;
    i2cCompareByteFile["rules"][0]["actions"][1]["i2c_compare_byte"]
                      ["register"] = "0x82";
    i2cCompareByteFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
        "0x40";
    i2cCompareByteFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
        "0x7F";
    // Valid: test i2c_compare_byte with all properties.
    {
        json configFile = i2cCompareByteFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test i2c_compare_byte with all required properties.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"].erase("mask");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test i2c_compare_byte with no register.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // Invalid: test i2c_compare_byte with no value.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }
    // Invalid: test i2c_compare_byte with property register wrong type.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_compare_byte with property value wrong type.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_compare_byte with property mask wrong type.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_compare_byte with property register more than 2 hex
    // digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property value more than 2 hex
    // digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property mask more than 2 hex digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property register less than 2 hex
    // digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property value less than 2 hex
    // digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property mask less than 2 hex digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property register no leading prefix.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property value no leading prefix.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property mask no leading prefix.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] = "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property register invalid hex digit.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property value invalid hex digit.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_byte with property mask invalid hex digit.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
}
TEST(ValidateRegulatorsConfigTest, I2CCompareBytes)
{
    json i2cCompareBytesFile = validConfigFile;
    i2cCompareBytesFile["rules"][0]["actions"][1]["i2c_compare_bytes"]
                       ["register"] = "0x82";
    i2cCompareBytesFile["rules"][0]["actions"][1]["i2c_compare_bytes"]
                       ["values"] = {"0x02", "0x73"};
    i2cCompareBytesFile["rules"][0]["actions"][1]["i2c_compare_bytes"]
                       ["masks"] = {"0x7F", "0x7F"};
    // Valid: test i2c_compare_bytes.
    {
        json configFile = i2cCompareBytesFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test i2c_compare_bytes with all required properties.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"].erase(
            "masks");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test i2c_compare_bytes with no register.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // Invalid: test i2c_compare_bytes with no values.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"].erase(
            "values");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'values' is a required property");
    }
    // Invalid: test i2c_compare_bytes with property values as empty array.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test i2c_compare_bytes with property masks as empty array.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test i2c_compare_bytes with property register wrong type.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_compare_bytes with property values wrong type.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // Invalid: test i2c_compare_bytes with property masks wrong type.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // Invalid: test i2c_compare_bytes with property register more than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property values more than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property masks more than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property register less than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property values less than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property masks less than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property register no leading prefix.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property values no leading prefix.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property masks no leading prefix.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property register invalid hex digit.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property values invalid hex digit.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_compare_bytes with property masks invalid hex digit.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
}
TEST(ValidateRegulatorsConfigTest, I2CInterface)
{
    // Valid: test i2c_interface.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: testi2c_interface with no bus.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"].erase("bus");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'bus' is a required property");
    }
    // Invalid: test i2c_interface with no address.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"].erase(
            "address");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'address' is a required property");
    }
    // Invalid: test i2c_interface with property bus wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["bus"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'integer'");
    }
    // Invalid: test i2c_interface with property address wrong
    // type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["address"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test i2c_interface with property bus less than
    // 0.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["bus"] = -1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "-1 is less than the minimum of 0");
    }
    // Invalid: test i2c_interface with property address wrong
    // format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["address"] =
            "0x700";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x700' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
}
TEST(ValidateRegulatorsConfigTest, I2CWriteBit)
{
    json i2cWriteBitFile = validConfigFile;
    i2cWriteBitFile["rules"][0]["actions"][1]["i2c_write_bit"]["register"] =
        "0xA0";
    i2cWriteBitFile["rules"][0]["actions"][1]["i2c_write_bit"]["position"] = 3;
    i2cWriteBitFile["rules"][0]["actions"][1]["i2c_write_bit"]["value"] = 1;
    // Valid: test rule actions i2c_write_bit.
    {
        json configFile = i2cWriteBitFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test i2c_write_bit with no register.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"].erase("register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // Invalid: test i2c_write_bit with no position.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"].erase("position");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'position' is a required property");
    }
    // Invalid: test i2c_write_bit with no value.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }
    // Invalid: test i2c_write_bit with register wrong type.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_write_bit with register wrong format.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["register"] =
            "0xA00";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xA00' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bit with position wrong type.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["position"] = 3.1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "3.1 is not of type u'integer'");
    }
    // Invalid: test i2c_write_bit with position greater than 7.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["position"] = 8;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "8 is greater than the maximum of 7");
    }
    // Invalid: test i2c_write_bit with position less than 0.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["position"] = -1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "-1 is less than the minimum of 0");
    }
    // Invalid: test i2c_write_bit with value wrong type.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'1' is not of type u'integer'");
    }
    // Invalid: test i2c_write_bit with value greater than 1.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["value"] = 2;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "2 is greater than the maximum of 1");
    }
    // Invalid: test i2c_write_bit with value less than 0.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["value"] = -1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "-1 is less than the minimum of 0");
    }
}
TEST(ValidateRegulatorsConfigTest, I2CWriteByte)
{
    json i2cWriteByteFile = validConfigFile;
    i2cWriteByteFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
        "0x82";
    i2cWriteByteFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] =
        "0x40";
    i2cWriteByteFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] =
        "0x7F";
    // Valid: test i2c_write_byte with all properties.
    {
        json configFile = i2cWriteByteFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test i2c_write_byte with all required properties.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"].erase("mask");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test i2c_write_byte with no register.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // Invalid: test i2c_write_byte with no value.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }
    // Invalid: test i2c_write_byte with property register wrong type.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_write_byte with property value wrong type.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_write_byte with property mask wrong type.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_write_byte with property register more than 2 hex
    // digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property value more than 2 hex
    // digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property mask more than 2 hex digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property register less than 2 hex
    // digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property value less than 2 hex
    // digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] = "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property mask less than 2 hex digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] = "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property register no leading prefix.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property value no leading prefix.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] = "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property mask no leading prefix.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] = "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property register invalid hex digit.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property value invalid hex digit.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_byte with property mask invalid hex digit.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] = "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
}
TEST(ValidateRegulatorsConfigTest, I2CWriteBytes)
{
    json i2cWriteBytesFile = validConfigFile;
    i2cWriteBytesFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
        "0x82";
    i2cWriteBytesFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"] = {
        "0x02", "0x73"};
    i2cWriteBytesFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"] = {
        "0x7F", "0x7F"};
    // Valid: test i2c_write_bytes.
    {
        json configFile = i2cWriteBytesFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test i2c_write_bytes with all required properties.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"].erase("masks");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test i2c_write_bytes with no register.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // Invalid: test i2c_write_bytes with no values.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"].erase("values");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'values' is a required property");
    }
    // Invalid: test i2c_write_bytes with property values as empty array.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test i2c_write_bytes with property masks as empty array.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test i2c_write_bytes with property register wrong type.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // Invalid: test i2c_write_bytes with property values wrong type.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // Invalid: test i2c_write_bytes with property masks wrong type.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // Invalid: test i2c_write_bytes with property register more than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property values more than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"][0] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property masks more than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"][0] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property register less than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property values less than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"][0] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property masks less than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"][0] =
            "0x8";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property register no leading prefix.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property values no leading prefix.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"][0] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property masks no leading prefix.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"][0] =
            "82";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'82' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property register invalid hex digit.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property values invalid hex digit.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"][0] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // Invalid: test i2c_write_bytes with property masks invalid hex digit.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"][0] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xG1' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
}
TEST(ValidateRegulatorsConfigTest, If)
{
    json ifFile = validConfigFile;
    ifFile["rules"][2]["actions"][0]["if"]["condition"]["run_rule"] =
        "set_voltage_rule";
    ifFile["rules"][2]["actions"][0]["if"]["then"][0]["run_rule"] =
        "read_sensors_rule";
    ifFile["rules"][2]["actions"][0]["if"]["else"][0]["run_rule"] =
        "read_sensors_rule";
    ifFile["rules"][2]["id"] = "rule_if";
    // Valid: test if.
    {
        json configFile = ifFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test if with required properties.
    {
        json configFile = ifFile;
        configFile["rules"][2]["actions"][0]["if"].erase("else");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test if with no property condition.
    {
        json configFile = ifFile;
        configFile["rules"][2]["actions"][0]["if"].erase("condition");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'condition' is a required property");
    }
    // Invalid: test if with no property then.
    {
        json configFile = ifFile;
        configFile["rules"][2]["actions"][0]["if"].erase("then");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'then' is a required property");
    }
    // Invalid: test if with property then empty array.
    {
        json configFile = ifFile;
        configFile["rules"][2]["actions"][0]["if"]["then"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test if with property else empty array.
    {
        json configFile = ifFile;
        configFile["rules"][2]["actions"][0]["if"]["else"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test if with property condition wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][2]["actions"][0]["if"]["condition"] = 1;
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "1 is valid under each of {u'required': [u'compare_presence']}, "
            "{u'required': [u'compare_vpd']}, {u'required': "
            "[u'i2c_compare_bit']}, {u'required': [u'i2c_compare_byte']}, "
            "{u'required': [u'i2c_compare_bytes']}, {u'required': "
            "[u'i2c_write_bit']}, {u'required': [u'i2c_write_byte']}, "
            "{u'required': [u'i2c_write_bytes']}, {u'required': [u'if']}, "
            "{u'required': [u'not']}, {u'required': [u'or']}, {u'required': "
            "[u'pmbus_write_vout_command']}, {u'required': "
            "[u'pmbus_read_sensor']}, {u'required': [u'run_rule']}, "
            "{u'required': [u'set_device']}, {u'required': [u'and']}");
    }
    // Invalid: test if with property then wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][2]["actions"][0]["if"]["then"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // Invalid: test if with property else wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][2]["actions"][0]["if"]["else"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
}
TEST(ValidateRegulatorsConfigTest, Not)
{
    json notFile = validConfigFile;
    notFile["rules"][0]["actions"][1]["not"]["i2c_compare_byte"]["register"] =
        "0xA0";
    notFile["rules"][0]["actions"][1]["not"]["i2c_compare_byte"]["value"] =
        "0xFF";
    // Valid: test not.
    {
        json configFile = notFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test not with wrong type.
    {
        json configFile = notFile;
        configFile["rules"][0]["actions"][1]["not"] = 1;
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "1 is valid under each of {u'required': [u'compare_presence']}, "
            "{u'required': [u'compare_vpd']}, {u'required': "
            "[u'i2c_compare_bit']}, {u'required': [u'i2c_compare_byte']}, "
            "{u'required': [u'i2c_compare_bytes']}, {u'required': "
            "[u'i2c_write_bit']}, {u'required': [u'i2c_write_byte']}, "
            "{u'required': [u'i2c_write_bytes']}, {u'required': [u'if']}, "
            "{u'required': [u'not']}, {u'required': [u'or']}, {u'required': "
            "[u'pmbus_write_vout_command']}, {u'required': "
            "[u'pmbus_read_sensor']}, {u'required': [u'run_rule']}, "
            "{u'required': [u'set_device']}, {u'required': [u'and']}");
    }
}
TEST(ValidateRegulatorsConfigTest, Or)
{
    json orFile = validConfigFile;
    orFile["rules"][0]["actions"][1]["or"][0]["i2c_compare_byte"]["register"] =
        "0xA0";
    orFile["rules"][0]["actions"][1]["or"][0]["i2c_compare_byte"]["value"] =
        "0x00";
    orFile["rules"][0]["actions"][1]["or"][1]["i2c_compare_byte"]["register"] =
        "0xA1";
    orFile["rules"][0]["actions"][1]["or"][1]["i2c_compare_byte"]["value"] =
        "0x00";
    // Valid: test or.
    {
        json configFile = orFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test or with empty array.
    {
        json configFile = orFile;
        configFile["rules"][0]["actions"][1]["or"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test or with wrong type.
    {
        json configFile = orFile;
        configFile["rules"][0]["actions"][1]["or"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
}
TEST(ValidateRegulatorsConfigTest, PmbusReadSensor)
{
    json pmbusReadSensorFile = validConfigFile;
    pmbusReadSensorFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["type"] =
        "vout";
    pmbusReadSensorFile["rules"][0]["actions"][1]["pmbus_read_sensor"]
                       ["command"] = "0x8B";
    pmbusReadSensorFile["rules"][0]["actions"][1]["pmbus_read_sensor"]
                       ["format"] = "linear_16";
    pmbusReadSensorFile["rules"][0]["actions"][1]["pmbus_read_sensor"]
                       ["exponent"] = -8;
    // Valid: test pmbus_read_sensor.
    {
        json configFile = pmbusReadSensorFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test pmbus_read_sensor with required properties.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase(
            "exponent");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test pmbus_read_sensor with no type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase("type");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'type' is a required property");
    }
    // Invalid: test pmbus_read_sensor with no command.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase(
            "command");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'command' is a required property");
    }
    // Invalid: test pmbus_read_sensor with no format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase(
            "format");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'format' is a required property");
    }
    // Invalid: test pmbus_read_sensor with property type wrong type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["type"] =
            true;
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "True is not one of [u'iout', u'iout_peak', u'iout_valley', "
            "u'pout', u'temperature', u'temperature_peak', u'vout', "
            "u'vout_peak', u'vout_valley']");
    }
    // Invalid: test pmbus_read_sensor with property command wrong type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["command"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test pmbus_read_sensor with property format wrong type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["format"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not one of [u'linear_11', u'linear_16']");
    }
    // Invalid: test pmbus_read_sensor with property exponent wrong type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["exponent"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'integer'");
    }
    // Invalid: test pmbus_read_sensor with property type wrong format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["type"] =
            "foo";
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'foo' is not one of [u'iout', u'iout_peak', u'iout_valley', "
            "u'pout', u'temperature', u'temperature_peak', u'vout', "
            "u'vout_peak', u'vout_valley']");
    }
    // Invalid: test pmbus_read_sensor with property command wrong format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["command"] =
            "0x8B0";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8B0' does not match u'^0x[0-9a-fA-F]{2}$'");
    }
    // Invalid: test pmbus_read_sensor with property format wrong format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["format"] =
            "foo";
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'foo' is not one of [u'linear_11', u'linear_16']");
    }
}
TEST(ValidateRegulatorsConfigTest, PmbusWriteVoutCommand)
{
    json pmbusWriteVoutCommandFile = validConfigFile;
    pmbusWriteVoutCommandFile["rules"][0]["actions"][1]
                             ["pmbus_write_vout_command"]["volts"] = 1.03;
    pmbusWriteVoutCommandFile["rules"][0]["actions"][1]
                             ["pmbus_write_vout_command"]["format"] = "linear";
    pmbusWriteVoutCommandFile["rules"][0]["actions"][1]
                             ["pmbus_write_vout_command"]["exponent"] = -8;
    pmbusWriteVoutCommandFile["rules"][0]["actions"][1]
                             ["pmbus_write_vout_command"]["is_verified"] = true;
    // Valid: test pmbus_write_vout_command.
    {
        json configFile = pmbusWriteVoutCommandFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test pmbus_write_vout_command with required properties.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"].erase(
            "volts");
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"].erase(
            "exponent");
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"].erase(
            "is_verified");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test pmbus_write_vout_command with no format.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"].erase(
            "format");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'format' is a required property");
    }
    // Invalid: test pmbus_write_vout_command with property volts wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["volts"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'number'");
    }
    // Invalid: test pmbus_write_vout_command with property format wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["format"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not one of [u'linear']");
    }
    // Invalid: test pmbus_write_vout_command with property exponent wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["exponent"] = 1.3;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1.3 is not of type u'integer'");
    }
    // Invalid: test pmbus_write_vout_command with property is_verified wrong
    // type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["is_verified"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'boolean'");
    }
    // Invalid: test pmbus_write_vout_command with property format wrong format.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["format"] = "foo";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'foo' is not one of [u'linear']");
    }
}
TEST(ValidateRegulatorsConfigTest, PresenceDetection)
{
    json presenceDetectionFile = validConfigFile;
    presenceDetectionFile
        ["chassis"][0]["devices"][0]["presence_detection"]["comments"][0] =
            "Regulator is only present on the FooBar backplane";
    presenceDetectionFile["chassis"][0]["devices"][0]["presence_detection"]
                         ["rule_id"] = "set_voltage_rule";
    // Valid: test presence_detection with only property rule_id.
    {
        json configFile = presenceDetectionFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test presence_detection with only property actions.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["presence_detection"]["actions"]
                  [0]["compare_presence"]["fru"] =
                      "/system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["presence_detection"]["actions"]
                  [0]["compare_presence"]["value"] = true;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "comments");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test presence_detection with both property rule_id and actions.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]["actions"]
                  [0]["compare_presence"]["fru"] =
                      "/system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["presence_detection"]["actions"]
                  [0]["compare_presence"]["value"] = true;
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "{u'comments': [u'Regulator is only present on the FooBar "
            "backplane'], u'actions': [{u'compare_presence': {u'value': True, "
            "u'fru': u'/system/chassis/motherboard/cpu3'}}], u'rule_id': "
            "u'set_voltage_rule'} is valid under each of "
            "{u'required': [u'actions']}, {u'required': [u'rule_id']}");
    }
    // Invalid: test presence_detection with no rule_id and actions.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "{u'comments': [u'Regulator is only present on the FooBar "
            "backplane']} is not valid under any of the given schemas");
    }
    // Invalid: test presence_detection with property comments wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test presence_detection with property rule_id wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["rule_id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test presence_detection with property actions wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["actions"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test presence_detection with property rule_id wrong format.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["rule_id"] = "id@";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id@' does not match u'^[A-Za-z0-9_]+$'");
    }
    // Invalid: test presence_detection with property comments empty array.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test presence_detection with property actions empty array.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
}
TEST(ValidateRegulatorsConfigTest, Rail)
{
    // Valid: test rail.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test rail with required properties.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0].erase("comments");
        configFile["chassis"][0]["devices"][0]["rails"][0].erase(
            "configuration");
        configFile["chassis"][0]["devices"][0]["rails"][0].erase(
            "sensor_monitoring");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test rail with no id.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id' is a required property");
    }
    // Invalid: test rail with comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test rail with id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test rail with configuration wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["configuration"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is valid under each of {u'required': "
                            "[u'actions']}, {u'required': [u'rule_id']}");
    }
    // Invalid: test rail with sensor_monitoring wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]
                  ["sensor_monitoring"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is valid under each of {u'required': "
                            "[u'actions']}, {u'required': [u'rule_id']}");
    }
    // Invalid: test rail with comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["comments"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test rail with id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["id"] = "id~";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id~' does not match u'^[A-Za-z0-9_]+$'");
    }
}
TEST(ValidateRegulatorsConfigTest, Rule)
{
    // valid test comments property, id property,
    // action property specified.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }

    // valid test rule with no comments
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("comments");
        EXPECT_JSON_VALID(configFile);
    }

    // invalid test comments property has invalid value type
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["comments"] = {1};
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }

    // invalid test rule with no ID
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id' is a required property");
    }

    // invalid test id property has invalid value type (not string)
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }

    // invalid test id property has invalid value
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["id"] = "foo%";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'foo%' does not match u'^[A-Za-z0-9_]+$'");
    }

    // invalid test rule with no actions property
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("actions");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'actions' is a required property");
    }

    // valid test rule with multiple actions
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["run_rule"] = "read_sensors_rule";
        EXPECT_JSON_VALID(configFile);
    }

    // invalid test actions property has invalid value type (not an array)
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }

    // invalid test actions property has invalid value of action
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0] = "foo";
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'foo' is valid under each of {u'required': "
            "[u'compare_presence']}, {u'required': [u'compare_vpd']}, "
            "{u'required': [u'i2c_compare_bit']}, {u'required': "
            "[u'i2c_compare_byte']}, {u'required': [u'i2c_compare_bytes']}, "
            "{u'required': [u'i2c_write_bit']}, {u'required': "
            "[u'i2c_write_byte']}, {u'required': [u'i2c_write_bytes']}, "
            "{u'required': [u'if']}, {u'required': [u'not']}, {u'required': "
            "[u'or']}, {u'required': [u'pmbus_write_vout_command']}, "
            "{u'required': [u'pmbus_read_sensor']}, {u'required': "
            "[u'run_rule']}, {u'required': [u'set_device']}, {u'required': "
            "[u'and']}");
    }

    // invalid test actions property has empty array
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
}
TEST(ValidateRegulatorsConfigTest, RunRule)
{
    json runRuleFile = validConfigFile;
    runRuleFile["rules"][0]["actions"][1]["run_rule"] = "read_sensors_rule";
    // Valid: test run_rule.
    {
        json configFile = runRuleFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test run_rule wrong type.
    {
        json configFile = runRuleFile;
        configFile["rules"][0]["actions"][1]["run_rule"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test run_rule wrong format.
    {
        json configFile = runRuleFile;
        configFile["rules"][0]["actions"][1]["run_rule"] = "set_voltage_rule%";
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'set_voltage_rule%' does not match u'^[A-Za-z0-9_]+$'");
    }
}
TEST(ValidateRegulatorsConfigTest, SensorMonitoring)
{
    // Valid: test rails sensor_monitoring with only property rule id.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test rails sensor_monitoring with only property actions.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"][0]["compare_presence"]["fru"] =
                      "/system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"][0]["compare_presence"]["value"] = true;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["comments"][0] = "comments";
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test rails sensor_monitoring with both property rule_id and
    // actions.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"][0]["compare_presence"]["fru"] =
                      "/system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"][0]["compare_presence"]["value"] = true;
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "{u'rule_id': u'read_sensors_rule', u'actions': "
            "[{u'compare_presence': {u'value': True, u'fru': "
            "u'/system/chassis/motherboard/cpu3'}}]} is valid under each of "
            "{u'required': [u'actions']}, {u'required': [u'rule_id']}");
    }
    // Invalid: test rails sensor_monitoring with no rule_id and actions.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "{} is not valid under any of the given schemas");
    }
    // Invalid: test rails sensor_monitoring with property comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test rails sensor_monitoring with property rule_id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["rule_id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test rails sensor_monitoring with property actions wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test rails sensor_monitoring with property rule_id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["rule_id"] = "id@";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id@' does not match u'^[A-Za-z0-9_]+$'");
    }
    // Invalid: test rails sensor_monitoring with property comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test rails sensor_monitoring with property actions empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
}
TEST(ValidateRegulatorsConfigTest, SetDevice)
{
    json setDeviceFile = validConfigFile;
    setDeviceFile["rules"][0]["actions"][1]["set_device"] = "vdd_regulator";
    // Valid: test set_device.
    {
        json configFile = setDeviceFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test set_device wrong type.
    {
        json configFile = setDeviceFile;
        configFile["rules"][0]["actions"][1]["set_device"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test set_device wrong format.
    {
        json configFile = setDeviceFile;
        configFile["rules"][0]["actions"][1]["set_device"] = "io_expander2%";
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'io_expander2%' does not match u'^[A-Za-z0-9_]+$'");
    }
}
TEST(ValidateRegulatorsConfigTest, DuplicateRuleID)
{
    // Invalid: test duplicate ID in rule.
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["id"] = "set_voltage_rule";
        configFile["rules"][2]["actions"][0]["pmbus_write_vout_command"]
                  ["format"] = "linear";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate rule ID.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, DuplicateChassisNumber)
{
    // Invalid: test duplicate number in chassis.
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["number"] = 1;
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate chassis number.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, DuplicateDeviceID)
{
    // Invalid: test duplicate ID in device.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][1]["id"] = "vdd_regulator";
        configFile["chassis"][0]["devices"][1]["is_regulator"] = true;
        configFile["chassis"][0]["devices"][1]["fru"] =
            "/system/chassis/motherboard/regulator1";
        configFile["chassis"][0]["devices"][1]["i2c_interface"]["bus"] = 2;
        configFile["chassis"][0]["devices"][1]["i2c_interface"]["address"] =
            "0x71";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate device ID.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, DuplicateRailID)
{
    // Invalid: test duplicate ID in rail.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][1]["id"] = "vdd";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate rail ID.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, InfiniteLoops)
{
    // Invalid: test run_rule with infinite loop (rules run each other).
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["run_rule"] = "set_voltage_rule2";
        configFile["rules"][2]["id"] = "set_voltage_rule1";
        configFile["rules"][3]["actions"][0]["run_rule"] = "set_voltage_rule1";
        configFile["rules"][3]["id"] = "set_voltage_rule2";
        EXPECT_JSON_INVALID(configFile,
                            "Infinite loop caused by run_rule actions.", "");
    }
    // Invalid: test run_rule with infinite loop (rule runs itself).
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["run_rule"] = "set_voltage_rule1";
        configFile["rules"][2]["id"] = "set_voltage_rule1";
        EXPECT_JSON_INVALID(configFile,
                            "Infinite loop caused by run_rule actions.", "");
    }
    // Invalid: test run_rule with infinite loop (indirect loop).
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["run_rule"] = "set_voltage_rule2";
        configFile["rules"][2]["id"] = "set_voltage_rule1";
        configFile["rules"][3]["actions"][0]["run_rule"] = "set_voltage_rule3";
        configFile["rules"][3]["id"] = "set_voltage_rule2";
        configFile["rules"][4]["actions"][0]["run_rule"] = "set_voltage_rule1";
        configFile["rules"][4]["id"] = "set_voltage_rule3";
        EXPECT_JSON_INVALID(configFile,
                            "Infinite loop caused by run_rule actions.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, RunRuleValueExists)
{
    // Invalid: test run_rule actions specify a rule ID that does not exist.
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["run_rule"] = "set_voltage_rule2";
        configFile["rules"][2]["id"] = "set_voltage_rule1";
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, SetDeviceValueExists)
{
    // Invalid: test set_device actions specify a device ID that does not exist.
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["set_device"] = "vdd_regulator2";
        configFile["rules"][2]["id"] = "set_voltage_rule1";
        EXPECT_JSON_INVALID(configFile, "Error: Device ID does not exist.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleIDExists)
{
    // Invalid: test rule_id property in configuration specifies a rule ID that
    // does not exist.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
            "set_voltage_rule2";
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.", "");
    }
    // Invalid: test rule_id property in presence_detection specifies a rule ID
    // that does not exist.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["rule_id"] = "set_voltage_rule2";
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.", "");
    }
    // Invalid: test rule_id property in sensor_monitoring specifies a rule ID
    // that does not exist.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["rule_id"] = "set_voltage_rule2";
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, NumberOfElementsInMasks)
{
    // Invalid: test number of elements in masks not equal to number in values
    // in i2c_compare_bytes.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "0x82";
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"] = {
            "0x02", "0x73"};
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"] = {
            "0x7F"};
        EXPECT_JSON_INVALID(configFile,
                            "Error: Invalid i2c_compare_bytes action.", "");
    }
    // Invalid: test number of elements in masks not equal to number in values
    // in i2c_write_bytes.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "0x82";
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"] = {
            "0x02", "0x73"};
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"] = {
            "0x7F"};
        EXPECT_JSON_INVALID(configFile,
                            "Error: Invalid i2c_write_bytes action.", "");
    }
}

TEST(ValidateRegulatorsConfigTest, CommandLineSyntax)
{
    std::string validateTool =
        " ../phosphor-regulators/tools/validate-regulators-config.py ";
    std::string schema = " -s ";
    std::string schemaFile =
        " ../phosphor-regulators/schema/config_schema.json ";
    std::string configuration = " -c ";
    std::string command;
    std::string errorMessage;
    std::string outputMessage;
    std::string outputMessageHelp =
        "usage: validate-regulators-config.py [-h] [-s SCHEMA_FILE]";
    int valid = 0;

    std::string fileName;
    fileName = writeDataToTmpFile(validConfigFile);
    // Valid: -s specified
    {
        command = validateTool + "-s " + schemaFile + configuration + fileName;
        expectCommandLineSyntax(fileName, errorMessage, outputMessage, command,
                                valid);
    }
    // Valid: --schema-file specified
    {
        command = validateTool + "--schema-file " + schemaFile + configuration +
                  fileName;
        expectCommandLineSyntax(fileName, errorMessage, outputMessage, command,
                                valid);
    }
    // Valid: -c specified
    {
        command = validateTool + schema + schemaFile + "-c " + fileName;
        expectCommandLineSyntax(fileName, errorMessage, outputMessage, command,
                                valid);
    }
    // Valid: --configuration-file specified
    {
        command = validateTool + schema + schemaFile + "--configuration-file " +
                  fileName;
        expectCommandLineSyntax(fileName, errorMessage, outputMessage, command,
                                valid);
    }
    // Valid: -h specified
    {
        command = validateTool + "-h ";
        expectCommandLineSyntax(fileName, errorMessage, outputMessageHelp,
                                command, valid);
    }
    // Valid: --help specified
    {
        command = validateTool + "--help ";
        expectCommandLineSyntax(fileName, errorMessage, outputMessageHelp,
                                command, valid);
    }
    // Invalid: -c/--configuration-file not specified
    {
        command = validateTool + schema + schemaFile;
        expectCommandLineSyntax(fileName,
                                "Error: Configuration file is required.",
                                outputMessageHelp, command, 1);
    }
    // Invalid: -s/--schema-file not specified
    {
        command = validateTool + configuration + fileName;
        expectCommandLineSyntax(fileName, "Error: Schema file is required.",
                                outputMessageHelp, command, 1);
    }
    // Invalid: -s specified more than once
    {
        command =
            validateTool + "-s -s " + schemaFile + configuration + fileName;
        expectCommandLineSyntax(fileName, outputMessageHelp, outputMessage,
                                command, 2);
    }
    // Invalid: -c specified more than once
    {
        command = validateTool + schema + schemaFile + "-c -c " + fileName;
        expectCommandLineSyntax(fileName, outputMessageHelp, outputMessage,
                                command, 2);
    }
    // Invalid: No file name specified after -c
    {
        command = validateTool + schema + schemaFile + configuration;
        expectCommandLineSyntax(fileName, outputMessageHelp, outputMessage,
                                command, 2);
    }
    // Invalid: No file name specified after -s
    {
        command = validateTool + schema + configuration + fileName;
        expectCommandLineSyntax(fileName, outputMessageHelp, outputMessage,
                                command, 2);
    }
    // Invalid: File specified after -c does not exist
    {
        command = validateTool + schema + schemaFile + configuration +
                  "../notExistFile";
        expectCommandLineSyntax(fileName, "Traceback (most recent call last):",
                                outputMessage, command, 1);
    }
    // Invalid: File specified after -s does not exist
    {
        command = validateTool + schema + "../notExistFile " + configuration +
                  fileName;
        expectCommandLineSyntax(fileName, "Traceback (most recent call last):",
                                outputMessage, command, 1);
    }
    // Invalid: File specified after -s is not right data format
    {
        std::string wrongFormatFile;
        wrongFormatFile = createTmpFile();
        std::ofstream out(wrongFormatFile);
        out << "foo";
        out.close();
        command =
            validateTool + schema + wrongFormatFile + configuration + fileName;
        expectCommandLineSyntax(fileName, "Traceback (most recent call last):",
                                outputMessage, command, 1);
    }
    // Invalid: File specified after -c is not readable
    {
        std::string notReadableFile;
        notReadableFile = writeDataToTmpFile(validConfigFile);
        command = validateTool + schema + schemaFile + configuration +
                  notReadableFile;
        chmod(notReadableFile.c_str(), 0222);
        expectCommandLineSyntax(
            notReadableFile,
            "Traceback (most recent call last):", outputMessage, command, 1);
        unlink(notReadableFile.c_str());
    }
    // Invalid: File specified after -s is not readable
    {
        std::string notReadableFile;
        notReadableFile = writeDataToTmpFile(validConfigFile);
        command =
            validateTool + schema + notReadableFile + configuration + fileName;
        chmod(notReadableFile.c_str(), 0222);
        expectCommandLineSyntax(
            notReadableFile,
            "Traceback (most recent call last):", outputMessage, command, 1);
        unlink(notReadableFile.c_str());
    }
    // Invalid: Unexpected parameter specified (like -g)
    {
        command = validateTool + schema + schemaFile + configuration + "-g" +
                  fileName;
        expectCommandLineSyntax(fileName, outputMessageHelp, outputMessage,
                                command, 2);
    }
    unlink(fileName.c_str());
}
