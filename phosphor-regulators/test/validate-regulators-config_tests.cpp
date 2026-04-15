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
#include "temporary_file.hpp"

#include <stdio.h>    // for popen(), pclose(), fgets()
#include <sys/stat.h> // for chmod()
#include <sys/wait.h> // for WEXITSTATUS

#include <nlohmann/json.hpp>

#include <cstdio>
#include <fstream>

#include <gtest/gtest.h>

#define EXPECT_FILE_VALID(configFile) expectFileValid(configFile)
#define EXPECT_FILE_INVALID(configFile, expectedOutput)                        \
    expectFileInvalid(configFile, expectedOutput)
#define EXPECT_JSON_VALID(configFileJson) expectJsonValid(configFileJson)
#define EXPECT_JSON_INVALID(configFileJson, expectedOutput)                    \
    expectJsonInvalid(configFileJson, expectedOutput)

using json = nlohmann::json;
using TemporaryFile = phosphor::power::util::TemporaryFile;

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
        },
        {
          "comments": [ "Detects presence of regulators associated with CPU3" ],
          "id": "detect_presence_rule",
          "actions": [
            {
              "compare_presence": {
                "fru": "system/chassis/motherboard/cpu3",
                "value": true
              }
            }
          ]
        },
        {
          "comments": [ "Detects and logs redundant phase faults" ],
          "id": "detect_phase_faults_rule",
          "actions": [
            {
              "if": {
                "condition": {
                  "i2c_compare_bit": { "register": "0x02", "position": 3, "value": 1 }
                },
                "then": [
                  { "log_phase_fault": { "type": "n" } }
                ]
              }
            }
          ]
        }
      ],

      "chassis_templates": [
        {
          "comments": ["Template for a chassis in a multi-chassis system"],
          "id": "multi_chassis_template",
          "number": "${chassis_number}",
          "inventory_path": "system/chassis${chassis_number}",
          "status_monitoring": {
            "is_present_monitored": "${is_present}",
            "is_available_monitored": "${is_available}",
            "is_enabled_monitored": "${is_enabled}"
          },
          "devices": [
            {
              "id": "chassis${chassis_number}_micro",
              "is_regulator": "${is_regulator}",
              "fru": "system/chassis/motherboard",
              "i2c_interface": { "bus": "${bus}", "address": "0x70"
              }
            },
            {
              "id": "chassis${chassis_number}_regulator_${Reg_Type_2}",
              "is_regulator": true,
              "fru": "system/chassis${chassis_number}/motherboard/regulator${reg_num}",
              "i2c_interface": { "bus": "${bus}", "address": "${address}"
              },
              "presence_detection": {
                "actions": [
                  {
                    "or": [
                      { "compare_presence": { "fru": "system/chassis/motherboard/cpu1", "value": "${presence_value}" } },
                      { "compare_vpd": { "fru": "system/chassis/motherboard", "keyword": "${vpd_keyword}", "byte_values": ["0x32", "${vpd_byte_val}"] } },
                      { "compare_vpd": { "fru": "system/chassis/disk_backplane", "keyword": "CCIN", "value": "${vpd_val}" } },
                      { "run_rule": "${rule_name}" }
                    ]
                  }
                ]
              },
              "configuration": {
                "actions": [
                  {
                    "pmbus_write_vout_command": { "volts": "${volts}",
                      "format": "${vout_format}",
                      "exponent": "${exponent}",
                      "is_verified": "${is_verified}"
                    }
                  },
                  { "i2c_write_bit": { "register": "0xA0", "position": "${bit_pos}", "value": 0 } },
                  { "i2c_write_byte": { "register": "${reg}", "value": "0xCC" } },
                  { "i2c_write_bytes": { "register": "0xA0", "values": ["0xFF", "${byte_val}"] } }
                ]
              },
              "phase_fault_detection": {
                "actions": [
                  { "set_device": "chassis${chassis_number}_micro" },
                  {
                    "if": {
                      "condition": {
                        "and": [
                          { "i2c_compare_bit": { "register": "0x02", "position": "${bit_pos}", "value": "${bit_val}" } },
                          { "i2c_compare_byte": { "register": "${reg}", "value": "0x00" } },
                          { "not":
                            { "i2c_compare_bytes": { "register": "0x82", "values": ["0x02", "0x73"], "masks": ["0x7F", "${byte_mask}"] } }
                          }
                        ]
                      },
                      "then": [
                        {
                          "i2c_capture_bytes": { "register": "0x04", "count": "${cap_count}" } },
                        { "log_phase_fault": { "type": "${phase_fault_type}" } }
                      ]
                    }
                  }
                ]
              },
              "rails": [
                {
                  "id": "chassis${chassis_number}_regulator_rail",
                  "sensor_monitoring": {
                    "actions": [
                      {
                        "pmbus_read_sensor": {
                          "type": "${sensor_type}",
                          "command": "${sensor_command}",
                          "format": "${sensor_format}",
                          "exponent": "${exponent}"
                        }
                      }
                    ]
                  }
                }
              ]
            }
          ]
        }
      ],

      "chassis": [
        {
          "comments": [ "Chassis number 1 containing CPUs and memory" ],
          "number": 1,
          "inventory_path": "system/chassis",
          "status_monitoring": {
            "is_present_monitored": true
          },
          "devices": [
            {
              "comments": [ "IR35221 regulator producing the Vdd rail" ],
              "id": "vdd_regulator",
              "is_regulator": true,
              "fru": "system/chassis/motherboard/regulator1",
              "i2c_interface": { "bus": 1, "address": "0x70" },
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
        },
        {
          "comments": [ "Chassis number 2 using a template" ],
          "template_id": "multi_chassis_template",
          "template_variable_values": {
            "address": "0x71",
            "bit_pos": "1",
            "bit_val": "0",
            "bus": "3",
            "byte_mask": "0x7F",
            "byte_val": "0x23",
            "cap_count": "2",
            "chassis_number": "2",
            "exponent": "-8",
            "is_available": "true",
            "is_enabled": "false",
            "is_present": "true",
            "is_regulator": "false",
            "is_verified": "true",
            "phase_fault_type": "n+1",
            "presence_value": "true",
            "reg": "0x04",
            "reg_num": "2",
            "Reg_Type_2": "vdd",
            "rule_name": "detect_presence_rule",
            "sensor_command": "0x8C",
            "sensor_format": "linear_11",
            "sensor_type": "iout",
            "volts": "1.03",
            "vout_format": "linear",
            "vpd_byte_val": "0x33",
            "vpd_keyword": "Model",
            "vpd_val": "FooBar"
          }
        }
      ]
    }
)"_json;

std::string getValidationToolCommand(const std::string& configFileName)
{
    std::string command =
        "../phosphor-regulators/tools/validate-regulators-config.py -s \
         ../phosphor-regulators/schema/config_schema.json -c ";
    command += configFileName;
    return command;
}

int runToolForOutputWithCommand(std::string command, std::string& output)
{
    // Run the command and merge stderr with stdout
    command += " 2>&1";

    // Execute the command and capture output
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }

    // Read all lines of output
    char buffer[256];
    output = "";
    while (fgets(buffer, sizeof buffer, pipe) != nullptr)
    {
        output += buffer;
    }

    // Close pipe and get exit status
    int returnValue = pclose(pipe);
    if (returnValue == -1)
    {
        throw std::runtime_error("pclose() failed!");
    }

    // Return the command exit status
    return WEXITSTATUS(returnValue);
}

