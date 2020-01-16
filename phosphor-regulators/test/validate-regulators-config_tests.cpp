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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <fstream>
#include <nlohmann/json.hpp>

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
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "1 is not of type u'string'");
    }

    // invalid test rule with no ID
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'id' is a required property");
    }

    // invalid test id property has invalid value type (not string)
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "True is not of type u'string'");
    }

    // invalid test id property has invalid value
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["id"] = "foo%";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'foo%' does not match u'^[A-Za-z0-9_]+$'");
    }

    // invalid test rule with no actions property
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("actions");
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
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
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "1 is not of type u'array'");
    }

    // invalid test actions property has invalid value of action
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0] = "foo";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'foo' is not of type u'object'");
    }

    // invalid test actions property has empty array
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "[] is too short");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleAnd)
{
    // test rule actions and valid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["and"][0]["i2c_compare_byte"]
                  ["register"] = "0xA0";
        configFile["rules"][0]["actions"][1]["and"][0]["i2c_compare_byte"]
                  ["value"] = "0x00";
        configFile["rules"][0]["actions"][1]["and"][1]["i2c_compare_byte"]
                  ["register"] = "0xA1";
        configFile["rules"][0]["actions"][1]["and"][1]["i2c_compare_byte"]
                  ["value"] = "0x00";
        EXPECT_JSON_VALID(configFile);
    }

    // test rule actions and with no property invalid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["and"][0] = json::array();
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "[] is not of type u'object'");
    }

    // test rule actions and with value not array invalid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["and"] = true;
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "True is not of type u'array'");
    }

    // test rule actions and with wrong action value invalid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["and"][0] = "foo";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'foo' is not of type u'object'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleComparePresence)
{
    // test rule actions compare_presence valid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] =
            "/system/chassis/motherboard/regulator2";
        configFile["rules"][0]["actions"][1]["compare_presence"]["value"] =
            true;
        EXPECT_JSON_VALID(configFile);
    }

    // test rule actions compare_presence with no FRU invalid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["value"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'fru' is a required property");
    }

    // test rule actions compare_presence with FRU string less than 1 invalid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] = "";
        configFile["rules"][0]["actions"][1]["compare_presence"]["value"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'' is too short");
    }

    // test rule actions compare_presence with no value invalid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] =
            "/system/chassis/motherboard/regulator2";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'value' is a required property");
    }

    // test rule actions compare_presence with value data not boolean invalid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] =
            "/system/chassis/motherboard/regulator2";
        configFile["rules"][0]["actions"][1]["compare_presence"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'1' is not of type u'boolean'");
    }

    // test rule actions compare_presence with FRU not string invalid.
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["compare_presence"]["fru"] = 1;
        configFile["rules"][0]["actions"][1]["compare_presence"]["value"] =
            true;
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
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
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'fru' is a required property");
    }

    // test rule actions compare_vpd with no keyword invalid
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"].erase("keyword");
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'keyword' is a required property");
    }

    // test rule actions compare_vpd with no value invalid
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"].erase("value");
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'value' is a required property");
    }

    // test rule actions compare_vpd with FRU not string invalid.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["fru"] = 1;
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "1 is not of type u'string'");
    }

    // test rule actions compare_vpd with FRU string less than 1 invalid.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'' is too short");
    }

    // test rule actions compare_vpd with keyword not "CCIN", "Manufacturer",
    // "Model", "PartNumber" invalid.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["keyword"] =
            "Number";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
                            "u'Number' is not one of [u'CCIN', "
                            "u'Manufacturer', u'Model', u'PartNumber']");
    }

    // test rule actions compare_vpd with value not string invalid.
    {
        json configFile = compareVpdFile;
        configFile["rules"][0]["actions"][1]["compare_vpd"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile, "Schema validation failed",
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
    //Valid: test rule actions if.
    {
        json configFile = ifFile;
        EXPECT_JSON_VALID(configFile);
    }
    //Valid: test rule actions if with required properties.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"].erase("else");
        EXPECT_JSON_VALID(configFile);
    }
    //Invalid: test rule actions if with no property condition.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"].erase("condition");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'condition' is a required property");
    }
    //Invalid: test rule actions if with no property then.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"].erase("then");
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "u'then' is a required property");
    }
    //Invalid: test rule actions if with property then empty array.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["then"]= json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    //Invalid: test rule actions if with property else empty array.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["else"]= json::array();
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "[] is too short");
    }
    //Invalid: test rule actions if with property condition wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["condition"]= 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'object'");
    }
    //Invalid: test rule actions if with property then wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["then"]= 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
    //Invalid: test rule actions if with property else wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][0]["actions"][1]["if"]["else"]= 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'array'");
    }
}
TEST(ValidateRegulatorsConfigTest, RuleNot)
{
    json notFile = validConfigFile;
    notFile["rules"][0]["actions"][1]["not"]["i2c_compare_byte"]
              ["register"] = "0xA0";
    notFile["rules"][0]["actions"][1]["not"]["i2c_compare_byte"]
              ["value"] = "0xFF";
    //Valid: test rule actions not.
    {
        json configFile = notFile;
        EXPECT_JSON_VALID(configFile);
    }
    //Invalid: test rule actions not with wrong type.
    {
        json configFile = notFile;
        configFile["rules"][0]["actions"][1]["not"]= 1;
        EXPECT_JSON_INVALID(configFile, "Validation failed.",
                            "1 is not of type u'object'");
    }
}
