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
    std::string command = "python ../tools/validate-regulators-config.py -s \
                           ../schema/config_schema.json -c ";
    command += configFileName;
    return command;
}

int runToolForOutput(const std::string& configFileName, std::string& output,
                     bool isReadingStderr = false)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    std::string command = getValidationToolCommand(configFileName);
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

void expectJsonValid(const json configFileJson)
{
    std::string fileName;
    fileName = createTmpFile();
    std::string jsonData = configFileJson.dump();
    std::ofstream out(fileName);
    out << jsonData;
    out.close();

    EXPECT_FILE_VALID(fileName);
    unlink(fileName.c_str());
}

void expectJsonInvalid(const json configFileJson,
                       const std::string& expectedErrorMessage,
                       const std::string& expectedOutputMessage)
{
    std::string fileName;
    fileName = createTmpFile();
    std::string jsonData = configFileJson.dump();
    std::ofstream out(fileName);
    out << jsonData;
    out.close();

    EXPECT_FILE_INVALID(fileName, expectedErrorMessage, expectedOutputMessage);
    unlink(fileName.c_str());
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
        configFile["rules"][0]["actions"][1]["run_rule"] =
            "set_page0_voltage_rule";
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
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'foo' is not of type u'object'");
    }

    // invalid test actions property has empty array
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
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
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'foo' is not of type u'object'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleComparePresence)
{
    json comparePresenceFile = validConfigFile;
    comparePresenceFile["rules"][0]["actions"][1]["compare_presence"]["fru"] =
        "/system/chassis/motherboard/regulator2";
    comparePresenceFile["rules"][0]["actions"][1]["compare_presence"]["value"] =
        true;
    // test rule actions compare_presence valid.
    {
        json configFile = comparePresenceFile;
        EXPECT_JSON_VALID(configFile);
    }

    // test rule actions compare_presence with no fru invalid.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"].erase("fru");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'fru' is a required property");
    }

    // test rule actions compare_presence with FRU string less than 1 invalid.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'' is too short");
    }

    // test rule actions compare_presence with no value invalid.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }

    // test rule actions compare_presence with value data not boolean invalid.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'1' is not of type u'boolean'");
    }

    // test rule actions compare_presence with FRU not string invalid.
    {
        json configFile = comparePresenceFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleCompareVpd)
{
    json compareVpdFile = validConfigFile;
    compareVpdFile["rules"][0]["actions"][1]["compare_vpd"]["fru"] =
        "/system/chassis/motherboard/regulator2";
    compareVpdFile["rules"][0]["actions"][1]["compare_vpd"]["keyword"] = "CCIN";
    compareVpdFile["rules"][0]["actions"][1]["compare_vpd"]["value"] = "2D35";
    // test rule actions compare_vpd valid
    {
        json configFile = compareVpdFile;
        EXPECT_JSON_VALID(configFile);
    }

    // test rule actions compare_vpd with no FRU invalid
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"].erase("fru");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'fru' is a required property");
    }

    // test rule actions compare_vpd with no keyword invalid
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"].erase("keyword");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'keyword' is a required property");
    }

    // test rule actions compare_vpd with no value invalid
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }

    // test rule actions compare_vpd with FRU not string invalid.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["fru"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }

    // test rule actions compare_vpd with FRU string less than 1 invalid.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'' is too short");
    }

    // test rule actions compare_vpd with keyword not "CCIN", "Manufacturer",
    // "Model", "PartNumber" invalid.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["keyword"] =
            "Number";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'Number' is not one of [u'CCIN', "
                            "u'Manufacturer', u'Model', u'PartNumber']");
    }

    // test rule actions compare_vpd with value not string invalid.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleI2cCompareBit)
{
    json i2cCompareBitFile = validConfigFile;
    i2cCompareBitFile["rules"][0]["actions"][1]["i2c_compare_bit"]["register"] =
        "0xA0";
    i2cCompareBitFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] =
        3;
    i2cCompareBitFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = 1;
    // test rule actions i2c_compare_bit valid.
    {
        json configFile = i2cCompareBitFile;
        EXPECT_JSON_VALID(configFile);
    }
    // test rule actions i2c_compare_bit with no register invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // test rule actions i2c_compare_bit with no position invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"].erase(
            "position");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'position' is a required property");
    }
    // test rule actions i2c_compare_bit with no value invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }
    // test rule actions i2c_compare_bit with register wrong type invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // test rule actions i2c_compare_bit with register wrong format invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["register"] =
            "0xA00";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0xA00' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // test rule actions i2c_compare_bit with position wrong type invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] =
            3.1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "3.1 is not of type u'integer'");
    }
    // test rule actions i2c_compare_bit with position greater than 7 invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] = 8;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "8 is greater than the maximum of 7");
    }
    // test rule actions i2c_compare_bit with position less than 0 invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] =
            -1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "-1 is less than the minimum of 0");
    }
    // test rule actions i2c_compare_bit with value wrong type invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'1' is not of type u'integer'");
    }
    // test rule actions i2c_compare_bit with value greater than 1 invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = 2;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "2 is greater than the maximum of 1");
    }
    // test rule actions i2c_compare_bit with value less than 0 invalid.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = -1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "-1 is less than the minimum of 0");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleI2cCompareByte)
{
    json i2cCompareByteFile = validConfigFile;
    i2cCompareByteFile["rules"][0]["actions"][1]["i2c_compare_byte"]
                      ["register"] = "0x82";
    i2cCompareByteFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
        "0x40";
    i2cCompareByteFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
        "0x7F";
    // test rule actions i2c_compare_byte with all property valid.
    {
        json configFile = i2cCompareByteFile;
        EXPECT_JSON_VALID(configFile);
    }
    // test rule actions i2c_compare_byte with required property valid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"].erase("mask");
        EXPECT_JSON_VALID(configFile);
    }
    // test rule actions i2c_compare_byte with no register invalid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // test rule actions i2c_compare_byte with no value invalid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'value' is a required property");
    }
    // test rule actions i2c_compare_byte with property register wrong type
    // invalid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // test rule actions i2c_compare_byte with property register wrong format
    // invalid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // test rule actions i2c_compare_byte with property value wrong type
    // invalid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // test rule actions i2c_compare_byte with property value wrong format
    // invalid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "0x400";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x400' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // test rule actions i2c_compare_byte with property mask wrong type invalid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // test rule actions i2c_compare_byte with property mask wrong format
    // invalid.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
            "0x7F0";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x7F0' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleI2cCompareBytes)
{
    json i2cCompareBytesFile = validConfigFile;
    i2cCompareBytesFile["rules"][0]["actions"][1]["i2c_compare_bytes"]
                       ["register"] = "0x82";
    i2cCompareBytesFile["rules"][0]["actions"][1]["i2c_compare_bytes"]
                       ["values"] = {"0x02", "0x73"};
    i2cCompareBytesFile["rules"][0]["actions"][1]["i2c_compare_bytes"]
                       ["masks"] = {"0x7F", "0x7F"};
    // test rule actions i2c_compare_bytes valid
    {
        json configFile = i2cCompareBytesFile;
        EXPECT_JSON_VALID(configFile);
    }
    // test rule actions i2c_compare_bytes with all required properties valid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"].erase(
            "masks");
        EXPECT_JSON_VALID(configFile);
    }
    // test rule actions i2c_compare_bytes with no register invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'register' is a required property");
    }
    // test rule actions i2c_compare_bytes with no values invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"].erase(
            "values");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'values' is a required property");
    }
    // test rule actions i2c_compare_bytes with property values as empty array
    // invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // test rule actions i2c_compare_bytes with property masks as empty array
    // invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // test rule actions i2c_compare_bytes with property register wrong type
    // invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'string'");
    }
    // test rule actions i2c_compare_bytes with property values wrong type
    // invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // test rule actions i2c_compare_bytes with property masks wrong type
    // invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // test rule actions i2c_compare_bytes with property register wrong format
    // invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x820' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // test rule actions i2c_compare_bytes with property values wrong format
    // invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "0x020";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x020' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
    // test rule actions i2c_compare_bytes with property masks wrong format
    // invalid
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "0x7F0";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x7F0' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleIf)
{
    json ifFile = validConfigFile;
    ifFile["rules"][0]["actions"][1]["if"]["condition"]["run_rule"] =
        "is_downlevel_regulator";
    ifFile["rules"][0]["actions"][1]["if"]["then"][0]["run_rule"] =
        "configure_downlevel_regulator";
    ifFile["rules"][0]["actions"][1]["if"]["else"][0]["run_rule"] =
        "configure_downlevel_regulator";
    // Valid: test rule actions if.
    {
        json configFile = ifFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test rule actions if with required properties.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"].erase("else");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test rule actions if with no property condition.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"].erase("condition");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'condition' is a required property");
    }
    // Invalid: test rule actions if with no property then.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"].erase("then");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'then' is a required property");
    }
    // Invalid: test rule actions if with property then empty array.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["then"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test rule actions if with property else empty array.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["else"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test rule actions if with property condition wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["condition"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'object'");
    }
    // Invalid: test rule actions if with property then wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["then"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    // Invalid: test rule actions if with property else wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["else"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleNot)
{
    json notFile = validConfigFile;
    notFile["rules"][0]["actions"][1]["not"]["i2c_compare_byte"]["register"] =
        "0xA0";
    notFile["rules"][0]["actions"][1]["not"]["i2c_compare_byte"]["value"] =
        "0xFF";
    // Valid: test rule actions not.
    {
        json configFile = notFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test rule actions not with wrong type.
    {
        json configFile = notFile;
        configFile["rules"][0]["actions"][1]["not"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'object'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleOr)
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
    // Valid: test rule actions or.
    {
        json configFile = orFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test rule actions or with empty array.
    {
        json configFile = orFile;
        configFile["rules"][0]["actions"][1]["or"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test rule actions or with wrong type.
    {
        json configFile = orFile;
        configFile["rules"][0]["actions"][1]["or"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
}
TEST(ValidateRegulatorsConfigTest, RulePmbusReadSensor)
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
    // Valid: test rule actions pmbus_read_sensor.
    {
        json configFile = pmbusReadSensorFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test rule actions pmbus_read_sensor with required properties.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase(
            "exponent");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test rule actions pmbus_read_sensor with no type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase("type");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'type' is a required property");
    }
    // Invalid: test rule actions pmbus_read_sensor with no command.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase(
            "command");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'command' is a required property");
    }
    // Invalid: test rule actions pmbus_read_sensor with no format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase(
            "format");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'format' is a required property");
    }
    // Invalid: test rule actions pmbus_read_sensor with property type wrong
    // type.
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
    // Invalid: test rule actions pmbus_read_sensor with property command wrong
    // type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["command"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test rule actions pmbus_read_sensor with property format wrong
    // type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["format"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not one of [u'linear_11', u'linear_16']");
    }
    // Invalid: test rule actions pmbus_read_sensor with property exponent wrong
    // type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["exponent"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'integer'");
    }
    // Invalid: test rule actions pmbus_read_sensor with property type wrong
    // format.
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
    // Invalid: test rule actions pmbus_read_sensor with property command wrong
    // format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["command"] =
            "0x8B0";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x8B0' does not match u'^0x[0-9a-fA-F]{2}$'");
    }
    // Invalid: test rule actions pmbus_read_sensor with property format wrong
    // format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["format"] =
            "foo";
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'foo' is not one of [u'linear_11', u'linear_16']");
    }
}
TEST(ValidateRegulatorsConfigTest, RulePmbusWriteVoutCommand)
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
    // Valid: test rule actions pmbus_write_vout_command.
    {
        json configFile = pmbusWriteVoutCommandFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test rule actions pmbus_write_vout_command with required
    // properties.
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
    // Invalid: test rule actions pmbus_write_vout_command with no format.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"].erase(
            "format");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'format' is a required property");
    }
    // Invalid: test rule actions pmbus_write_vout_command with property volts
    // wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["volts"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'number'");
    }
    // Invalid: test rule actions pmbus_write_vout_command with property format
    // wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["format"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not one of [u'linear']");
    }
    // Invalid: test rule actions pmbus_write_vout_command with property
    // exponent wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["exponent"] = 1.3;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1.3 is not of type u'integer'");
    }
    // Invalid: test rule actions pmbus_write_vout_command with property
    // is_verified wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["is_verified"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'boolean'");
    }
    // Invalid: test rule actions pmbus_write_vout_command with property format
    // wrong format.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["format"] = "foo";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'foo' is not one of [u'linear']");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleRunRule)
{
    json runRuleFile = validConfigFile;
    runRuleFile["rules"][0]["actions"][1]["run_rule"] = "set_voltage_rule1";
    // Valid: test rule actions run_rule.
    {
        json configFile = runRuleFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test rule actions run_rule wrong type.
    {
        json configFile = runRuleFile;
        configFile["rules"][0]["actions"][1]["run_rule"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test rule actions run_rule wrong format.
    {
        json configFile = runRuleFile;
        configFile["rules"][0]["actions"][1]["run_rule"] = "set_voltage_rule%";
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'set_voltage_rule%' does not match u'^[A-Za-z0-9_]+$'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleSetDevice)
{
    json setDeviceFile = validConfigFile;
    setDeviceFile["rules"][0]["actions"][1]["set_device"] = "io_expander2";
    // Valid: test rule actions set_device.
    {
        json configFile = setDeviceFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test rule actions set_device wrong type.
    {
        json configFile = setDeviceFile;
        configFile["rules"][0]["actions"][1]["set_device"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test rule actions set_device wrong format.
    {
        json configFile = setDeviceFile;
        configFile["rules"][0]["actions"][1]["set_device"] = "io_expander2%";
        EXPECT_JSON_INVALID(
            configFile, "Validation failed.",
            "u'io_expander2%' does not match u'^[A-Za-z0-9_]+$'");
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
TEST(ValidateRegulatorsConfigTest, ChassisDevices)
{

    // Valid: test chassis devices.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis devices with required properties.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("comments");
        configFile["chassis"][0]["devices"][0].erase("presence_detection");
        configFile["chassis"][0]["devices"][0].erase("configuration");
        configFile["chassis"][0]["devices"][0].erase("rails");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis devices with no id.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id' is a required property");
    }
    // Invalid: test chassis devices with no is_regulator.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("is_regulator");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'is_regulator' is a required property");
    }
    // Invalid: test chassis devices with no fru.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("fru");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'fru' is a required property");
    }
    // Invalid: test chassis devices with no i2c_interface.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("i2c_interface");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'i2c_interface' is a required property");
    }
    // Invalid: test chassis devices with property comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test chassis devices with property id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test chassis devices with property is_regulator wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["is_regulator"] = 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'boolean'");
    }
    // Invalid: test chassis devices with property fru wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["fru"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test chassis devices with property i2c_interface wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'object'");
    }
    // Invalid: test chassis devices with property presence_detection wrong
    // type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'object'");
    }
    // Invalid: test chassis devices with property configuration wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["configuration"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'object'");
    }
    // Invalid: test chassis devices with property rails wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test chassis devices with property comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test chassis devices with property fru lenth less than 1.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'' is too short");
    }
    // Invalid: test chassis devices with property id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["id"] = "id#";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id#' does not match u'^[A-Za-z0-9_]+$'");
    }
    // Invalid: test chassis devices with property rails empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
}
TEST(ValidateRegulatorsConfigTest, ChassisDevicesI2cInterface)
{
    // Valid: test chassis devices i2c_interface.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis devices i2c_interface with no bus.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"].erase("bus");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'bus' is a required property");
    }
    // Invalid: test chassis devices i2c_interface with no address.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"].erase(
            "address");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'address' is a required property");
    }
    // Invalid: test chassis devices i2c_interface with property bus wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["bus"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'integer'");
    }
    // Invalid: test chassis devices i2c_interface with property address wrong
    // type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["address"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test chassis devices i2c_interface with property bus less than
    // 0.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["bus"] = -1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "-1 is less than the minimum of 0");
    }
    // Invalid: test chassis devices i2c_interface with property address wrong
    // format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["address"] =
            "0x700";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'0x700' does not match u'^0x[0-9A-Fa-f]{2}$'");
    }
}
TEST(ValidateRegulatorsConfigTest, ChassisDevicesPresenceDetection)
{
    json presenceDetectionFile = validConfigFile;
    presenceDetectionFile
        ["chassis"][0]["devices"][0]["presence_detection"]["comments"][0] =
            "Regulator is only present on the FooBar backplane";
    presenceDetectionFile["chassis"][0]["devices"][0]["presence_detection"]
                         ["rule_id"] = "is_foobar_backplane_installed_rule";
    // Valid: test chassis devices presence_detection with only property
    // rule_id.
    {
        json configFile = presenceDetectionFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis devices presence_detection with only property
    // actions.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["presence_detection"]["actions"]
                  [0]["compare_presence"]["fru"] =
                      "/system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["presence_detection"]["actions"]
                  [0]["compare_presence"]["value"] = true;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis devices presence_detection with both property
    // rule_id and actions.
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
            "u'is_foobar_backplane_installed_rule'} is valid under each of "
            "{u'required': [u'actions']}, {u'required': [u'rule_id']}");
    }
    // Invalid: test chassis devices presence_detection with no rule_id and
    // actions.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'rule_id' is a required property");
    }
    // Invalid: test chassis devices presence_detection with property comments
    // wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test chassis devices presence_detection with property rule_id
    // wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["rule_id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test chassis devices presence_detection with property actions
    // wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["presence_detection"]["actions"]
                  [0] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'object'");
    }
    // Invalid: test chassis devices presence_detection with property rule_id
    // wrong format.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["rule_id"] = "id@";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id@' does not match u'^[A-Za-z0-9_]+$'");
    }
    // Invalid: test chassis devices presence_detection with property comments
    // empty array.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test chassis devices presence_detection with property actions
    // empty array.
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
TEST(ValidateRegulatorsConfigTest, ChassisDevicesConfiguration)
{
    json configurationFile = validConfigFile;
    configurationFile["chassis"][0]["devices"][0]["configuration"]["comments"]
                     [0] = "Set rail to 1.25V using standard rule";
    configurationFile["chassis"][0]["devices"][0]["configuration"]["volts"] =
        1.25;
    configurationFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
        "set_voltage_rule";
    // Valid: test chassis devices configuration with property rule_id and with
    // no actions.
    {
        json configFile = configurationFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis devices configuration with property actions and with
    // no rule_id.
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
    // Invalid: test chassis devices configuration with both property actions
    // rule_id.
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
    // Invalid: test chassis devices configuration with no rule_id and actions.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'rule_id' is a required property");
    }
    // Invalid: test chassis devices configuration with property volts wrong
    // type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["volts"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'number'");
    }
    // Invalid: test chassis devices configuration with property rule_id wrong
    // type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test chassis devices configuration with property actions wrong
    // type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test chassis devices configuration with property comments empty
    // array.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["comments"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test chassis devices configuration with property rule_id wrong
    // format.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
            "id!";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id!' does not match u'^[A-Za-z0-9_]+$'");
    }
    // Invalid: test chassis devices configuration with property actions empty
    // array.
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
TEST(ValidateRegulatorsConfigTest, ChassisDevicesRails)
{
    // Valid: test chassis devices rails.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis devices rails with required properties.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0].erase("comments");
        configFile["chassis"][0]["devices"][0]["rails"][0].erase(
            "configuration");
        configFile["chassis"][0]["devices"][0]["rails"][0].erase(
            "sensor_monitoring");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis devices rails with no id.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id' is a required property");
    }
    // Invalid: test chassis devices rails with comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test chassis devices rails with id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test chassis devices rails with configuration wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["configuration"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'object'");
    }
    // Invalid: test chassis devices rails with sensor_monitoring wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]
                  ["sensor_monitoring"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'object'");
    }
    // Invalid: test chassis devices rails with comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["comments"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test chassis devices rails with id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["id"] = "id~";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id~' does not match u'^[A-Za-z0-9_]+$'");
    }
}
TEST(ValidateRegulatorsConfigTest, ChassisDevicesRailSensorMonitoring)
{
    // Valid: test chassis devices rails sensor_monitoring with only property
    // rule id.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis devices rails sensor_monitoring with only property
    // actions.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"][0]["compare_presence"]["fru"] =
                      "/system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"][0]["compare_presence"]["value"] = true;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis devices rails sensor_monitoring with both property
    // rule_id and actions.
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
    // Invalid: test chassis devices rails sensor_monitoring with no rule_id and
    // actions.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'rule_id' is a required property");
    }
    // Invalid: test chassis devices rails sensor_monitoring with property
    // comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'array'");
    }
    // Invalid: test chassis devices rails sensor_monitoring with property
    // rule_id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["rule_id"] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'string'");
    }
    // Invalid: test chassis devices rails sensor_monitoring with property
    // actions wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"][0] = true;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "True is not of type u'object'");
    }
    // Invalid: test chassis devices rails sensor_monitoring with property
    // rule_id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["rule_id"] = "id@";
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'id@' does not match u'^[A-Za-z0-9_]+$'");
    }
    // Invalid: test chassis devices rails sensor_monitoring with property
    // comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    // Invalid: test chassis devices rails sensor_monitoring with property
    // actions empty array.
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
TEST(ValidateRegulatorsConfigTest, RuleDuplicateRuleID)
{
    // Invalid: test duplicate ID in rule.
    {
        json configFile = validConfigFile;
        configFile["rules"][1]["id"] = "set_voltage_rule";
        configFile["rules"][1]["actions"][0]["pmbus_write_vout_command"]
                  ["format"] = "linear";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate rule ID.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleDuplicateChassisNumber)
{
    // Invalid: test duplicate number in chassis.
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["number"] = 1;
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate chassis number.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleDuplicateDeviceID)
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
TEST(ValidateRegulatorsConfigTest, RuleDuplicateRailID)
{
    // Invalid: test duplicate ID in rail.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][1]["id"] = "vdd";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate rail ID.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleDuplicateGlobalID)
{
    // Invalid: test duplicate ID in global.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][1]["id"] =
            "vdd_regulator";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate global ID.", "");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleInfinitLoops)
{
    // Invalid: test rule actions run_rule with infinite loops.
    {
        json configFile = validConfigFile;
        configFile["rules"][1]["actions"][0]["run_rule"] = "set_voltage_rule2";
        configFile["rules"][1]["id"] = "set_voltage_rule1";
        configFile["rules"][2]["actions"][0]["run_rule"] = "set_voltage_rule1";
        configFile["rules"][2]["id"] = "set_voltage_rule2";
        EXPECT_JSON_INVALID(configFile, "Error: Infinite loops.", "");
    }
}
