/**
 * Copyright © 2024 IBM Corporation
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
#include "config_file_parser.hpp"
#include "config_file_parser_error.hpp"
#include "rail.hpp"
#include "temporary_file.hpp"

#include <sys/stat.h> // for chmod()

#include <nlohmann/json.hpp>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;
using namespace phosphor::power::sequencer::config_file_parser;
using namespace phosphor::power::sequencer::config_file_parser::internal;
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
                "rails": [
                    {
                        "name": "VDD_CPU0",
                        "page": 11,
                        "check_status_vout": true
                    },
                    {
                        "name": "VCS_CPU1",
                        "presence": "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1",
                        "gpio": { "line": 60 }
                    }
                ]
            }
        )"_json;

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        std::vector<std::unique_ptr<Rail>> rails = parse(pathName);

        EXPECT_EQ(rails.size(), 2);
        EXPECT_EQ(rails[0]->getName(), "VDD_CPU0");
        EXPECT_EQ(rails[1]->getName(), "VCS_CPU1");
    }

    // Test where fails: File does not exist
    {
        std::filesystem::path pathName{"/tmp/non_existent_file"};
        EXPECT_THROW(parse(pathName), ConfigFileParserError);
    }

    // Test where fails: File is not readable
    {
        const json configFileContents = R"(
            {
                "rails": [
                    {
                        "name": "VDD_CPU0"
                    }
                ]
            }
        )"_json;

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        chmod(pathName.c_str(), 0222);
        EXPECT_THROW(parse(pathName), ConfigFileParserError);
    }

    // Test where fails: File is not valid JSON
    {
        const std::string configFileContents = "] foo [";

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        EXPECT_THROW(parse(pathName), ConfigFileParserError);
    }

    // Test where fails: JSON does not conform to config file format
    {
        const json configFileContents = R"( [ "foo", "bar" ] )"_json;

        TemporaryFile configFile;
        std::filesystem::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        EXPECT_THROW(parse(pathName), ConfigFileParserError);
    }
}

TEST(ConfigFileParserTests, GetRequiredProperty)
{
    // Test where property exists
    {
        const json element = R"( { "name": "VDD_CPU0" } )"_json;
        const json& propertyElement = getRequiredProperty(element, "name");
        EXPECT_EQ(propertyElement.get<std::string>(), "VDD_CPU0");
    }

    // Test where property does not exist
    try
    {
        const json element = R"( { "foo": 23 } )"_json;
        getRequiredProperty(element, "name");
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: name");
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

TEST(ConfigFileParserTests, ParseGPIO)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
                "line": 60
            }
        )"_json;
        GPIO gpio = parseGPIO(element);
        EXPECT_EQ(gpio.line, 60);
        EXPECT_FALSE(gpio.activeLow);
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
                "line": 131,
                "active_low": true
            }
        )"_json;
        GPIO gpio = parseGPIO(element);
        EXPECT_EQ(gpio.line, 131);
        EXPECT_TRUE(gpio.activeLow);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "vdda", "vddb" ] )"_json;
        parseGPIO(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required line property not specified
    try
    {
        const json element = R"(
            {
                "active_low": true
            }
        )"_json;
        parseGPIO(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: line");
    }

    // Test where fails: line value is invalid
    try
    {
        const json element = R"(
            {
                "line": -131,
                "active_low": true
            }
        )"_json;
        parseGPIO(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an unsigned integer");
    }

    // Test where fails: active_low value is invalid
    try
    {
        const json element = R"(
            {
                "line": 131,
                "active_low": "true"
            }
        )"_json;
        parseGPIO(element);
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
                "line": 131,
                "foo": "bar"
            }
        )"_json;
        parseGPIO(element);
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
                "name": "VDD_CPU0"
            }
        )"_json;
        std::unique_ptr<Rail> rail = parseRail(element);
        EXPECT_EQ(rail->getName(), "VDD_CPU0");
        EXPECT_FALSE(rail->getPresence().has_value());
        EXPECT_FALSE(rail->getPage().has_value());
        EXPECT_FALSE(rail->getCheckStatusVout());
        EXPECT_FALSE(rail->getCompareVoltageToLimits());
        EXPECT_FALSE(rail->getGPIO().has_value());
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "presence": "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1",
                "page": 11,
                "check_status_vout": true,
                "compare_voltage_to_limits": true,
                "gpio": { "line": 60, "active_low": true }
            }
        )"_json;
        std::unique_ptr<Rail> rail = parseRail(element);
        EXPECT_EQ(rail->getName(), "VCS_CPU1");
        EXPECT_TRUE(rail->getPresence().has_value());
        EXPECT_EQ(
            rail->getPresence().value(),
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1");
        EXPECT_TRUE(rail->getPage().has_value());
        EXPECT_EQ(rail->getPage().value(), 11);
        EXPECT_TRUE(rail->getCheckStatusVout());
        EXPECT_TRUE(rail->getCompareVoltageToLimits());
        EXPECT_TRUE(rail->getGPIO().has_value());
        EXPECT_EQ(rail->getGPIO().value().line, 60);
        EXPECT_TRUE(rail->getGPIO().value().activeLow);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "vdda", "vddb" ] )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required name property not specified
    try
    {
        const json element = R"(
            {
                "page": 11
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: name");
    }

    // Test where fails: name value is invalid
    try
    {
        const json element = R"(
            {
                "name": 31,
                "page": 11
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: presence value is invalid
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "presence": false
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: page value is invalid
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "page": 256
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit unsigned integer");
    }

    // Test where fails: check_status_vout value is invalid
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "check_status_vout": "false"
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }

    // Test where fails: compare_voltage_to_limits value is invalid
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "compare_voltage_to_limits": 23
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }

    // Test where fails: gpio value is invalid
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "gpio": 131
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: check_status_vout is true and page not specified
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "check_status_vout": true
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: page");
    }

    // Test where fails: compare_voltage_to_limits is true and page not
    // specified
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "compare_voltage_to_limits": true
            }
        )"_json;
        parseRail(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: page");
    }

    // Test where fails: Invalid property specified
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "foo": "bar"
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
    // Test where works: Array is empty
    {
        const json element = R"(
            [
            ]
        )"_json;
        std::vector<std::unique_ptr<Rail>> rails = parseRailArray(element);
        EXPECT_EQ(rails.size(), 0);
    }

    // Test where works: Array is not empty
    {
        const json element = R"(
            [
                { "name": "VDD_CPU0" },
                { "name": "VCS_CPU1" }
            ]
        )"_json;
        std::vector<std::unique_ptr<Rail>> rails = parseRailArray(element);
        EXPECT_EQ(rails.size(), 2);
        EXPECT_EQ(rails[0]->getName(), "VDD_CPU0");
        EXPECT_EQ(rails[1]->getName(), "VCS_CPU1");
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

    // Test where fails: Element within array is invalid
    try
    {
        const json element = R"(
            [
                { "name": "VDD_CPU0" },
                23
            ]
        )"_json;
        parseRailArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }
}

TEST(ConfigFileParserTests, ParseRoot)
{
    // Test where works
    {
        const json element = R"(
            {
                "rails": [
                    {
                        "name": "VDD_CPU0",
                        "page": 11,
                        "check_status_vout": true
                    },
                    {
                        "name": "VCS_CPU1",
                        "presence": "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1",
                        "gpio": { "line": 60 }
                    }
                ]
            }
        )"_json;
        std::vector<std::unique_ptr<Rail>> rails = parseRoot(element);
        EXPECT_EQ(rails.size(), 2);
        EXPECT_EQ(rails[0]->getName(), "VDD_CPU0");
        EXPECT_EQ(rails[1]->getName(), "VCS_CPU1");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "VDD_CPU0", "VCS_CPU1" ] )"_json;
        parseRoot(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required rails property not specified
    try
    {
        const json element = R"(
            {
            }
        )"_json;
        parseRoot(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: rails");
    }

    // Test where fails: rails value is invalid
    try
    {
        const json element = R"(
            {
                "rails": 31
            }
        )"_json;
        parseRoot(element);
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
                "rails": [
                    {
                        "name": "VDD_CPU0",
                        "page": 11,
                        "check_status_vout": true
                    }
                ],
                "foo": true
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
        const json element = "vdd_cpu1";
        std::string value = parseString(element, false);
        EXPECT_EQ(value, "vdd_cpu1");
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

TEST(ConfigFileParserTests, ParseUnsignedInteger)
{
    // Test where works: 1
    {
        const json element = R"( 1 )"_json;
        unsigned int value = parseUnsignedInteger(element);
        EXPECT_EQ(value, 1);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.5 )"_json;
        parseUnsignedInteger(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an unsigned integer");
    }

    // Test where fails: Value < 0
    try
    {
        const json element = R"( -1 )"_json;
        parseUnsignedInteger(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an unsigned integer");
    }
}

TEST(ConfigFileParserTests, VerifyIsArray)
{
    // Test where element is an array
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        verifyIsArray(element);
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
    {
        const json element = R"( { "foo": "bar" } )"_json;
        verifyIsObject(element);
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
    {
        const json element = R"(
            {
                "line": 131,
                "active_low": true
            }
        )"_json;
        verifyPropertyCount(element, 2);
    }

    // Test where element has unexpected number of properties
    try
    {
        const json element = R"(
            {
                "line": 131,
                "active_low": true,
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