int runToolForOutput(const std::string& configFileName, std::string& output)
{
    std::string command = getValidationToolCommand(configFileName);
    return runToolForOutputWithCommand(command, output);
}

void expectFileValid(const std::string& configFileName)
{
    std::string output;
    EXPECT_EQ(runToolForOutput(configFileName, output), 0)
        << "output: " << output;
    EXPECT_EQ(output, "") << "output: " << output;
}

void expectFileInvalid(const std::string& configFileName,
                       const std::string& expectedOutput)
{
    std::string output;
    EXPECT_EQ(runToolForOutput(configFileName, output), 1)
        << "output: " << output;
    if (expectedOutput != "")
    {
        EXPECT_TRUE(output.contains(expectedOutput))
            << "output: " << output << "\nexpectedOutput: " << expectedOutput;
    }
}

void writeDataToFile(const json configFileJson, std::string fileName)
{
    std::string jsonData = configFileJson.dump();
    std::ofstream out(fileName);
    out << jsonData;
    out.close();
}

void expectJsonValid(const json configFileJson)
{
    TemporaryFile tmpFile;
    std::string fileName = tmpFile.getPath().string();
    writeDataToFile(configFileJson, fileName);

    EXPECT_FILE_VALID(fileName);
}

void expectJsonInvalid(const json configFileJson,
                       const std::string& expectedOutput)
{
    TemporaryFile tmpFile;
    std::string fileName = tmpFile.getPath().string();
    writeDataToFile(configFileJson, fileName);

    EXPECT_FILE_INVALID(fileName, expectedOutput);
}

void expectCommandLineSyntax(const std::string& command,
                             const std::string& expectedOutput, int status)
{
    std::string output;
    EXPECT_EQ(runToolForOutputWithCommand(command, output), status);
    if (expectedOutput != "")
    {
        EXPECT_TRUE(output.contains(expectedOutput))
            << "output: " << output << "\nexpectedOutput: " << expectedOutput;
    }
}

TEST(ValidateRegulatorsConfigTest, Action)
{
    // Valid: Comments property not specified
    // Also tests all supported action types
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: Comments property specified
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0]["comments"][0] =
            "Set VOUT_COMMAND";
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: Wrong data type for comments (should be array of string)
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: Wrong data type for action type (such as "i2c_write_byte": true)
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: Empty comments array
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0]["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: Comments array has wrong element type (should be string)
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0]["comments"][0] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'string'");
    }
    // Invalid: No action type specified
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["comments"][0] =
            "Check if bit 3 is on";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: Multiple action types specified (such as both 'compare_presence'
    // and 'pmbus_write_vout_command')
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0]["compare_presence"]["value"] =
            true;
        EXPECT_JSON_INVALID(configFile, "is valid under each of");
    }
    // Invalid: Unexpected property specified (like 'foo')
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][1]["foo"] = "foo";
        EXPECT_JSON_INVALID(
            configFile,
            "Additional properties are not allowed ('foo' was unexpected)");
    }
}

TEST(ValidateRegulatorsConfigTest, And)
{
    // Valid (validConfigFile contains an "and" action).
    {
        json configFile = validConfigFile;
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
        EXPECT_JSON_INVALID(configFile, "[] is too short");
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
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
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
        EXPECT_JSON_INVALID(configFile, "'foo' is not of type 'object'");
    }
}

TEST(ValidateRegulatorsConfigTest, Chassis)
{
    // Valid: test chassis with all non-template properties.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis with only required non-template properties.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0].erase("comments");
        configFile["chassis"][0].erase("status_monitoring");
        configFile["chassis"][0].erase("devices");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis with no number.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0].erase("number");
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis with no inventory_path.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0].erase("inventory_path");
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis with property comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test chassis with property number wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["number"] = 1.3;
        EXPECT_JSON_INVALID(configFile,
                            "1.3 is not valid under any of the given schemas");
    }
    // Invalid: test chassis with property inventory_path wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["inventory_path"] = 2;
        EXPECT_JSON_INVALID(configFile, "2 is not of type 'string'");
    }
    // Invalid: test chassis with property status_monitoring wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["status_monitoring"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test chassis with property devices wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test chassis with property comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test chassis with property devices empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test chassis with property number less than 1.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["number"] = 0;
        EXPECT_JSON_INVALID(configFile, "0 is less than the minimum of 1");
    }
    // Invalid: test chassis with property inventory_path empty string.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["inventory_path"] = "";
        EXPECT_JSON_INVALID(configFile, "'' is too short");
    }
    // Invalid: test chassis has property containing a variable
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["inventory_path"] =
            "system/chassis${chassis_num}";
        EXPECT_JSON_INVALID(configFile,
                            "Error: Chassis contains template variable.");
    }
    // Valid: test chassis using template
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis using template with no template_id
    {
        json configFile = validConfigFile;
        configFile["chassis"][1].erase("template_id");
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis using template with no template_variable_values
    {
        json configFile = validConfigFile;
        configFile["chassis"][1].erase("template_variable_values");
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis with both traditional and template properties
    // (has number and template_id)
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["template_id"] = "multi_chassis_template";
        configFile["chassis"][0]["template_variable_values"]["chassis_number"] =
            "1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis with neither mode complete (missing number from
    // traditional, missing template_variable_values from template)
    {
        json configFile = validConfigFile;
        configFile["chassis"][0].erase("number");
        configFile["chassis"][0]["template_id"] = "multi_chassis_template";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis using template with property template_id wrong
    // type
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis using template with property
    // template_variable_values wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_variable_values"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test chassis using invalid template_id
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_id"] = "does_not_exist";
        EXPECT_JSON_INVALID(configFile,
                            "Error: Chassis template ID does not exist");
    }
    // Invalid: test chassis using template but missing one of the variables
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_variable_values"].erase("exponent");
        EXPECT_JSON_INVALID(
            configFile,
            "Error: No value specified for the following template variables: exponent");
    }
}

TEST(ValidateRegulatorsConfigTest, ChassisTemplate)
{
    // Valid: test chassis_template with all properties
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis_template with only required properties
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0].erase("comments");
        configFile["chassis_templates"][0].erase("status_monitoring");
        configFile["chassis_templates"][0].erase("devices");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis_template with no id property
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "'id' is a required property");
    }
    // Invalid: test chassis_template with no number property
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0].erase("number");
        EXPECT_JSON_INVALID(configFile, "'number' is a required property");
    }
    // Invalid: test chassis_template with no inventory_path property
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0].erase("inventory_path");
        EXPECT_JSON_INVALID(configFile,
                            "'inventory_path' is a required property");
    }
    // Invalid: test chassis_template with property comments wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test chassis_template with property id wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis_template with property number wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test chassis_template with property inventory_path wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["inventory_path"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'string'");
    }
    // Invalid: test chassis_template with property status_monitoring wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["status_monitoring"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test chassis_template with property devices wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test chassis_template with property comments empty array
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test chassis_template with property devices empty array
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test chassis_template with property number less than 1
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = 0;
        EXPECT_JSON_INVALID(configFile, "0 is less than the minimum of 1");
    }
    // Invalid: test chassis_template with property inventory_path empty string
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["inventory_path"] = "";
        EXPECT_JSON_INVALID(configFile, "'' is too short");
    }
    // Invalid: test chassis_template with invalid property
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["foo"] = true;
        EXPECT_JSON_INVALID(
            configFile,
            "Additional properties are not allowed ('foo' was unexpected)");
    }
    // Invalid: test chassis_template with duplicate id
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][1]["id"] = "multi_chassis_template";
        configFile["chassis_templates"][1]["number"] = 2;
        configFile["chassis_templates"][1]["inventory_path"] = "foo";
        EXPECT_JSON_INVALID(configFile,
                            "Error: Duplicate chassis_template ID.");
    }
}

