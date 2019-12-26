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
#define EXPECT_FILE_INVALID(tmpFile, expectedErrorMessage, expectedOutputMessage) \
        expectFileInvalid(tmpFile, expectedErrorMessage, expectedOutputMessage)

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

std::string runToolStderr(const std::string& config_file_name)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    std::string command = getValidationToolCommand(config_file_name);

    // only get the stderr from validation tool.
    command += " 2>&1 >/dev/null";

    char buffer[128];
    std::string result = "";
    // to get the stderr from the validation tool.
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    if (fgets(buffer, sizeof buffer, pipe) != NULL)
    {
        result = buffer;
    }
    pclose(pipe);
    return result;
}

std::string runToolStdout(const std::string& config_file_name)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    std::string command = getValidationToolCommand(config_file_name);

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
    pclose(pipe);
    return result;
}

int runToolExitCode(const std::string& config_file_name)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    std::string command = getValidationToolCommand(config_file_name);

    // to get the exit code for the validation tool.
    int exitCode = system(command.c_str());
    return exitCode;
}

void expectFileValid(const std::string& tmpFile)
{
    EXPECT_EQ(runToolExitCode(tmpFile), 0);
    EXPECT_EQ(runToolStderr(tmpFile), "");
    EXPECT_EQ(runToolStdout(tmpFile), "");
}

void expectFileInvalid(const std::string& tmpFile, \
                       const std::string& expectedErrorMessage, \
                       const std::string& expectedOutputMessage)
{
    EXPECT_NE(runToolExitCode(tmpFile), 0);
    EXPECT_EQ(runToolStderr(tmpFile), expectedErrorMessage);
    EXPECT_EQ(runToolStdout(tmpFile), expectedOutputMessage);
}

TEST(ValidateRegulatorsConfigTest, Rule)
{

    std::string fileName ;

    // valid test comments property, id property,
    // action property specified.
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        std::string jsonData = configFile.dump();
        // write json data to tmp file.
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_VALID(fileName);
        unlink(fileName.c_str());
    }

    // valid test rule with no comments
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data for rule without comments.
        configFile["rules"][0].erase("comments");

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_VALID(fileName);
        unlink(fileName.c_str());
    }

    //invalid test comments property has invalid value type
    {
        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data
        configFile["rules"][0]["comments"] = {1};

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_INVALID(fileName, \
                            "Schema validation failed\n", \
                            "1 is not of type u'string'\n");
        unlink(fileName.c_str());
    }

    // invalid test rule with no ID
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data for rule without ID.
        configFile["rules"][0].erase("id");

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_INVALID(fileName, \
                            "Schema validation failed\n", \
                            "u'id' is a required property\n");
        unlink(fileName.c_str());
    }

    // invalid test id property has invalid value type (not strings)
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data with id = true
        configFile["rules"][0]["id"] = true;

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_INVALID(fileName, \
                            "Schema validation failed\n", \
                            "True is not of type u'string'\n");
        unlink(fileName.c_str());
    }

    // invalid test id property has invalid value
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data with id = foo%
        configFile["rules"][0]["id"] = "foo%";

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_INVALID(fileName, \
                            "Schema validation failed\n", \
                            "u'foo%' does not match u'^[A-Za-z0-9_]+$'\n");
        unlink(fileName.c_str());
    }

    // invalid test rule with no action
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data for rule without actions
        configFile["rules"][0].erase("actions");

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_INVALID(fileName, \
                            "Schema validation failed\n", \
                            "u'actions' is a required property\n");
        unlink(fileName.c_str());
    }

    // valid test rule with multiple actions
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data with multiple actions
        configFile["rules"][0]["actions"][1]["run_rule"] = "set_page0_voltage_rule";

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_VALID(fileName);
        unlink(fileName.c_str());
    }

    // invalid test action property has invalid value type(not an array)
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data with action = 1
        configFile["rules"][0]["actions"] = 1;

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_INVALID(fileName, \
                            "Schema validation failed\n", \
                            "1 is not of type u'array'\n");
        unlink(fileName.c_str());
    }

    // invalid test action property has invalid value of action
    {

        fileName = createTmpFile();
        json configFile = validConfigFile;
        // create json data with action[0] = foo
        configFile["rules"][0]["actions"][0] = "foo";

        std::string jsonData = configFile.dump();
        std::ofstream out(fileName);
        out << jsonData;
        out.close();

        EXPECT_FILE_INVALID(fileName, \
                            "Schema validation failed\n", \
                            "u'foo' is not of type u'object'\n");
        unlink(fileName.c_str());
    }

}
