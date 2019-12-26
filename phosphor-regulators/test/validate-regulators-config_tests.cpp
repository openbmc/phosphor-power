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
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#define EXPECT_FILE_VALID(tmpFile) expectFileValid(tmpFile)
#define EXPECT_FILE_INVALID(tmpFile, \
                            expectedErrorMessage, \
                            expectedOutputMessage) \
        expectFileInvalid(tmpFile, \
                          expectedErrorMessage, \
                          expectedOutputMessage)
#define EXPECT_JSON_VALID(configFileJson) expectJsonValid(configFileJson)
#define EXPECT_JSON_INVALID(configFileJson, \
                            expectedErrorMessage, \
                            expectedOutputMessage) \
        expectJsonInvalid(configFileJson, \
                          expectedErrorMessage, \
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
    char file_name[] = "/tmp/temp-XXXXXX";
    int fd = mkstemp(file_name);
    if (fd == -1)
    {
        perror("Can't create temporary file");
    }
    close(fd);
    return file_name;
}

std::string getValidationToolCommand(const std::string& config_file_name)
{
    std::string command = "python ../tools/validate-regulators-config.py -s \
                           ../schema/config_schema.json -c ";
    command += config_file_name;
    return command;
}

std::string runToolForOutout(const std::string& config_file_name, bool isReadingStderr = false)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    std::string command = getValidationToolCommand(config_file_name);
    // reading the stderr while isReadingStderr is true.
    if (isReadingStderr == true)
        command += " 2>&1 >/dev/null";

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
    int exitCode = pclose(pipe);
    return (exitCode, result);
}

void expectFileValid(const std::string& tmpFile)
{
    EXPECT_EQ(runToolForOutout(tmpFile, true), (0,""));
    EXPECT_EQ(runToolForOutout(tmpFile), (0,""));
}

void expectFileInvalid(const std::string& tmpFile, \
                       const std::string& expectedErrorMessage, \
                       const std::string& expectedOutputMessage)
{
    EXPECT_EQ(runToolForOutout(tmpFile, true), (256, expectedErrorMessage));
    EXPECT_EQ(runToolForOutout(tmpFile), (256, expectedOutputMessage));
}

void expectJsonValid(const json configFileJson)
{
    std::string fileName ;
    fileName = createTmpFile();
    std::string jsonData = configFileJson.dump();
    std::ofstream out(fileName);
    out << jsonData;
    out.close();

    EXPECT_FILE_VALID(fileName);
    unlink(fileName.c_str());
}

void expectJsonInvalid(const json configFileJson, \
                       const std::string& expectedErrorMessage, \
                       const std::string& expectedOutputMessage)
{
    std::string fileName ;
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

    std::string fileName ;

    // valid test comments property, id property,
    // action property specified.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }

    // valid test rule with no comments
    {
        json configFile = validConfigFile;
        // create json data for rule without comments.
        configFile["rules"][0].erase("comments");
        EXPECT_JSON_VALID(configFile);
    }

    //invalid test comments property has invalid value type
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["comments"] = {1};
        EXPECT_JSON_INVALID(configFile, \
                            "Schema validation failed\n", \
                            "1 is not of type u'string'\n");
    }

    // invalid test rule with no ID
    {
        json configFile = validConfigFile;
        // create json data for rule without ID.
        configFile["rules"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, \
                            "Schema validation failed\n", \
                            "u'id' is a required property\n");
    }

    // invalid test id property has invalid value type (not strings)
    {
        json configFile = validConfigFile;
        // create json data with id = true
        configFile["rules"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile, \
                            "Schema validation failed\n", \
                            "True is not of type u'string'\n");
    }

    // invalid test id property has invalid value
    {
        json configFile = validConfigFile;
        // create json data with id = foo%
        configFile["rules"][0]["id"] = "foo%";
        EXPECT_JSON_INVALID(configFile, \
                            "Schema validation failed\n", \
                            "u'foo%' does not match u'^[A-Za-z0-9_]+$'\n");
    }

    // invalid test rule with no action
    {
        json configFile = validConfigFile;
        // create json data for rule without actions
        configFile["rules"][0].erase("actions");
        EXPECT_JSON_INVALID(configFile, \
                            "Schema validation failed\n", \
                            "u'actions' is a required property\n");
    }

    // valid test rule with multiple actions
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data with multiple actions
        configFile["rules"][0]["actions"][1]["run_rule"] = \
            "set_page0_voltage_rule";
        EXPECT_JSON_VALID(configFile);
    }

    // invalid test action property has invalid value type(not an array)
    {
        json configFile = validConfigFile;
        // create json data with action = 1
        configFile["rules"][0]["actions"] = 1;
        EXPECT_JSON_INVALID(configFile, \
                            "Schema validation failed\n", \
                            "1 is not of type u'array'\n");
    }

    // invalid test action property has invalid value of action
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data with action[0] = foo
        configFile["rules"][0]["actions"][0] = "foo";
        EXPECT_JSON_INVALID(configFile, \
                            "Schema validation failed\n", \
                            "u'foo' is not of type u'object'\n");
    }

}