TEST(ValidateRegulatorsConfigTest, ChassisTemplates)
{
    // Valid: test chassis_templates with one template
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test chassis_templates with multiple templates
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][1]["id"] = "chassis_template_2";
        configFile["chassis_templates"][1]["number"] = "${chassis_number}";
        configFile["chassis_templates"][1]["inventory_path"] =
            "system/chassis${chassis_number}";
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test chassis_templates with empty array
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test chassis_templates wrong type (not array)
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
}

TEST(ValidateRegulatorsConfigTest, ComparePresence)
{
    // Valid.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }

    // Invalid: no FRU property.
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["compare_presence"].erase("fru");
        EXPECT_JSON_INVALID(configFile, "'fru' is a required property");
    }

    // Invalid: FRU property length is string less than 1.
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["compare_presence"]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "'' is too short");
    }

    // Invalid: no value property.
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["compare_presence"].erase("value");
        EXPECT_JSON_INVALID(configFile, "'value' is a required property");
    }

    // Invalid: value property type is not boolean.
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["compare_presence"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "'1' does not match '.*");
    }

    // Invalid: FRU property type is not string.
    {
        json configFile = validConfigFile;
        configFile["rules"][2]["actions"][0]["compare_presence"]["fru"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'string'");
    }

    // Valid: value property using string with variable
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
}

TEST(ValidateRegulatorsConfigTest, CompareVpd)
{
    // Valid: value property: not empty.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }

    // Valid: value property: empty.
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][2]["compare_vpd"]["value"] = "";
        EXPECT_JSON_VALID(configFile);
    }

    // Valid: byte_values property: not empty.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }

    // Valid: byte_values property: empty.
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][1]["compare_vpd"]["byte_values"] =
                      json::array();
        EXPECT_JSON_VALID(configFile);
    }

    // Invalid: no FRU property.
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][1]["compare_vpd"]
                      .erase("fru");
        EXPECT_JSON_INVALID(configFile, "'fru' is a required property");
    }

    // Invalid: no keyword property.
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][1]["compare_vpd"]
                      .erase("keyword");
        EXPECT_JSON_INVALID(configFile, "'keyword' is a required property");
    }

    // Invalid: no value property.
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][2]["compare_vpd"]
                      .erase("value");
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: property FRU wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][2]["compare_vpd"]["fru"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'string'");
    }

    // Invalid: property FRU is string less than 1.
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][2]["compare_vpd"]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "'' is too short");
    }

    // Invalid: property keyword is not "CCIN", "Manufacturer", "Model",
    // "PartNumber", "HW"
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][2]["compare_vpd"]["keyword"] = "Number";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: property value wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][2]["compare_vpd"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'string'");
    }

    // Invalid: property byte_values has wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][1]["compare_vpd"]["byte_values"] =
                      "0x50";
        EXPECT_JSON_INVALID(configFile, "'0x50' is not of type 'array'");
    }

    // Invalid: properties byte_values and value both exist
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["presence_detection"]
                  ["actions"][0]["or"][2]["compare_vpd"]["byte_values"] = {
                      "0x01", "0x02"};
        EXPECT_JSON_INVALID(configFile, "is valid under each of");
    }

    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
}

