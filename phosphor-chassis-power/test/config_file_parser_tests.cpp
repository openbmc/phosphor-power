/**
 * Copyright © 2026 IBM Corporation
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

#include "chassis.hpp"
#include "config_file_parser.hpp"
#include "config_file_parser_error.hpp"
#include "gpio.hpp"
#include "temporary_file.hpp"

#include <sys/stat.h> // for chmod()

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::chassis;
using namespace phosphor::power::chassis::config_file_parser;
using namespace phosphor::power::chassis::config_file_parser::internal;
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
    // Test where works: Minimal chassis (only ChassisNumber)
    {
        const json configFileContents = R"(
            [
              { "ChassisNumber": 1 },
              { "ChassisNumber": 2 },
              { "ChassisNumber": 3 }
            ]
        )"_json;

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        std::vector<std::unique_ptr<Chassis>> chassis = parse(pathName);

        EXPECT_EQ(chassis.size(), 3);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[1]->getNumber(), 2);
        EXPECT_EQ(chassis[2]->getNumber(), 3);
    }

    // Test where works: Chassis with all optional properties
    {
        const json configFileContents = R"(
            [
              {
                "ChassisNumber": 1,
                "PresencePath": "/dev/i2c-159",
                "PresenceGpio": {
                  "Name": "presence-chassis1",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultUnlatchedGpio": {
                  "Name": "power-chassis1-standby-fault-unlatched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchedGpio": {
                  "Name": "power-chassis1-standby-fault-latched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchResetGpio": {
                  "Name": "power-chassis1-standby-fault-reset",
                  "Direction": "Output",
                  "Polarity": "Low"
                },
                "EnableSystemResetGpio": {
                  "Name": "reset-enable-chassis1-standby-power",
                  "Direction": "Output",
                  "Polarity": "Low"
                }
              }
            ]
        )"_json;

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        std::vector<std::unique_ptr<Chassis>> chassis = parse(pathName);

        EXPECT_EQ(chassis.size(), 1);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[0]->getGpios().size(), 5);
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
            [
              { "ChassisNumber": 1 }
            ]
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

TEST(ConfigFileParserTests, ParseChassis)
{
    // Test where works: Only required property (ChassisNumber) specified
    {
        const json element = R"(
        {
            "ChassisNumber": 1
        }
    )"_json;

        std::unique_ptr<Chassis> chassis = parseChassis(element);
        EXPECT_EQ(chassis->getNumber(), 1);
        EXPECT_EQ(chassis->getGpios().size(), 0);
    }

    // Test where works: Only PresencePath specified
    {
        const json element = R"(
            {
                "ChassisNumber": 3,
                "PresencePath": "/dev/i2c-259"
            }
        )"_json;
        std::unique_ptr<Chassis> chassis = parseChassis(element);
        EXPECT_EQ(chassis->getNumber(), 3);
        EXPECT_EQ(chassis->getGpios().size(), 0);
        EXPECT_EQ(chassis->getPresencePath(),
                  "/xyz/openbmc_project/inventory/dev/i2c-259");
    }

    // Test where works: All optional GPIO properties specified
    {
        const json element = R"(
        {
            "ChassisNumber": 2,
            "PresencePath": "/dev/i2c-259",
            "PresenceGpio": {
                "Name": "presence-chassis2",
                "Direction": "Input",
                "Polarity": "Low"
            },
            "FaultUnlatchedGpio": {
                "Name": "power-chassis2-standby-fault-unlatched",
                "Direction": "Input",
                "Polarity": "Low"
            },
            "FaultLatchedGpio": {
                "Name": "power-chassis2-standby-fault-latched",
                "Direction": "Input",
                "Polarity": "Low"
            },
            "FaultLatchResetGpio": {
                "Name": "power-chassis2-standby-fault-reset",
                "Direction": "Output",
                "Polarity": "Low"
            },
            "EnableSystemResetGpio": {
                "Name": "reset-enable-chassis2-standby-power",
                "Direction": "Output",
                "Polarity": "Low"
            }
        }
    )"_json;

        std::unique_ptr<Chassis> chassis = parseChassis(element);
        EXPECT_EQ(chassis->getNumber(), 2);
        EXPECT_EQ(chassis->getPresencePath(),
                  "/xyz/openbmc_project/inventory/dev/i2c-259");
        EXPECT_EQ(chassis->getGpios().size(), 5);
        EXPECT_EQ(chassis->getGpios()[0]->getName(), "presence-chassis2");
        EXPECT_EQ(chassis->getGpios()[1]->getName(),
                  "power-chassis2-standby-fault-unlatched");
        EXPECT_EQ(chassis->getGpios()[2]->getName(),
                  "power-chassis2-standby-fault-latched");
        EXPECT_EQ(chassis->getGpios()[3]->getName(),
                  "power-chassis2-standby-fault-reset");
        EXPECT_EQ(chassis->getGpios()[4]->getName(),
                  "reset-enable-chassis2-standby-power");
    }

    // Test where works: Only PresenceGpio specified
    {
        const json element = R"(
            {
                "ChassisNumber": 3,
                "PresenceGpio": {
                    "Name": "presence-chassis3",
                    "Direction": "Input",
                    "Polarity": "Low"
                }
            }
        )"_json;
        std::unique_ptr<Chassis> chassis = parseChassis(element);
        EXPECT_EQ(chassis->getNumber(), 3);
        EXPECT_EQ(chassis->getGpios().size(), 1);
        EXPECT_EQ(chassis->getGpios()[0]->getName(), "presence-chassis3");
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

    // Test where fails: Required ChassisNumber property not specified
    try
    {
        const json element = R"(
            {
                "PresencePath": "/dev/i2c-159"
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: ChassisNumber");
    }

    // Test where fails: ChassisNumber value is < 1
    try
    {
        const json element = R"(
            {
                "ChassisNumber": 0
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid chassis number: Must be > 0");
    }

    // Test where fails: ChassisNumber value is not an integer
    try
    {
        const json element = R"(
            {
                "ChassisNumber": 0.5
            }
        )"_json;
        parseChassis(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
                "ChassisNumber": 1,
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
}

TEST(ConfigFileParserTests, ParseChassisArray)
{
    // Test where works
    {
        const json element = R"(
            [
              { "ChassisNumber": 1 },
              { "ChassisNumber": 2 }
            ]
        )"_json;
        std::vector<std::unique_ptr<Chassis>> chassis =
            parseChassisArray(element);
        EXPECT_EQ(chassis.size(), 2);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[1]->getNumber(), 2);
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

TEST(ConfigFileParserTests, ParseGPIOs)
{
    // Test where works: All required properties specified
    {
        const json element = R"(
            {
              "Name": "presence-chassis1",
              "Direction": "Input",
              "Polarity": "Low"
            }
        )"_json;
        std::unique_ptr<Gpio> gpio = parseGpio(element);
        EXPECT_EQ(gpio->getName(), "presence-chassis1");
        EXPECT_EQ(gpio->getDirection(), gpioDirection::Input);
        EXPECT_EQ(gpio->getPolarity(), gpioPolarity::Low);
    }

    // Test where works: Output direction and High polarity
    {
        const json element = R"(
            {
              "Name": "power-chassis1-standby-fault-reset",
              "Direction": "Output",
              "Polarity": "High"
            }
        )"_json;
        std::unique_ptr<Gpio> gpio = parseGpio(element);
        EXPECT_EQ(gpio->getName(), "power-chassis1-standby-fault-reset");
        EXPECT_EQ(gpio->getDirection(), gpioDirection::Output);
        EXPECT_EQ(gpio->getPolarity(), gpioPolarity::High);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "0xFF", "0x01" ] )"_json;
        parseGpio(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required Name property not specified
    try
    {
        const json element = R"(
            {
              "Direction": "Input",
              "Polarity": "Low"
            }
        )"_json;
        parseGpio(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: Name");
    }

    // Test where fails: Required Direction property not specified
    try
    {
        const json element = R"(
            {
              "Name": "presence-chassis1",
              "Polarity": "Low"
            }
        )"_json;
        parseGpio(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: Direction");
    }

    // Test where fails: Required Polarity property not specified
    try
    {
        const json element = R"(
            {
              "Name": "presence-chassis1",
              "Direction": "Input"
            }
        )"_json;
        parseGpio(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: Polarity");
    }

    // Test where fails: Name value is not a string
    try
    {
        const json element = R"(
            {
              "Name": 1,
              "Direction": "Input",
              "Polarity": "Low"
            }
        )"_json;
        parseGpio(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Direction value is not a string
    try
    {
        const json element = R"(
            {
              "Name": "presence-chassis1",
              "Direction": 1,
              "Polarity": "Low"
            }
        )"_json;
        parseGpio(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Polarity value is not a string
    try
    {
        const json element = R"(
            {
              "Name": "presence-chassis1",
              "Direction": "Input",
              "Polarity": 1
            }
        )"_json;
        parseGpio(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
              "Name": "presence-chassis1",
              "Direction": "Input",
              "Polarity": "Low",
              "foo": "bar"
            }
        )"_json;
        parseGpio(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseInventoryPath)
{
    // Test where works: Relative path (no leading slash)
    {
        const json element = R"( "system/chassis1" )"_json;
        std::string path = parseInventoryPath(element);
        EXPECT_EQ(path, "/xyz/openbmc_project/inventory/system/chassis1");
    }

    // Test where works: Path with leading slash
    {
        const json element = R"( "/system/chassis1" )"_json;
        std::string path = parseInventoryPath(element);
        EXPECT_EQ(path, "/xyz/openbmc_project/inventory/system/chassis1");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = R"( 1 )"_json;
        parseInventoryPath(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Element is an empty string
    try
    {
        const json element = R"( "" )"_json;
        parseInventoryPath(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }
}

TEST(ConfigFileParserTests, ParseRoot)
{
    // Test where works: Array with one chassis
    {
        const json element = R"(
            [
              { "ChassisNumber": 1 }
            ]
        )"_json;
        std::vector<std::unique_ptr<Chassis>> chassis = parseRoot(element);
        EXPECT_EQ(chassis.size(), 1);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
    }

    // Test where works: Array with multiple chassis
    {
        const json element = R"(
            [
              { "ChassisNumber": 1 },
              { "ChassisNumber": 2 },
              { "ChassisNumber": 3 }
            ]
        )"_json;
        std::vector<std::unique_ptr<Chassis>> chassis = parseRoot(element);
        EXPECT_EQ(chassis.size(), 3);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[1]->getNumber(), 2);
        EXPECT_EQ(chassis[2]->getNumber(), 3);
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
              "foo": "bar"
            }
        )"_json;
        parseRoot(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(ConfigFileParserTests, ParseManyChassis)
{
    // Test where works: Chassis with all optional properties
    {
        const json configFileContents = R"(
            [
              {
                "ChassisNumber": 1,
                "PresencePath": "/dev/i2c-159",
                "PresenceGpio": {
                  "Name": "presence-chassis1",
                  "Direction": "Input",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 2,
                "PresencePath": "/dev/i2c-259",
                "PresenceGpio": {
                  "Name": "presence-chassis2",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultUnlatchedGpio": {
                  "Name": "power-chassis2-standby-fault-unlatched",
                  "Direction": "Input",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 3,
                "PresencePath": "/dev/i2c-359",
                "PresenceGpio": {
                  "Name": "presence-chassis3",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultUnlatchedGpio": {
                  "Name": "power-chassis3-standby-fault-unlatched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchedGpio": {
                  "Name": "power-chassis3-standby-fault-latched",
                  "Direction": "Input",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 4,
                "PresencePath": "/dev/i2c-459",
                "PresenceGpio": {
                  "Name": "presence-chassis4",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultUnlatchedGpio": {
                  "Name": "power-chassis4-standby-fault-unlatched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchedGpio": {
                  "Name": "power-chassis4-standby-fault-latched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchResetGpio": {
                  "Name": "power-chassis4-standby-fault-reset",
                  "Direction": "Output",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 5,
                "PresencePath": "/dev/i2c-559",
                "PresenceGpio": {
                  "Name": "presence-chassis5",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultUnlatchedGpio": {
                  "Name": "power-chassis5-standby-fault-unlatched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchedGpio": {
                  "Name": "power-chassis5-standby-fault-latched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchResetGpio": {
                  "Name": "power-chassis5-standby-fault-reset",
                  "Direction": "Output",
                  "Polarity": "Low"
                },
                "EnableSystemResetGpio": {
                  "Name": "reset-enable-chassis5-standby-power",
                  "Direction": "Output",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 6,
                "PresencePath": "/dev/i2c-659",
                "PresenceGpio": {
                  "Name": "presence-chassis6",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultUnlatchedGpio": {
                  "Name": "power-chassis6-standby-fault-unlatched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchedGpio": {
                  "Name": "power-chassis6-standby-fault-latched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchResetGpio": {
                  "Name": "power-chassis6-standby-fault-reset",
                  "Direction": "Output",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 7,
                "PresencePath": "/dev/i2c-759",
                "PresenceGpio": {
                  "Name": "presence-chassis7",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultUnlatchedGpio": {
                  "Name": "power-chassis7-standby-fault-unlatched",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultLatchedGpio": {
                  "Name": "power-chassis7-standby-fault-latched",
                  "Direction": "Input",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 8,
                "PresencePath": "/dev/i2c-859",
                "PresenceGpio": {
                  "Name": "presence-chassis8",
                  "Direction": "Input",
                  "Polarity": "Low"
                },
                "FaultUnlatchedGpio": {
                  "Name": "power-chassis8-standby-fault-unlatched",
                  "Direction": "Input",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 9,
                "PresencePath": "/dev/i2c-959",
                "PresenceGpio": {
                  "Name": "presence-chassis9",
                  "Direction": "Input",
                  "Polarity": "Low"
                }
              },
              {
                "ChassisNumber": 10,
                "PresencePath": "/dev/i2c-1059"
              }
            ]
        )"_json;

        std::vector<int> gpiosPerChassis = {1, 2, 3, 4, 5, 4, 3, 2, 1, 0};

        std::vector<std::string> gpioNamePresenceGpio = {
            "presence-chassis1", "presence-chassis2",
            "presence-chassis3", "presence-chassis4",
            "presence-chassis5", "presence-chassis6",
            "presence-chassis7", "presence-chassis8",
            "presence-chassis9", "",
        };

        std::vector<std::string> gpioNameFaultUnlatchedGpio = {
            "power-chassis1-standby-fault-unlatched",
            "power-chassis2-standby-fault-unlatched",
            "power-chassis3-standby-fault-unlatched",
            "power-chassis4-standby-fault-unlatched",
            "power-chassis5-standby-fault-unlatched",
            "power-chassis6-standby-fault-unlatched",
            "power-chassis7-standby-fault-unlatched",
            "power-chassis8-standby-fault-unlatched",
            "power-chassis9-standby-fault-unlatched",
            "",
        };

        std::vector<std::string> gpioNameFaultLatchedGpio = {
            "power-chassis1-standby-fault-latched",
            "power-chassis2-standby-fault-latched",
            "power-chassis3-standby-fault-latched",
            "power-chassis4-standby-fault-latched",
            "power-chassis5-standby-fault-latched",
            "power-chassis6-standby-fault-latched",
            "power-chassis7-standby-fault-latched",
            "power-chassis8-standby-fault-latched",
            "power-chassis9-standby-fault-latched",
            "",
        };

        std::vector<std::string> gpioNameFaultLatchResetGpio = {
            "power-chassis1-standby-fault-reset",
            "power-chassis2-standby-fault-reset",
            "power-chassis3-standby-fault-reset",
            "power-chassis4-standby-fault-reset",
            "power-chassis5-standby-fault-reset",
            "power-chassis6-standby-fault-reset",
            "power-chassis7-standby-fault-reset",
            "power-chassis8-standby-fault-reset",
            "power-chassis9-standby-fault-reset",
            "",
        };

        std::vector<std::string> gpioNameEnableSystemResetGpio = {
            "reset-enable-chassis1-standby-power",
            "reset-enable-chassis2-standby-power",
            "reset-enable-chassis3-standby-power",
            "reset-enable-chassis4-standby-power",
            "reset-enable-chassis5-standby-power",
            "reset-enable-chassis6-standby-power",
            "reset-enable-chassis7-standby-power",
            "reset-enable-chassis8-standby-power",
            "reset-enable-chassis9-standby-power",
            "",
        };

        std::vector<gpioDirection> gpioDirection = {
            gpioDirection::Input,  gpioDirection::Input,  gpioDirection::Input,
            gpioDirection::Output, gpioDirection::Output, gpioDirection::Output,
            gpioDirection::Input,  gpioDirection::Input,  gpioDirection::Input,
        };

        std::vector<gpioPolarity> gpioPolarity = {
            gpioPolarity::Low, gpioPolarity::Low, gpioPolarity::Low,
            gpioPolarity::Low, gpioPolarity::Low, gpioPolarity::Low,
            gpioPolarity::Low, gpioPolarity::Low, gpioPolarity::Low,
        };

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        std::vector<std::unique_ptr<Chassis>> chassis = parse(pathName);

        EXPECT_EQ(chassis.size(), 10);

        int indexNumber = 0;
        // Loop over the vector of Chassis.
        for (auto& thisChassis : chassis)
        {
            EXPECT_EQ(thisChassis->getNumber(), indexNumber + 1);
            EXPECT_EQ(thisChassis->getGpios().size(),
                      gpiosPerChassis[indexNumber]);
            const auto& chassisGpios = thisChassis->getGpios();
            EXPECT_EQ(chassisGpios.size(), gpiosPerChassis[indexNumber]);
            int thisGpiosIndex = 0;
            // Loop over this chassis's vector of GPIOs.
            for (auto& thisGpio : chassisGpios)
            {
                switch (thisGpiosIndex)
                {
                    case 4:
                        EXPECT_EQ(thisGpio->getName(),
                                  gpioNameEnableSystemResetGpio[indexNumber]);
                        break;
                    case 3:
                        EXPECT_EQ(thisGpio->getName(),
                                  gpioNameFaultLatchResetGpio[indexNumber]);
                        break;
                    case 2:
                        EXPECT_EQ(thisGpio->getName(),
                                  gpioNameFaultLatchedGpio[indexNumber]);
                        break;
                    case 1:
                        EXPECT_EQ(thisGpio->getName(),
                                  gpioNameFaultUnlatchedGpio[indexNumber]);
                        break;
                    case 0:
                        EXPECT_EQ(thisGpio->getName(),
                                  gpioNamePresenceGpio[indexNumber]);
                        break;
                    default:
                        break;
                }
                EXPECT_EQ(thisGpio->getDirection(),
                          gpioDirection[thisGpiosIndex]);
                EXPECT_EQ(thisGpio->getPolarity(),
                          gpioPolarity[thisGpiosIndex]);
                ++thisGpiosIndex;
            }
            ++indexNumber;
        }
    }
}
