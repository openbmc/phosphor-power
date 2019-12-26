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

#include <fstream>
#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

#define EXPECT_FILE_VALID(tmpFile) expectFileValid(tmpFile)
#define EXPECT_FILE_INVALID(tmpFile, expectedErrorMessage,                     \
                            expectedOutputMessage)                             \
    expectFileInvalid(tmpFile, expectedErrorMessage, expectedOutputMessage)
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

std::string runToolForOutput(const std::string& configFileName,
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
    if (fgets(buffer, sizeof buffer, pipe) != NULL)
    {
        result = buffer;
    }
    int returnValue = pclose(pipe);
    // Check if pclose() failed
    if (returnValue == -1)
    {
        // unable to close pipe.  Print error and exit function.
        throw std::runtime_error("pclose() failed!");
    }

    // Get command exit status from return value
    int exitStatus = WEXITSTATUS(returnValue);
    return (exitStatus, result);
}

void expectFileValid(const std::string& tmpFile)
{
    EXPECT_EQ(runToolForOutput(tmpFile, true), (0, ""));
    EXPECT_EQ(runToolForOutput(tmpFile), (0, ""));
}

void expectFileInvalid(const std::string& tmpFile,
                       const std::string& expectedErrorMessage,
                       const std::string& expectedOutputMessage)
{
    EXPECT_EQ(runToolForOutput(tmpFile, true), (256, expectedErrorMessage));
    EXPECT_EQ(runToolForOutput(tmpFile), (256, expectedOutputMessage));
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
        EXPECT_JSON_INVALID(configFile, "Schema validation failed\n",
                            "1 is not of type u'string'\n");
    }

    // invalid test rule with no ID
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "Schema validation failed\n",
                            "u'id' is a required property\n");
    }

    // invalid test id property has invalid value type (not string)
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile, "Schema validation failed\n",
                            "True is not of type u'string'\n");
    }

    // invalid test id property has invalid value
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["id"] = "foo%";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed\n",
                            "u'foo%' does not match u'^[A-Za-z0-9_]+$'\n");
    }

    // invalid test rule with no actions property
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("actions");
        EXPECT_JSON_INVALID(configFile, "Schema validation failed\n",
                            "u'actions' is a required property\n");
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
        EXPECT_JSON_INVALID(configFile, "Schema validation failed\n",
                            "1 is not of type u'array'\n");
    }

    // invalid test actions property has invalid value of action
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0] = "foo";
        EXPECT_JSON_INVALID(configFile, "Schema validation failed\n",
                            "u'foo' is not of type u'object'\n");
    }

    // invalid test actions property has empty array
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "Schema validation failed\n",
                            "[] is too short\n");
    }
}