TEST(ValidateRegulatorsConfigTest, ConfigFile)
{
    // Valid: Only required properties specified
    {
        json configFile;
        configFile["chassis"][0]["number"] = 1;
        configFile["chassis"][0]["inventory_path"] = "system/chassis";
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: All properties specified
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: Multiple chassis templates and multiple chassis
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][1]["id"] = "multi_chassis_template2";
        configFile["chassis_templates"][1]["number"] = "${chassis_number}";
        configFile["chassis_templates"][1]["inventory_path"] =
            "system/chassis${chassis_number}";
        configFile["chassis"][2]["template_id"] = "multi_chassis_template2";
        configFile["chassis"][2]["template_variable_values"]["chassis_number"] =
            "3";
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: Required chassis property not specified
    {
        json configFile = validConfigFile;
        configFile.erase("chassis");
        EXPECT_JSON_INVALID(configFile, "'chassis' is a required property");
    }
    // Invalid: Wrong data type for comments
    {
        json configFile = validConfigFile;
        configFile["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: Wrong data type for rules
    {
        json configFile = validConfigFile;
        configFile["rules"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: Wrong data type for chassis_templates
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: Wrong data type for chassis
    {
        json configFile = validConfigFile;
        configFile["chassis"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: Empty comments array;
    {
        json configFile = validConfigFile;
        configFile["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: Empty rules array
    {
        json configFile = validConfigFile;
        configFile["rules"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: Empty chassis_templates array
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: Empty chassis array
    {
        json configFile = validConfigFile;
        configFile["chassis"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: Comments array has wrong element type (should be string)
    {
        json configFile = validConfigFile;
        configFile["comments"][0] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'string'");
    }
    // Invalid: Rules array has wrong element type (should be rule)
    {
        json configFile = validConfigFile;
        configFile["rules"][0] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: Chassis templates array has wrong element type (should be
    // chassis_template)
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: Chassis array has wrong element type (should be chassis)
    {
        json configFile = validConfigFile;
        configFile["chassis"][0] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: Unexpected property specified
    {
        json configFile = validConfigFile;
        configFile["foo"] = json::array();
        EXPECT_JSON_INVALID(
            configFile,
            "Additional properties are not allowed ('foo' was unexpected)");
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
        configFile["chassis"][0]["devices"][0]["configuration"]["comments"][1] =
            "test multiple array elements in comments.";
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test configuration with property actions and with no rule_id.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"][0]
                  ["compare_presence"]["fru"] =
                      "system/chassis/motherboard/cpu3";
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
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }
    // Invalid: test configuration with both actions and rule_id properties.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"][0]
                  ["compare_presence"]["fru"] =
                      "system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"][0]
                  ["compare_presence"]["value"] = true;
        EXPECT_JSON_INVALID(configFile, "is valid under each of");
    }
    // Invalid: test configuration with no rule_id and actions.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test configuration with property volts wrong type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["volts"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test configuration with property rule_id wrong type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
            true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test configuration with property actions wrong type.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"] =
            true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test configuration with property comments empty array.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["comments"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test configuration with property rule_id wrong format.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"]["rule_id"] =
            "id!";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test configuration with property actions empty array.
    {
        json configFile = configurationFile;
        configFile["chassis"][0]["devices"][0]["configuration"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["configuration"]["actions"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
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
        configFile["chassis"][0]["devices"][0].erase("phase_fault_detection");
        configFile["chassis"][0]["devices"][0].erase("rails");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test devices with no id.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "'id' is a required property");
    }
    // Invalid: test devices with no is_regulator.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("is_regulator");
        EXPECT_JSON_INVALID(configFile, "should not be valid under");
    }
    // Invalid: test devices with no fru.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("fru");
        EXPECT_JSON_INVALID(configFile, "'fru' is a required property");
    }
    // Invalid: test devices with no i2c_interface.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0].erase("i2c_interface");
        EXPECT_JSON_INVALID(configFile,
                            "'i2c_interface' is a required property");
    }
    // Invalid: is_regulator=false: phase_fault_detection specified
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["is_regulator"] = false;
        configFile["chassis"][0]["devices"][0].erase("rails");
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["rule_id"] = "detect_phase_faults_rule";
        EXPECT_JSON_INVALID(configFile, "should not be valid under");
    }
    // Invalid: is_regulator=false: rails specified
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["is_regulator"] = false;
        EXPECT_JSON_INVALID(configFile, "should not be valid under");
    }
    // Invalid: is_regulator=false: phase_fault_detection and rails specified
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["is_regulator"] = false;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["rule_id"] = "detect_phase_faults_rule";
        EXPECT_JSON_INVALID(configFile, "should not be valid under");
    }
    // Invalid: test devices with property comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test devices with property id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test devices with property is_regulator wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["is_regulator"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test devices with property fru wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["fru"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'string'");
    }
    // Invalid: test devices with property i2c_interface wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test devices with property presence_detection wrong
    // type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test devices with property configuration wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["configuration"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test devices with property phase_fault_detection wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test devices with property rails wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test devices with property comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test devices with property fru length less than 1.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["fru"] = "";
        EXPECT_JSON_INVALID(configFile, "'' is too short");
    }
    // Invalid: test devices with property id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["id"] = "id#";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test devices with property rails empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Valid: test where some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
}

TEST(ValidateRegulatorsConfigTest, I2CCaptureBytes)
{
    json initialFile = validConfigFile;
    initialFile["rules"][0]["actions"][1]["i2c_capture_bytes"]["register"] =
        "0xA0";
    initialFile["rules"][0]["actions"][1]["i2c_capture_bytes"]["count"] = 2;

    // Valid: All required properties
    {
        json configFile = initialFile;
        EXPECT_JSON_VALID(configFile);
    }

    // Invalid: register not specified
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["i2c_capture_bytes"].erase(
            "register");
        EXPECT_JSON_INVALID(configFile, "'register' is a required property");
    }

    // Invalid: count not specified
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["i2c_capture_bytes"].erase(
            "count");
        EXPECT_JSON_INVALID(configFile, "'count' is a required property");
    }

    // Invalid: invalid property specified
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["i2c_capture_bytes"]["foo"] = true;
        EXPECT_JSON_INVALID(
            configFile,
            "Additional properties are not allowed ('foo' was unexpected)");
    }

    // Invalid: register has wrong data type
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["i2c_capture_bytes"]["register"] =
            1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: register has wrong format
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["i2c_capture_bytes"]["register"] =
            "0xA00";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: count has wrong data type
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["i2c_capture_bytes"]["count"] =
            3.1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: count < 1
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["i2c_capture_bytes"]["count"] = 0;
        EXPECT_JSON_INVALID(configFile, "0 is less than the minimum of 1");
    }

    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
        EXPECT_JSON_INVALID(configFile, "'register' is a required property");
    }
    // Invalid: test i2c_compare_bit with no position.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"].erase(
            "position");
        EXPECT_JSON_INVALID(configFile, "'position' is a required property");
    }
    // Invalid: test i2c_compare_bit with no value.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"].erase("value");
        EXPECT_JSON_INVALID(configFile, "'value' is a required property");
    }
    // Invalid: test i2c_compare_bit with register wrong type.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bit with register wrong format.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["register"] =
            "0xA00";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bit with position wrong type.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] =
            3.1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bit with position greater than 7.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] = 8;
        EXPECT_JSON_INVALID(configFile, "8 is greater than the maximum of 7");
    }
    // Invalid: test i2c_compare_bit with position less than 0.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["position"] =
            -1;
        EXPECT_JSON_INVALID(configFile, "-1 is less than the minimum of 0");
    }
    // Invalid: test i2c_compare_bit with value wrong type.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "does not match '.*");
    }
    // Invalid: test i2c_compare_bit with value greater than 1.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = 2;
        EXPECT_JSON_INVALID(configFile, "2 is greater than the maximum of 1");
    }
    // Invalid: test i2c_compare_bit with value less than 0.
    {
        json configFile = i2cCompareBitFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bit"]["value"] = -1;
        EXPECT_JSON_INVALID(configFile, "-1 is less than the minimum of 0");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
        EXPECT_JSON_INVALID(configFile, "'register' is a required property");
    }
    // Invalid: test i2c_compare_byte with no value.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"].erase("value");
        EXPECT_JSON_INVALID(configFile, "'value' is a required property");
    }
    // Invalid: test i2c_compare_byte with property register wrong type.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property value wrong type.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property mask wrong type.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property register more than 2 hex
    // digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property value more than 2 hex
    // digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property mask more than 2 hex digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property register less than 2 hex
    // digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property value less than 2 hex
    // digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property mask less than 2 hex digits.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property register no leading prefix.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property value no leading prefix.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property mask no leading prefix.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] = "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property register invalid hex digit.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["register"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property value invalid hex digit.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["value"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_byte with property mask invalid hex digit.
    {
        json configFile = i2cCompareByteFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_byte"]["mask"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
        EXPECT_JSON_INVALID(configFile, "'register' is a required property");
    }
    // Invalid: test i2c_compare_bytes with no values.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"].erase(
            "values");
        EXPECT_JSON_INVALID(configFile, "'values' is a required property");
    }
    // Invalid: test i2c_compare_bytes with property values as empty array.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test i2c_compare_bytes with property masks as empty array.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test i2c_compare_bytes with property register wrong type.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property values wrong type.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }
    // Invalid: test i2c_compare_bytes with property masks wrong type.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }
    // Invalid: test i2c_compare_bytes with property register more than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property values more than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property masks more than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property register less than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property values less than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property masks less than 2 hex
    // digits.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property register no leading prefix.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property values no leading prefix.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property masks no leading prefix.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property register invalid hex digit.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["register"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property values invalid hex digit.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["values"][0] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_compare_bytes with property masks invalid hex digit.
    {
        json configFile = i2cCompareBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_compare_bytes"]["masks"][0] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
}

TEST(ValidateRegulatorsConfigTest, I2CInterface)
{
    // Valid: test i2c_interface.
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test i2c_interface with no bus.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"].erase("bus");
        EXPECT_JSON_INVALID(configFile, "'bus' is a required property");
    }
    // Invalid: test i2c_interface with no address.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"].erase(
            "address");
        EXPECT_JSON_INVALID(configFile, "'address' is a required property");
    }
    // Invalid: test i2c_interface with property bus wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["bus"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_interface with property address wrong
    // type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["address"] =
            true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_interface with property bus less than
    // 0.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["bus"] = -1;
        EXPECT_JSON_INVALID(configFile, "-1 is less than the minimum of 0");
    }
    // Invalid: test i2c_interface with property address wrong
    // format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["i2c_interface"]["address"] =
            "0x700";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
        EXPECT_JSON_INVALID(configFile, "'register' is a required property");
    }
    // Invalid: test i2c_write_bit with no position.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"].erase("position");
        EXPECT_JSON_INVALID(configFile, "'position' is a required property");
    }
    // Invalid: test i2c_write_bit with no value.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"].erase("value");
        EXPECT_JSON_INVALID(configFile, "'value' is a required property");
    }
    // Invalid: test i2c_write_bit with register wrong type.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bit with register wrong format.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["register"] =
            "0xA00";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bit with position wrong type.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["position"] = 3.1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bit with position greater than 7.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["position"] = 8;
        EXPECT_JSON_INVALID(configFile, "8 is greater than the maximum of 7");
    }
    // Invalid: test i2c_write_bit with position less than 0.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["position"] = -1;
        EXPECT_JSON_INVALID(configFile, "-1 is less than the minimum of 0");
    }
    // Invalid: test i2c_write_bit with value wrong type.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["value"] = "1";
        EXPECT_JSON_INVALID(configFile, "does not match '.*");
    }
    // Invalid: test i2c_write_bit with value greater than 1.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["value"] = 2;
        EXPECT_JSON_INVALID(configFile, "2 is greater than the maximum of 1");
    }
    // Invalid: test i2c_write_bit with value less than 0.
    {
        json configFile = i2cWriteBitFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bit"]["value"] = -1;
        EXPECT_JSON_INVALID(configFile, "-1 is less than the minimum of 0");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
        EXPECT_JSON_INVALID(configFile, "'register' is a required property");
    }
    // Invalid: test i2c_write_byte with no value.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"].erase("value");
        EXPECT_JSON_INVALID(configFile, "'value' is a required property");
    }
    // Invalid: test i2c_write_byte with property register wrong type.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property value wrong type.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property mask wrong type.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property register more than 2 hex
    // digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property value more than 2 hex
    // digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property mask more than 2 hex digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property register less than 2 hex
    // digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property value less than 2 hex
    // digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] = "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property mask less than 2 hex digits.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] = "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property register no leading prefix.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property value no leading prefix.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] = "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property mask no leading prefix.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] = "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property register invalid hex digit.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["register"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property value invalid hex digit.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["value"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_byte with property mask invalid hex digit.
    {
        json configFile = i2cWriteByteFile;
        configFile["rules"][0]["actions"][1]["i2c_write_byte"]["mask"] = "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
        EXPECT_JSON_INVALID(configFile, "'register' is a required property");
    }
    // Invalid: test i2c_write_bytes with no values.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"].erase("values");
        EXPECT_JSON_INVALID(configFile, "'values' is a required property");
    }
    // Invalid: test i2c_write_bytes with property values as empty array.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test i2c_write_bytes with property masks as empty array.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test i2c_write_bytes with property register wrong type.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property values wrong type.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }
    // Invalid: test i2c_write_bytes with property masks wrong type.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }
    // Invalid: test i2c_write_bytes with property register more than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property values more than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"][0] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property masks more than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"][0] =
            "0x820";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property register less than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property values less than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"][0] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property masks less than 2 hex
    // digits.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"][0] =
            "0x8";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property register no leading prefix.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property values no leading prefix.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"][0] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property masks no leading prefix.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"][0] =
            "82";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property register invalid hex digit.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["register"] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property values invalid hex digit.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["values"][0] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test i2c_write_bytes with property masks invalid hex digit.
    {
        json configFile = i2cWriteBytesFile;
        configFile["rules"][0]["actions"][1]["i2c_write_bytes"]["masks"][0] =
            "0xG1";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
}

TEST(ValidateRegulatorsConfigTest, If)
{
    json ifFile = validConfigFile;
    ifFile["rules"][4]["actions"][0]["if"]["condition"]["run_rule"] =
        "set_voltage_rule";
    ifFile["rules"][4]["actions"][0]["if"]["then"][0]["run_rule"] =
        "read_sensors_rule";
    ifFile["rules"][4]["actions"][0]["if"]["else"][0]["run_rule"] =
        "read_sensors_rule";
    ifFile["rules"][4]["id"] = "rule_if";
    // Valid: test if.
    {
        json configFile = ifFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test if with required properties.
    {
        json configFile = ifFile;
        configFile["rules"][4]["actions"][0]["if"].erase("else");
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test if with no property condition.
    {
        json configFile = ifFile;
        configFile["rules"][4]["actions"][0]["if"].erase("condition");
        EXPECT_JSON_INVALID(configFile, "'condition' is a required property");
    }
    // Invalid: test if with no property then.
    {
        json configFile = ifFile;
        configFile["rules"][4]["actions"][0]["if"].erase("then");
        EXPECT_JSON_INVALID(configFile, "'then' is a required property");
    }
    // Invalid: test if with property then empty array.
    {
        json configFile = ifFile;
        configFile["rules"][4]["actions"][0]["if"]["then"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test if with property else empty array.
    {
        json configFile = ifFile;
        configFile["rules"][4]["actions"][0]["if"]["else"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test if with property condition wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][4]["actions"][0]["if"]["condition"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'object'");
    }
    // Invalid: test if with property then wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][4]["actions"][0]["if"]["then"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }
    // Invalid: test if with property else wrong type.
    {
        json configFile = ifFile;
        configFile["rules"][4]["actions"][0]["if"]["else"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }
}

TEST(ValidateRegulatorsConfigTest, LogPhaseFault)
{
    json initialFile = validConfigFile;
    initialFile["rules"][0]["actions"][1]["log_phase_fault"]["type"] = "n";

    // Valid: All required properties
    {
        json configFile = initialFile;
        EXPECT_JSON_VALID(configFile);
    }

    // Invalid: type not specified
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["log_phase_fault"].erase("type");
        EXPECT_JSON_INVALID(configFile, "'type' is a required property");
    }

    // Invalid: invalid property specified
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["log_phase_fault"]["foo"] = true;
        EXPECT_JSON_INVALID(
            configFile,
            "Additional properties are not allowed ('foo' was unexpected)");
    }

    // Invalid: type has wrong data type
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["log_phase_fault"]["type"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: type has invalid value
    {
        json configFile = initialFile;
        configFile["rules"][0]["actions"][1]["log_phase_fault"]["type"] = "n+2";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'object'");
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
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test or with wrong type.
    {
        json configFile = orFile;
        configFile["rules"][0]["actions"][1]["or"] = 1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }
}

TEST(ValidateRegulatorsConfigTest, PhaseFaultDetection)
{
    json initialFile = validConfigFile;
    initialFile["chassis"][0]["devices"][0]["phase_fault_detection"]
               ["rule_id"] = "detect_phase_faults_rule";

    // Valid: comments specified
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["comments"][0] = "Detect phase faults";
        EXPECT_JSON_VALID(configFile);
    }

    // Valid: device_id specified
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["device_id"] = "vdd_regulator";
        EXPECT_JSON_VALID(configFile);
    }

    // Valid: rule_id specified
    {
        json configFile = initialFile;
        EXPECT_JSON_VALID(configFile);
    }

    // Valid: actions specified
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["actions"][0]["run_rule"] = "detect_phase_faults_rule";
        EXPECT_JSON_VALID(configFile);
    }

    // Invalid: rule_id and actions specified
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["actions"][0]["run_rule"] = "detect_phase_faults_rule";
        EXPECT_JSON_INVALID(configFile, "is valid under each of");
    }

    // Invalid: neither rule_id nor actions specified
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"].erase(
            "rule_id");
        EXPECT_JSON_INVALID(configFile,
                            "{} is not valid under any of the given schemas");
    }

    // Invalid: comments has wrong data type
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }

    // Invalid: device_id has wrong data type
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["device_id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: rule_id has wrong data type
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["rule_id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: actions has wrong data type
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["actions"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }

    // Invalid: device_id has invalid format
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["device_id"] = "id@";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: rule_id has invalid format
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["rule_id"] = "id@";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // Invalid: comments array is empty
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }

    // Invalid: actions array is empty
    {
        json configFile = initialFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
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
        EXPECT_JSON_INVALID(configFile, "'type' is a required property");
    }
    // Invalid: test pmbus_read_sensor with no command.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase(
            "command");
        EXPECT_JSON_INVALID(configFile, "'command' is a required property");
    }
    // Invalid: test pmbus_read_sensor with no format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"].erase(
            "format");
        EXPECT_JSON_INVALID(configFile, "'format' is a required property");
    }
    // Invalid: test pmbus_read_sensor with property type wrong type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["type"] =
            true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_read_sensor with property command wrong type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["command"] =
            true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_read_sensor with property format wrong type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["format"] =
            true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_read_sensor with property exponent wrong type.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["exponent"] =
            true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_read_sensor with property type wrong format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["type"] =
            "foo";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_read_sensor with property command wrong format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["command"] =
            "0x8B0";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_read_sensor with property format wrong format.
    {
        json configFile = pmbusReadSensorFile;
        configFile["rules"][0]["actions"][1]["pmbus_read_sensor"]["format"] =
            "foo";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
        EXPECT_JSON_INVALID(configFile, "'format' is a required property");
    }
    // Invalid: test pmbus_write_vout_command with property volts wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["volts"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_write_vout_command with property format wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["format"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_write_vout_command with property exponent wrong type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["exponent"] = 1.3;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_write_vout_command with property is_verified wrong
    // type.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["is_verified"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test pmbus_write_vout_command with property format wrong format.
    {
        json configFile = pmbusWriteVoutCommandFile;
        configFile["rules"][0]["actions"][1]["pmbus_write_vout_command"]
                  ["format"] = "foo";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: some property values contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
}

TEST(ValidateRegulatorsConfigTest, PresenceDetection)
{
    json presenceDetectionFile = validConfigFile;
    presenceDetectionFile["chassis"][0]["devices"][0]["presence_detection"]
                         ["comments"][0] =
                             "Regulator is only present if CPU3 is present";
    presenceDetectionFile["chassis"][0]["devices"][0]["presence_detection"]
                         ["rule_id"] = "detect_presence_rule";
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
                      "system/chassis/motherboard/cpu3";
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
                      "system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["presence_detection"]["actions"]
                  [0]["compare_presence"]["value"] = true;
        EXPECT_JSON_INVALID(configFile, "is valid under each of");
    }
    // Invalid: test presence_detection with no rule_id and actions.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test presence_detection with property comments wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test presence_detection with property rule_id wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["rule_id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test presence_detection with property actions wrong type.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["actions"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test presence_detection with property rule_id wrong format.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["rule_id"] = "id@";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test presence_detection with property comments empty array.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test presence_detection with property actions empty array.
    {
        json configFile = presenceDetectionFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"].erase(
            "rule_id");
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
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
        EXPECT_JSON_INVALID(configFile, "'id' is a required property");
    }
    // Invalid: test rail with comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test rail with id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test rail with configuration wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["configuration"] =
            true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test rail with sensor_monitoring wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]
                  ["sensor_monitoring"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
    // Invalid: test rail with comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["comments"] =
            json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test rail with id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["id"] = "id~";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
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
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'string'");
    }

    // invalid test rule with no ID
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("id");
        EXPECT_JSON_INVALID(configFile, "'id' is a required property");
    }

    // invalid test id property has invalid value type (not string)
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // invalid test id property has invalid value
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["id"] = "foo%";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }

    // invalid test rule with no actions property
    {
        json configFile = validConfigFile;
        configFile["rules"][0].erase("actions");
        EXPECT_JSON_INVALID(configFile, "'actions' is a required property");
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
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'array'");
    }

    // invalid test actions property has invalid value of action
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0] = "foo";
        EXPECT_JSON_INVALID(configFile, "'foo' is not of type 'object'");
    }

    // invalid test actions property has empty array
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }

    // invalid contains a template variable
    {
        json configFile = validConfigFile;
        configFile["rules"][0]["actions"][0]["pmbus_write_vout_command"]
                  ["format"] = "${format}";
        EXPECT_JSON_INVALID(configFile,
                            "Error: Rule contains template variable.");
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
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test run_rule wrong format.
    {
        json configFile = runRuleFile;
        configFile["rules"][0]["actions"][1]["run_rule"] = "set_voltage_rule%";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: rule id contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
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
                      "system/chassis/motherboard/cpu3";
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
                      "system/chassis/motherboard/cpu3";
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"][0]["compare_presence"]["value"] = true;
        EXPECT_JSON_INVALID(configFile, "is valid under each of");
    }
    // Invalid: test rails sensor_monitoring with no rule_id and actions.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test rails sensor_monitoring with property comments wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["comments"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test rails sensor_monitoring with property rule_id wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["rule_id"] = true;
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test rails sensor_monitoring with property actions wrong type.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'array'");
    }
    // Invalid: test rails sensor_monitoring with property rule_id wrong format.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["rule_id"] = "id@";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test rails sensor_monitoring with property comments empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["comments"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
    }
    // Invalid: test rails sensor_monitoring with property actions empty array.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
            .erase("rule_id");
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["actions"] = json::array();
        EXPECT_JSON_INVALID(configFile, "[] is too short");
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
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Invalid: test set_device wrong format.
    {
        json configFile = setDeviceFile;
        configFile["rules"][0]["actions"][1]["set_device"] = "io_expander2%";
        EXPECT_JSON_INVALID(configFile,
                            "is not valid under any of the given schemas");
    }
    // Valid: device id contain variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
}
TEST(ValidateRegulatorsConfigTest, StatusMonitoring)
{
    // Valid: test status_monitoring with all properties specified
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["status_monitoring"]["is_present_monitored"] =
            true;
        configFile["chassis"][0]["status_monitoring"]
                  ["is_available_monitored"] = true;
        configFile["chassis"][0]["status_monitoring"]["is_enabled_monitored"] =
            false;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test status_monitoring with only one property specified
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["status_monitoring"] = json::object();
        configFile["chassis"][0]["status_monitoring"]
                  ["is_available_monitored"] = true;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test status_monitoring with no properties specified
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["status_monitoring"] = json::object();
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test status_monitoring in chassis_template
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["status_monitoring"]
                  ["is_present_monitored"] = true;
        configFile["chassis_templates"][0]["status_monitoring"]
                  ["is_enabled_monitored"] = false;
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: test status_monitoring with template variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test status_monitoring with property is_present_monitored wrong
    // type
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["status_monitoring"]["is_present_monitored"] =
            "true";
        EXPECT_JSON_INVALID(configFile, "'true' does not match '.");
    }
    // Invalid: test status_monitoring with property is_available_monitored
    // wrong type
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["status_monitoring"]
                  ["is_available_monitored"] = 1;
        EXPECT_JSON_INVALID(configFile,
                            "1 is not valid under any of the given schemas");
    }
    // Invalid: test status_monitoring with property is_enabled_monitored wrong
    // type
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["status_monitoring"]["is_enabled_monitored"] =
            json::array();
        EXPECT_JSON_INVALID(configFile,
                            "[] is not valid under any of the given schemas");
    }
    // Invalid: test status_monitoring with invalid property
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["status_monitoring"]["is_present_monitored"] =
            true;
        configFile["chassis"][0]["status_monitoring"]["foo"] = true;
        EXPECT_JSON_INVALID(
            configFile,
            "Additional properties are not allowed ('foo' was unexpected)");
    }
}

TEST(ValidateRegulatorsConfigTest, StringWithVariables)
{
    // Valid: String with single variable
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = "${chassis_number}";
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: String with multiple variables
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["fru"] =
            "system/chassis${chassis_number}/motherboard/regulator${reg_num}";
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: String with same variable multiple times
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["inventory_path"] =
            "system/chassis${chassis_number}/board${chassis_number}";
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: Variable at start of string
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["id"] =
            "${chassis_number}_vdd";
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: Variable in middle of string
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["id"] =
            "regulator_${chassis_number}_vdd";
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: Variable at end of string
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["id"] =
            "regulator_${reg_num}";
        EXPECT_JSON_VALID(configFile);
    }
    // Valid: Variable name with letters, numbers, and underscores
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["id"] =
            "${Reg_Type_2}_regulator1";
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: String without variables
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["devices"][1]["i2c_interface"]
                  ["bus"] = "bus_num";
        EXPECT_JSON_INVALID(configFile, "does not match '.*");
    }
    // Invalid: Malformed variable syntax - missing closing brace
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = "${chassis_number";
        EXPECT_JSON_INVALID(configFile, "'${chassis_number' does not match");
    }
    // Invalid: Malformed variable syntax - missing opening brace
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = "$chassis_number}";
        EXPECT_JSON_INVALID(configFile, "'$chassis_number}' does not match");
    }
    // Invalid: Malformed variable syntax - missing dollar sign
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = "{chassis_number}";
        EXPECT_JSON_INVALID(configFile, "'{chassis_number}' does not match");
    }
    // Invalid: Malformed variable syntax - using parentheses instead of braces
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = "$(chassis_number)";
        EXPECT_JSON_INVALID(configFile, "'$(chassis_number)' does not match");
    }
    // Invalid: Variable name with invalid characters (hyphen)
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = "${chassis-number}";
        EXPECT_JSON_INVALID(configFile, "'${chassis-number}' does not match");
    }
    // Invalid: Variable name with invalid characters (space)
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = "${chassis number}";
        EXPECT_JSON_INVALID(configFile, "'${chassis number}' does not match");
    }
    // Invalid: Empty variable name
    {
        json configFile = validConfigFile;
        configFile["chassis_templates"][0]["number"] = "${}";
        EXPECT_JSON_INVALID(configFile, "'${}' does not match");
    }
}

TEST(ValidateRegulatorsConfigTest, TemplateVariableValues)
{
    // Valid: template_variable_values with multiple variables
    {
        json configFile = validConfigFile;
        EXPECT_JSON_VALID(configFile);
    }
    // Invalid: test template_variable_values with empty object
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_variable_values"] = json::object();
        EXPECT_JSON_INVALID(configFile, "{} does not have enough properties");
    }
    // Invalid: test template_variable_values with variable name containing
    // invalid character (hyphen)
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_variable_values"]["chassis-number"] =
            "1";
        EXPECT_JSON_INVALID(configFile, "does not match any of the regexes");
    }
    // Invalid: test template_variable_values with variable name containing
    // invalid character (space)
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_variable_values"]["chassis number"] =
            "1";
        EXPECT_JSON_INVALID(configFile, "does not match any of the regexes");
    }
    // Invalid: test template_variable_values with non-string value
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_variable_values"]["chassis_number"] =
            1;
        EXPECT_JSON_INVALID(configFile, "1 is not of type 'string'");
    }
    // Invalid: test template_variable_values wrong type (not object)
    {
        json configFile = validConfigFile;
        configFile["chassis"][1]["template_variable_values"] = true;
        EXPECT_JSON_INVALID(configFile, "True is not of type 'object'");
    }
}

TEST(ValidateRegulatorsConfigTest, DuplicateRuleID)
{
    // Invalid: test duplicate ID in rule.
    {
        json configFile = validConfigFile;
        configFile["rules"][4]["id"] = "set_voltage_rule";
        configFile["rules"][4]["actions"][0]["pmbus_write_vout_command"]
                  ["format"] = "linear";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate rule ID.");
    }
}

TEST(ValidateRegulatorsConfigTest, DuplicateChassisNumber)
{
    // Invalid: test duplicate number in chassis.
    {
        json configFile = validConfigFile;
        configFile["chassis"][2]["number"] = 1;
        configFile["chassis"][2]["inventory_path"] = "system/chassis3";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate chassis number.");
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
            "system/chassis/motherboard/regulator1";
        configFile["chassis"][0]["devices"][1]["i2c_interface"]["bus"] = 2;
        configFile["chassis"][0]["devices"][1]["i2c_interface"]["address"] =
            "0x71";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate device ID.");
    }
}

TEST(ValidateRegulatorsConfigTest, DuplicateRailID)
{
    // Invalid: test duplicate ID in rail.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][1]["id"] = "vdd";
        EXPECT_JSON_INVALID(configFile, "Error: Duplicate rail ID.");
    }
}

TEST(ValidateRegulatorsConfigTest, InfiniteLoops)
{
    // Invalid: test run_rule with infinite loop (rules run each other).
    {
        json configFile = validConfigFile;
        configFile["rules"][4]["actions"][0]["run_rule"] = "set_voltage_rule2";
        configFile["rules"][4]["id"] = "set_voltage_rule1";
        configFile["rules"][5]["actions"][0]["run_rule"] = "set_voltage_rule1";
        configFile["rules"][5]["id"] = "set_voltage_rule2";
        EXPECT_JSON_INVALID(configFile,
                            "Infinite loop caused by run_rule actions.");
    }
    // Invalid: test run_rule with infinite loop (rule runs itself).
    {
        json configFile = validConfigFile;
        configFile["rules"][4]["actions"][0]["run_rule"] = "set_voltage_rule1";
        configFile["rules"][4]["id"] = "set_voltage_rule1";
        EXPECT_JSON_INVALID(configFile,
                            "Infinite loop caused by run_rule actions.");
    }
    // Invalid: test run_rule with infinite loop (indirect loop).
    {
        json configFile = validConfigFile;
        configFile["rules"][4]["actions"][0]["run_rule"] = "set_voltage_rule2";
        configFile["rules"][4]["id"] = "set_voltage_rule1";
        configFile["rules"][5]["actions"][0]["run_rule"] = "set_voltage_rule3";
        configFile["rules"][5]["id"] = "set_voltage_rule2";
        configFile["rules"][6]["actions"][0]["run_rule"] = "set_voltage_rule1";
        configFile["rules"][6]["id"] = "set_voltage_rule3";
        EXPECT_JSON_INVALID(configFile,
                            "Infinite loop caused by run_rule actions.");
    }
}

TEST(ValidateRegulatorsConfigTest, RunRuleValueExists)
{
    // Invalid: test run_rule actions specify a rule ID that does not exist.
    {
        json configFile = validConfigFile;
        configFile["rules"][4]["actions"][0]["run_rule"] = "set_voltage_rule2";
        configFile["rules"][4]["id"] = "set_voltage_rule1";
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.");
    }
}

TEST(ValidateRegulatorsConfigTest, SetDeviceValueExists)
{
    // Invalid: test set_device actions specify a device ID that does not exist.
    {
        json configFile = validConfigFile;
        configFile["rules"][4]["actions"][0]["set_device"] = "vdd_regulator2";
        configFile["rules"][4]["id"] = "set_voltage_rule1";
        EXPECT_JSON_INVALID(configFile, "Error: Device ID does not exist.");
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
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.");
    }
    // Invalid: test rule_id property in presence_detection specifies a rule ID
    // that does not exist.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["presence_detection"]
                  ["rule_id"] = "detect_presence_rule2";
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.");
    }
    // Invalid: test rule_id property in phase_fault_detection specifies a rule
    // ID that does not exist.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["rule_id"] = "detect_phase_faults_rule2";
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.");
    }
    // Invalid: test rule_id property in sensor_monitoring specifies a rule ID
    // that does not exist.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["rails"][0]["sensor_monitoring"]
                  ["rule_id"] = "read_sensors_rule2";
        EXPECT_JSON_INVALID(configFile, "Error: Rule ID does not exist.");
    }
}

TEST(ValidateRegulatorsConfigTest, DeviceIDExists)
{
    // Invalid: test device_id property in phase_fault_detection specifies a
    // device ID that does not exist.
    {
        json configFile = validConfigFile;
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["device_id"] = "vdd_regulator2";
        configFile["chassis"][0]["devices"][0]["phase_fault_detection"]
                  ["rule_id"] = "detect_phase_faults_rule";
        EXPECT_JSON_INVALID(configFile, "Error: Device ID does not exist.");
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
                            "Error: Invalid i2c_compare_bytes action.");
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
                            "Error: Invalid i2c_write_bytes action.");
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
    std::string help =
        "usage: validate-regulators-config.py [-h] [-s SCHEMA_FILE]";
    std::string command;

    TemporaryFile tmpFile;
    std::string fileName = tmpFile.getPath().string();
    writeDataToFile(validConfigFile, fileName);
    // Valid: -s specified
    {
        command = validateTool + "-s " + schemaFile + configuration + fileName;
        expectCommandLineSyntax(command, "", 0);
    }
    // Valid: --schema-file specified
    {
        command = validateTool + "--schema-file " + schemaFile + configuration +
                  fileName;
        expectCommandLineSyntax(command, "", 0);
    }
    // Valid: -c specified
    {
        command = validateTool + schema + schemaFile + "-c " + fileName;
        expectCommandLineSyntax(command, "", 0);
    }
    // Valid: --configuration-file specified
    {
        command = validateTool + schema + schemaFile + "--configuration-file " +
                  fileName;
        expectCommandLineSyntax(command, "", 0);
    }
    // Valid: -h specified
    {
        command = validateTool + "-h ";
        expectCommandLineSyntax(command, help, 0);
    }
    // Valid: --help specified
    {
        command = validateTool + "--help ";
        expectCommandLineSyntax(command, help, 0);
    }
    // Invalid: -c/--configuration-file not specified
    {
        command = validateTool + schema + schemaFile;
        expectCommandLineSyntax(command,
                                "Error: Configuration file is required.", 1);
    }
    // Invalid: -s/--schema-file not specified
    {
        command = validateTool + configuration + fileName;
        expectCommandLineSyntax(command, "Error: Schema file is required.", 1);
    }
    // Invalid: -c specified more than once
    {
        command = validateTool + schema + schemaFile + "-c -c " + fileName;
        expectCommandLineSyntax(command, help, 2);
    }
    // Invalid: -s specified more than once
    {
        command = validateTool + "-s -s " + schemaFile + configuration +
                  fileName;
        expectCommandLineSyntax(command, help, 2);
    }
    // Invalid: No file name specified after -c
    {
        command = validateTool + schema + schemaFile + configuration;
        expectCommandLineSyntax(command, help, 2);
    }
    // Invalid: No file name specified after -s
    {
        command = validateTool + schema + configuration + fileName;
        expectCommandLineSyntax(command, help, 2);
    }
    // Invalid: File specified after -c does not exist
    {
        command = validateTool + schema + schemaFile + configuration +
                  "../notExistFile";
        expectCommandLineSyntax(command,
                                "Error: Configuration file does not exist.", 1);
    }
    // Invalid: File specified after -s does not exist
    {
        command = validateTool + schema + "../notExistFile " + configuration +
                  fileName;
        expectCommandLineSyntax(command, "Error: Schema file does not exist.",
                                1);
    }
    // Invalid: File specified after -c is not right data format
    {
        TemporaryFile wrongFormatFile;
        std::string wrongFormatFileName = wrongFormatFile.getPath().string();
        std::ofstream out(wrongFormatFileName);
        out << "foo";
        out.close();
        command = validateTool + schema + schemaFile + configuration +
                  wrongFormatFileName;
        expectCommandLineSyntax(
            command, "Error: Configuration file is not in the JSON format.", 1);
    }
    // Invalid: File specified after -s is not right data format
    {
        TemporaryFile wrongFormatFile;
        std::string wrongFormatFileName = wrongFormatFile.getPath().string();
        std::ofstream out(wrongFormatFileName);
        out << "foo";
        out.close();
        command = validateTool + schema + wrongFormatFileName + configuration +
                  fileName;
        expectCommandLineSyntax(
            command, "Error: Schema file is not in the JSON format.", 1);
    }
    // Invalid: File specified after -c is not readable
    {
        TemporaryFile notReadableFile;
        std::string notReadableFileName = notReadableFile.getPath().string();
        writeDataToFile(validConfigFile, notReadableFileName);
        command = validateTool + schema + schemaFile + configuration +
                  notReadableFileName;
        chmod(notReadableFileName.c_str(), 0222);
        expectCommandLineSyntax(
            command, "Error: Configuration file is not readable.", 1);
    }
    // Invalid: File specified after -s is not readable
    {
        TemporaryFile notReadableFile;
        std::string notReadableFileName = notReadableFile.getPath().string();
        writeDataToFile(validConfigFile, notReadableFileName);
        command = validateTool + schema + notReadableFileName + configuration +
                  fileName;
        chmod(notReadableFileName.c_str(), 0222);
        expectCommandLineSyntax(command, "Error: Schema file is not readable.",
                                1);
    }
    // Invalid: Unexpected parameter specified (like -g)
    {
        command = validateTool + schema + schemaFile + configuration +
                  fileName + " -g";
        expectCommandLineSyntax(command, help, 2);
    }
}
