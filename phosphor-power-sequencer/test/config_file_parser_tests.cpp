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
#include "chassis.hpp"
#include "config_file_parser.hpp"
#include "config_file_parser_error.hpp"
#include "mock_services.hpp"
#include "power_sequencer_device.hpp"
#include "rail.hpp"
#include "temporary_file.hpp"
#include "temporary_subdirectory.hpp"

#include <sys/stat.h> // for chmod()

#include <nlohmann/json.hpp>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;
using namespace phosphor::power::sequencer::config_file_parser;
using namespace phosphor::power::sequencer::config_file_parser::internal;
using namespace phosphor::power::util;
using json = nlohmann::json;
namespace fs = std::filesystem;

void writeConfigFile(const fs::path& pathName, const std::string& contents)
{
    std::ofstream file{pathName};
    file << contents;
}

void writeConfigFile(const fs::path& pathName, const json& contents)
{
    std::ofstream file{pathName};
    file << contents;
}

TEST(ConfigFileParserTests, Find)
{
    std::vector<std::string> compatibleSystemTypes{
        "com.acme.Hardware.Chassis.Model.MegaServer4CPU",
        "com.acme.Hardware.Chassis.Model.MegaServer",
        "com.acme.Hardware.Chassis.Model.Server"};

    // Test where works: Fully qualified system type: First in list
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "com.acme.Hardware.Chassis.Model.MegaServer4CPU.json";
        writeConfigFile(configFilePath, std::string{""});

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_EQ(pathFound, configFilePath);
    }

    // Test where works: Fully qualified system type: Second in list
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "com.acme.Hardware.Chassis.Model.MegaServer.json";
        writeConfigFile(configFilePath, std::string{""});

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_EQ(pathFound, configFilePath);
    }

    // Test where works: Last node in system type: Second in list
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "MegaServer.json";
        writeConfigFile(configFilePath, std::string{""});

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_EQ(pathFound, configFilePath);
    }

    // Test where works: Last node in system type: Last in list
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "Server.json";
        writeConfigFile(configFilePath, std::string{""});

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_EQ(pathFound, configFilePath);
    }

    // Test where works: System type has no '.'
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "Server.json";
        writeConfigFile(configFilePath, std::string{""});

        std::vector<std::string> noDotSystemTypes{"MegaServer4CPU",
                                                  "MegaServer", "Server"};
        fs::path pathFound = find(noDotSystemTypes, configFileDirPath);
        EXPECT_EQ(pathFound, configFilePath);
    }

    // Test where fails: System type list is empty
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "Server.json";
        writeConfigFile(configFilePath, std::string{""});

        std::vector<std::string> emptySystemTypes{};
        fs::path pathFound = find(emptySystemTypes, configFileDirPath);
        EXPECT_TRUE(pathFound.empty());
    }

    // Test where fails: Configuration file directory is empty
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_TRUE(pathFound.empty());
    }

    // Test where fails: Configuration file directory does not exist
    {
        fs::path configFileDirPath{"/tmp/does_not_exist_XYZ"};

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_TRUE(pathFound.empty());
    }

    // Test where fails: Configuration file directory is not readable
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();
        fs::permissions(configFileDirPath, fs::perms::none);

        EXPECT_THROW(find(compatibleSystemTypes, configFileDirPath),
                     std::exception);

        fs::permissions(configFileDirPath, fs::perms::owner_all);
    }

    // Test where fails: No matching file name found
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "com.acme.Hardware.Chassis.Model.MegaServer";
        writeConfigFile(configFilePath, std::string{""});

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_TRUE(pathFound.empty());
    }

    // Test where fails: Matching file name is a directory: Fully qualified
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "com.acme.Hardware.Chassis.Model.MegaServer4CPU.json";
        fs::create_directory(configFilePath);

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_TRUE(pathFound.empty());
    }

    // Test where fails: Matching file name is a directory: Last node
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "MegaServer.json";
        fs::create_directory(configFilePath);

        fs::path pathFound = find(compatibleSystemTypes, configFileDirPath);
        EXPECT_TRUE(pathFound.empty());
    }

    // Test where fails: System type has no '.'
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "MegaServer2CPU.json";
        writeConfigFile(configFilePath, std::string{""});

        std::vector<std::string> noDotSystemTypes{"MegaServer4CPU",
                                                  "MegaServer", "Server", ""};
        fs::path pathFound = find(noDotSystemTypes, configFileDirPath);
        EXPECT_TRUE(pathFound.empty());
    }

    // Test where fails: System type ends with '.'
    {
        TemporarySubDirectory configFileDir;
        fs::path configFileDirPath = configFileDir.getPath();

        fs::path configFilePath = configFileDirPath;
        configFilePath /= "MegaServer4CPU.json";
        writeConfigFile(configFilePath, std::string{""});

        std::vector<std::string> dotAtEndSystemTypes{
            "com.acme.Hardware.Chassis.Model.MegaServer4CPU.", "a.", "."};
        fs::path pathFound = find(dotAtEndSystemTypes, configFileDirPath);
        EXPECT_TRUE(pathFound.empty());
    }
}

TEST(ConfigFileParserTests, Parse)
{
    // Test where works
    {
        const json configFileContents = R"(
            {
              "chassis": [
                {
                  "number": 1,
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis1",
                  "power_sequencers": []
                },
                {
                  "number": 2,
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis2",
                  "power_sequencers": []
                }
              ]
            }
        )"_json;

        TemporaryFile configFile;
        fs::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        MockServices services{};
        auto chassis = parse(pathName, services);

        EXPECT_EQ(chassis.size(), 2);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[0]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis1");
        EXPECT_EQ(chassis[1]->getNumber(), 2);
        EXPECT_EQ(chassis[1]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
    }

    // Test where fails: File does not exist
    {
        fs::path pathName{"/tmp/non_existent_file"};
        MockServices services{};
        EXPECT_THROW(parse(pathName, services), ConfigFileParserError);
    }

    // Test where fails: File is not readable
    {
        const json configFileContents = R"(
            {
              "chassis": [
                {
                  "number": 1,
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis1",
                  "power_sequencers": []
                }
              ]
            }
        )"_json;

        TemporaryFile configFile;
        fs::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        chmod(pathName.c_str(), 0222);
        MockServices services{};
        EXPECT_THROW(parse(pathName, services), ConfigFileParserError);
    }

    // Test where fails: File is not valid JSON
    {
        const std::string configFileContents = "] foo [";

        TemporaryFile configFile;
        fs::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        MockServices services{};
        EXPECT_THROW(parse(pathName, services), ConfigFileParserError);
    }

    // Test where fails: JSON does not conform to config file format
    {
        const json configFileContents = R"( [ "foo", "bar" ] )"_json;

        TemporaryFile configFile;
        fs::path pathName{configFile.getPath()};
        writeConfigFile(pathName, configFileContents);

        MockServices services{};
        EXPECT_THROW(parse(pathName, services), ConfigFileParserError);
    }
}

TEST(ConfigFileParserTests, ParseChassis)
{
    // Constants used by multiple tests
    const json templateElement = R"(
        {
          "id": "foo_chassis",
          "number": "${chassis_number}",
          "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
          "power_sequencers": [
            {
              "type": "UCD90320",
              "i2c_interface": { "bus": "${bus}", "address": "${address}" },
              "power_control_gpio_name": "power-chassis${chassis_number}-control",
              "power_good_gpio_name": "power-chassis${chassis_number}-good",
              "rails": []
            }
          ]
        }
    )"_json;
    const std::map<std::string, JSONRefWrapper> chassisTemplates{
        {"foo_chassis", templateElement}};

    // Test where works: Does not use a template
    {
        const json element = R"(
            {
              "number": 1,
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
              "power_sequencers": [
                {
                  "type": "UCD90320",
                  "i2c_interface": { "bus": 3, "address": "0x11" },
                  "power_control_gpio_name": "power-chassis-control",
                  "power_good_gpio_name": "power-chassis-good",
                  "rails": []
                }
              ]
            }
        )"_json;
        MockServices services{};
        auto chassis = parseChassis(element, chassisTemplates, services);
        EXPECT_EQ(chassis->getNumber(), 1);
        EXPECT_EQ(chassis->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis");
        EXPECT_EQ(chassis->getPowerSequencers().size(), 1);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getName(), "UCD90320");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getBus(), 3);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getAddress(), 0x11);
    }

    // Test where works: Uses template: No comments specified
    {
        const json element = R"(
            {
              "template_id": "foo_chassis",
              "template_variable_values": {
                "chassis_number": "2",
                "bus": "13",
                "address": "0x70"
              }
            }
        )"_json;
        MockServices services{};
        auto chassis = parseChassis(element, chassisTemplates, services);
        EXPECT_EQ(chassis->getNumber(), 2);
        EXPECT_EQ(chassis->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
        EXPECT_EQ(chassis->getPowerSequencers().size(), 1);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getName(), "UCD90320");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getBus(), 13);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getAddress(), 0x70);
    }

    // Test where works: Uses template: Comments specified
    {
        const json element = R"(
            {
              "comments": ["Chassis 3: Standard hardware layout"],
              "template_id": "foo_chassis",
              "template_variable_values": {
                "chassis_number": "3",
                "bus": "23",
                "address": "0x54"
              }
            }
        )"_json;
        MockServices services{};
        auto chassis = parseChassis(element, chassisTemplates, services);
        EXPECT_EQ(chassis->getNumber(), 3);
        EXPECT_EQ(chassis->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis3");
        EXPECT_EQ(chassis->getPowerSequencers().size(), 1);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getName(), "UCD90320");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getBus(), 23);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getAddress(), 0x54);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "vdda", "vddb" ] )"_json;
        MockServices services{};
        parseChassis(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Does not use a template: Cannot parse properties
    try
    {
        const json element = R"(
            {
              "number": "one",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
              "power_sequencers": []
            }
        )"_json;
        MockServices services{};
        parseChassis(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Uses template: Required template_variable_values
    // property not specified
    try
    {
        const json element = R"(
            {
              "template_id": "foo_chassis"
            }
        )"_json;
        MockServices services{};
        parseChassis(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(),
                     "Required property missing: template_variable_values");
    }

    // Test where fails: Uses template: template_id value is invalid: Not a
    // string
    try
    {
        const json element = R"(
            {
              "template_id": 23,
              "template_variable_values": { "chassis_number": "2" }
            }
        )"_json;
        MockServices services{};
        parseChassis(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Uses template: template_id value is invalid: No
    // matching template
    try
    {
        const json element = R"(
            {
              "template_id": "does_not_exist",
              "template_variable_values": { "chassis_number": "2" }
            }
        )"_json;
        MockServices services{};
        parseChassis(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid chassis template id: does_not_exist");
    }

    // Test where fails: Uses template: template_variable_values value is
    // invalid
    try
    {
        const json element = R"(
            {
              "template_id": "foo_chassis",
              "template_variable_values": { "chassis_number": 2 }
            }
        )"_json;
        MockServices services{};
        parseChassis(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Uses template: Invalid property specified
    try
    {
        const json element = R"(
            {
              "template_id": "foo_chassis",
              "template_variable_values": { "chassis_number": "2" },
              "foo": "bar"
            }
        )"_json;
        MockServices services{};
        parseChassis(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Uses template: Cannot parse properties in template
    try
    {
        const json element = R"(
            {
              "template_id": "foo_chassis",
              "template_variable_values": { "chassis_number": "0" }
            }
        )"_json;
        MockServices services{};
        parseChassis(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid chassis number: Must be > 0");
    }
}

TEST(ConfigFileParserTests, ParseChassisArray)
{
    // Constants used by multiple tests
    const json fooTemplateElement = R"(
        {
          "id": "foo_chassis",
          "number": "${chassis_number}",
          "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
          "power_sequencers": []
        }
    )"_json;
    const json barTemplateElement = R"(
        {
          "id": "bar_chassis",
          "number": "${chassis_number}",
          "inventory_path": "/xyz/openbmc_project/inventory/system/bar_chassis${chassis_number}",
          "power_sequencers": []
        }
    )"_json;
    const std::map<std::string, JSONRefWrapper> chassisTemplates{
        {"foo_chassis", fooTemplateElement},
        {"bar_chassis", barTemplateElement}};

    // Test where works: Array is empty
    {
        const json element = R"(
            [
            ]
        )"_json;
        MockServices services{};
        auto chassis = parseChassisArray(element, chassisTemplates, services);
        EXPECT_EQ(chassis.size(), 0);
    }

    // Test where works: Template not used
    {
        const json element = R"(
            [
              {
                "number": 1,
                "inventory_path": "/xyz/openbmc_project/inventory/system/chassis1",
                "power_sequencers": []
              },
              {
                "number": 2,
                "inventory_path": "/xyz/openbmc_project/inventory/system/chassis2",
                "power_sequencers": []
              }
            ]
        )"_json;
        MockServices services{};
        auto chassis = parseChassisArray(element, chassisTemplates, services);
        EXPECT_EQ(chassis.size(), 2);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[0]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis1");
        EXPECT_EQ(chassis[1]->getNumber(), 2);
        EXPECT_EQ(chassis[1]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
    }

    // Test where works: Template used
    {
        const json element = R"(
            [
              {
                "template_id": "foo_chassis",
                "template_variable_values": { "chassis_number": "2" }
              },
              {
                "template_id": "bar_chassis",
                "template_variable_values": { "chassis_number": "3" }
              }
            ]
        )"_json;
        MockServices services{};
        auto chassis = parseChassisArray(element, chassisTemplates, services);
        EXPECT_EQ(chassis.size(), 2);
        EXPECT_EQ(chassis[0]->getNumber(), 2);
        EXPECT_EQ(chassis[0]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
        EXPECT_EQ(chassis[1]->getNumber(), 3);
        EXPECT_EQ(chassis[1]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/bar_chassis3");
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
                "foo": "bar"
            }
        )"_json;
        MockServices services{};
        parseChassisArray(element, chassisTemplates, services);
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
              {
                "number": true,
                "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
                "power_sequencers": []
              }
            ]
        )"_json;
        MockServices services{};
        parseChassisArray(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Invalid variable value specified
    try
    {
        const json element = R"(
            [
                {
                  "template_id": "foo_chassis",
                  "template_variable_values": { "chassis_number": "two" }
                }
            ]
        )"_json;
        MockServices services{};
        parseChassisArray(element, chassisTemplates, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }
}

TEST(ConfigFileParserTests, ParseChassisProperties)
{
    // Test where works: Parse chassis object without template/variables: Has
    // comments property
    {
        const json element = R"(
            {
              "comments": [ "Chassis 1: Has all CPUs, fans, and PSUs" ],
              "number": 1,
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
              "power_sequencers": [
                {
                  "type": "UCD90160",
                  "i2c_interface": { "bus": 3, "address": "0x11" },
                  "power_control_gpio_name": "power-chassis-control",
                  "power_good_gpio_name": "power-chassis-good",
                  "rails": [ { "name": "VDD_CPU0" }, { "name": "VCS_CPU1" } ]
                }
              ]
            }
        )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        auto chassis = parseChassisProperties(element, isChassisTemplate,
                                              variables, services);
        EXPECT_EQ(chassis->getNumber(), 1);
        EXPECT_EQ(chassis->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis");
        EXPECT_EQ(chassis->getPowerSequencers().size(), 1);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getName(), "UCD90160");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getBus(), 3);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getAddress(), 0x11);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getPowerControlGPIOName(),
                  "power-chassis-control");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getPowerGoodGPIOName(),
                  "power-chassis-good");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getRails().size(), 2);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getRails()[0]->getName(),
                  "VDD_CPU0");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getRails()[1]->getName(),
                  "VCS_CPU1");
    }

    // Test where works: Parse chassis_template object with variables: No
    // comments property
    {
        const json element = R"(
            {
              "id": "foo_chassis",
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": [
                {
                  "type": "UCD90320",
                  "i2c_interface": { "bus": "${bus}", "address": "${address}" },
                  "power_control_gpio_name": "power-chassis${chassis_number}-control",
                  "power_good_gpio_name": "power-chassis${chassis_number}-good",
                  "rails": [ { "name": "vio${chassis_number}" } ]
                }
              ]
            }
        )"_json;
        bool isChassisTemplate{true};
        std::map<std::string, std::string> variables{
            {"chassis_number", "2"}, {"bus", "12"}, {"address", "0x71"}};
        MockServices services{};
        auto chassis = parseChassisProperties(element, isChassisTemplate,
                                              variables, services);
        EXPECT_EQ(chassis->getNumber(), 2);
        EXPECT_EQ(chassis->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
        EXPECT_EQ(chassis->getPowerSequencers().size(), 1);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getName(), "UCD90320");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getBus(), 12);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getAddress(), 0x71);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getPowerControlGPIOName(),
                  "power-chassis2-control");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getPowerGoodGPIOName(),
                  "power-chassis2-good");
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getRails().size(), 1);
        EXPECT_EQ(chassis->getPowerSequencers()[0]->getRails()[0]->getName(),
                  "vio2");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( true )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required id property not specified in chassis template
    try
    {
        const json element = R"(
            {
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": []
            }
        )"_json;
        bool isChassisTemplate{true};
        std::map<std::string, std::string> variables{{"chassis_number", "2"}};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: id");
    }

    // Test where fails: Required number property not specified
    try
    {
        const json element = R"(
            {
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
              "power_sequencers": []
            }
        )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
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
              "id": "foo_chassis",
              "number": "${chassis_number}",
              "power_sequencers": []
            }
        )"_json;
        bool isChassisTemplate{true};
        std::map<std::string, std::string> variables{{"chassis_number", "2"}};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: inventory_path");
    }

    // Test where fails: Required power_sequencers property not specified
    try
    {
        const json element = R"(
            {
              "number": 1,
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis"
            }
        )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: power_sequencers");
    }

    // Test where fails: number value is invalid: Not an integer
    try
    {
        const json element = R"(
            {
              "number": "two",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
              "power_sequencers": []
            }
        )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: number value is invalid: Equal to 0
    try
    {
        const json element = R"(
            {
              "number": 0,
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
              "power_sequencers": []
            }
        )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid chassis number: Must be > 0");
    }

    // Test where fails: inventory_path value is invalid
    try
    {
        const json element = R"(
            {
              "number": 1,
              "inventory_path": "",
              "power_sequencers": []
            }
        )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }

    // Test where fails: power_sequencers value is invalid
    try
    {
        const json element = R"(
            {
              "number": 1,
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
              "power_sequencers": { "name": "foo" }
            }
        )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
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
              "foo": "bar",
              "number": 1,
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
              "power_sequencers": []
            }
        )"_json;
        bool isChassisTemplate{false};
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Invalid variable value specified
    try
    {
        const json element = R"(
            {
              "id": "foo_chassis",
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": []
            }
        )"_json;
        bool isChassisTemplate{true};
        std::map<std::string, std::string> variables{{"chassis_number", "two"}};
        MockServices services{};
        parseChassisProperties(element, isChassisTemplate, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }
}

TEST(ConfigFileParserTests, ParseChassisTemplate)
{
    // Test where works: comments specified
    {
        const json element = R"(
            {
              "comments": [ "This is a template for the foo chassis type",
                            "Chassis contains a UCD90320 power sequencer" ],
              "id": "foo_chassis",
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": [
                {
                  "type": "UCD90320",
                  "i2c_interface": { "bus": "${bus}", "address": "0x11" },
                  "power_control_gpio_name": "power-chassis${chassis_number}-control",
                  "power_good_gpio_name": "power-chassis${chassis_number}-good",
                  "rails": [ { "name": "VDD_CPU0" }, { "name": "VCS_CPU1" } ]
                }
              ]
            }
        )"_json;
        auto [id, jsonRef] = parseChassisTemplate(element);
        EXPECT_EQ(id, "foo_chassis");
        EXPECT_EQ(jsonRef.get()["number"], "${chassis_number}");
        EXPECT_EQ(jsonRef.get()["power_sequencers"].size(), 1);
        EXPECT_EQ(jsonRef.get()["power_sequencers"][0]["type"], "UCD90320");
    }

    // Test where works: comments not specified
    {
        const json element = R"(
            {
              "id": "foo_chassis",
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": []
            }
        )"_json;
        auto [id, jsonRef] = parseChassisTemplate(element);
        EXPECT_EQ(id, "foo_chassis");
        EXPECT_EQ(jsonRef.get()["number"], "${chassis_number}");
        EXPECT_EQ(
            jsonRef.get()["inventory_path"],
            "/xyz/openbmc_project/inventory/system/chassis${chassis_number}");
        EXPECT_EQ(jsonRef.get()["power_sequencers"].size(), 0);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "vdda", "vddb" ] )"_json;
        parseChassisTemplate(element);
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
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": []
            }
        )"_json;
        parseChassisTemplate(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: id");
    }

    // Test where fails: Required number property not specified
    try
    {
        const json element = R"(
            {
              "id": "foo_chassis",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": []
            }
        )"_json;
        parseChassisTemplate(element);
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
              "id": "foo_chassis",
              "number": "${chassis_number}",
              "power_sequencers": []
            }
        )"_json;
        parseChassisTemplate(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: inventory_path");
    }

    // Test where fails: Required power_sequencers property not specified
    try
    {
        const json element = R"(
            {
              "id": "foo_chassis",
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}"
            }
        )"_json;
        parseChassisTemplate(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: power_sequencers");
    }

    // Test where fails: id value is invalid
    try
    {
        const json element = R"(
            {
              "id": 13,
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": []
            }
        )"_json;
        parseChassisTemplate(element);
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
              "id": "foo_chassis",
              "number": "${chassis_number}",
              "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
              "power_sequencers": [],
              "foo": "bar"
            }
        )"_json;
        parseChassisTemplate(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseChassisTemplateArray)
{
    // Test where works: Array is empty
    {
        const json element = R"(
            [
            ]
        )"_json;
        auto chassisTemplates = parseChassisTemplateArray(element);
        EXPECT_EQ(chassisTemplates.size(), 0);
    }

    // Test where works: Array is not empty
    {
        const json element = R"(
            [
              {
                "id": "foo_chassis",
                "number": "${chassis_number}",
                "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
                "power_sequencers": []
              },
              {
                "id": "bar_chassis",
                "number": "${number}",
                "inventory_path": "/xyz/openbmc_project/inventory/system/bar_chassis${number}",
                "power_sequencers": []
              }
            ]
        )"_json;
        auto chassisTemplates = parseChassisTemplateArray(element);
        EXPECT_EQ(chassisTemplates.size(), 2);
        EXPECT_EQ(chassisTemplates.at("foo_chassis").get()["number"],
                  "${chassis_number}");
        EXPECT_EQ(
            chassisTemplates.at("foo_chassis").get()["inventory_path"],
            "/xyz/openbmc_project/inventory/system/chassis${chassis_number}");
        EXPECT_EQ(chassisTemplates.at("bar_chassis").get()["number"],
                  "${number}");
        EXPECT_EQ(chassisTemplates.at("bar_chassis").get()["inventory_path"],
                  "/xyz/openbmc_project/inventory/system/bar_chassis${number}");
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
                "foo": "bar"
            }
        )"_json;
        parseChassisTemplateArray(element);
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
              {
                "id": "foo_chassis",
                "number": "${chassis_number}",
                "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
                "power_sequencers": []
              },
              {
                "id": "bar_chassis"
              }
            ]
        )"_json;
        parseChassisTemplateArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: number");
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
        std::map<std::string, std::string> variables{};
        auto gpio = parseGPIO(element, variables);
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
        std::map<std::string, std::string> variables{};
        auto gpio = parseGPIO(element, variables);
        EXPECT_EQ(gpio.line, 131);
        EXPECT_TRUE(gpio.activeLow);
    }

    // Test where works: Variables specified
    {
        const json element = R"(
            {
                "line": "${line}",
                "active_low": "${active_low}"
            }
        )"_json;
        std::map<std::string, std::string> variables{{"line", "54"},
                                                     {"active_low", "false"}};
        auto gpio = parseGPIO(element, variables);
        EXPECT_EQ(gpio.line, 54);
        EXPECT_FALSE(gpio.activeLow);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "vdda", "vddb" ] )"_json;
        std::map<std::string, std::string> variables{};
        parseGPIO(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseGPIO(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseGPIO(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseGPIO(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseGPIO(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Invalid variable value specified
    try
    {
        const json element = R"(
            {
                "line": "${line}",
                "active_low": "${active_low}"
            }
        )"_json;
        std::map<std::string, std::string> variables{{"line", "-1"},
                                                     {"active_low", "false"}};
        parseGPIO(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an unsigned integer");
    }
}

TEST(ConfigFileParserTests, ParseI2CInterface)
{
    // Test where works: No variables
    {
        const json element = R"(
            {
                "bus": 2,
                "address": "0x70"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        auto [bus, address] = parseI2CInterface(element, variables);
        EXPECT_EQ(bus, 2);
        EXPECT_EQ(address, 0x70);
    }

    // Test where works: Variables specified
    {
        const json element = R"(
            {
                "bus": "${bus}",
                "address": "${address}"
            }
        )"_json;
        std::map<std::string, std::string> variables{{"bus", "3"},
                                                     {"address", "0x23"}};
        auto [bus, address] = parseI2CInterface(element, variables);
        EXPECT_EQ(bus, 3);
        EXPECT_EQ(address, 0x23);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ 1, "0x70" ] )"_json;
        std::map<std::string, std::string> variables{};
        parseI2CInterface(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required bus property not specified
    try
    {
        const json element = R"(
            {
                "address": "0x70"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseI2CInterface(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: bus");
    }

    // Test where fails: Required address property not specified
    try
    {
        const json element = R"(
            {
                "bus": 2
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseI2CInterface(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: address");
    }

    // Test where fails: bus value is invalid
    try
    {
        const json element = R"(
            {
                "bus": 1.1,
                "address": "0x70"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseI2CInterface(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: address value is invalid
    try
    {
        const json element = R"(
            {
                "bus": 2,
                "address": 70
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseI2CInterface(element, variables);
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
                "bus": 2,
                "address": "0x70",
                "foo": "bar"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseI2CInterface(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Invalid variable value specified
    try
    {
        const json element = R"(
            {
                "bus": "${bus}",
                "address": "${address}"
            }
        )"_json;
        std::map<std::string, std::string> variables{{"bus", "foo"},
                                                     {"address", "0x23"}};
        parseI2CInterface(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }
}

TEST(ConfigFileParserTests, ParsePowerSequencer)
{
    // Test where works: Has comments property: Type is "UCD90160"
    {
        const json element = R"(
            {
              "comments": [ "Power sequencer in chassis 1",
                            "Controls VDD rails" ],
              "type": "UCD90160",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "rails": [ { "name": "VDD_CPU0" }, { "name": "VCS_CPU1" } ]
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        auto powerSequencer = parsePowerSequencer(element, variables, services);
        EXPECT_EQ(powerSequencer->getName(), "UCD90160");
        EXPECT_EQ(powerSequencer->getBus(), 3);
        EXPECT_EQ(powerSequencer->getAddress(), 0x11);
        EXPECT_EQ(powerSequencer->getPowerControlGPIOName(),
                  "power-chassis-control");
        EXPECT_EQ(powerSequencer->getPowerGoodGPIOName(), "power-chassis-good");
        EXPECT_EQ(powerSequencer->getRails().size(), 2);
        EXPECT_EQ(powerSequencer->getRails()[0]->getName(), "VDD_CPU0");
        EXPECT_EQ(powerSequencer->getRails()[1]->getName(), "VCS_CPU1");
    }

    // Test where works: No comments property: Variables specified: Type is
    // "UCD90320"
    {
        const json element = R"(
            {
              "type": "${type}",
              "i2c_interface": { "bus": "${bus}", "address": "${address}" },
              "power_control_gpio_name": "${power_control_gpio_name}",
              "power_good_gpio_name": "${power_good_gpio_name}",
              "rails": [ { "name": "${rail1}" }, { "name": "${rail2}" } ]
            }
        )"_json;
        std::map<std::string, std::string> variables{
            {"type", "UCD90320"},
            {"bus", "4"},
            {"address", "0x24"},
            {"power_control_gpio_name", "power_on"},
            {"power_good_gpio_name", "pgood"},
            {"rail1", "cpu1"},
            {"rail2", "cpu2"}};
        MockServices services{};
        auto powerSequencer = parsePowerSequencer(element, variables, services);
        EXPECT_EQ(powerSequencer->getName(), "UCD90320");
        EXPECT_EQ(powerSequencer->getBus(), 4);
        EXPECT_EQ(powerSequencer->getAddress(), 0x24);
        EXPECT_EQ(powerSequencer->getPowerControlGPIOName(), "power_on");
        EXPECT_EQ(powerSequencer->getPowerGoodGPIOName(), "pgood");
        EXPECT_EQ(powerSequencer->getRails().size(), 2);
        EXPECT_EQ(powerSequencer->getRails()[0]->getName(), "cpu1");
        EXPECT_EQ(powerSequencer->getRails()[1]->getName(), "cpu2");
    }

    // Test where works: Type is "gpios_only_device"
    {
        const json element = R"(
            {
              "type": "gpios_only_device",
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        auto powerSequencer = parsePowerSequencer(element, variables, services);
        EXPECT_EQ(powerSequencer->getName(), "gpios_only_device");
        EXPECT_EQ(powerSequencer->getBus(), 0);
        EXPECT_EQ(powerSequencer->getAddress(), 0);
        EXPECT_EQ(powerSequencer->getPowerControlGPIOName(),
                  "power-chassis-control");
        EXPECT_EQ(powerSequencer->getPowerGoodGPIOName(), "power-chassis-good");
        EXPECT_EQ(powerSequencer->getRails().size(), 0);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "vdda", "vddb" ] )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
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
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: type");
    }

    // Test where fails: Required i2c_interface property not specified
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: i2c_interface");
    }

    // Test where fails: Required power_control_gpio_name property not specified
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_good_gpio_name": "power-chassis-good",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(),
                     "Required property missing: power_control_gpio_name");
    }

    // Test where fails: Required power_good_gpio_name property not specified
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(),
                     "Required property missing: power_good_gpio_name");
    }

    // Test where fails: Required rails property not specified
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: rails");
    }

    // Test where fails: type value is invalid: Not a string
    try
    {
        const json element = R"(
            {
              "type": true,
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: type value is invalid: Not a supported type
    try
    {
        const json element = R"(
            {
              "type": "foo_bar",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid power sequencer type: foo_bar");
    }

    // Test where fails: i2c_interface value is invalid
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "i2c_interface": 3,
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: power_control_gpio_name value is invalid
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": [],
              "power_good_gpio_name": "power-chassis-good",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: power_good_gpio_name value is invalid
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": 12,
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: rails value is invalid
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "rails": [ { "name": 33 } ]
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
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
              "type": "UCD90320",
              "i2c_interface": { "bus": 3, "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "driver_name": "foo",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Invalid variable value specified
    try
    {
        const json element = R"(
            {
              "type": "UCD90320",
              "i2c_interface": { "bus": "${bus}", "address": "0x11" },
              "power_control_gpio_name": "power-chassis-control",
              "power_good_gpio_name": "power-chassis-good",
              "rails": []
            }
        )"_json;
        std::map<std::string, std::string> variables{{"bus", "two"}};
        MockServices services{};
        parsePowerSequencer(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }
}

TEST(ConfigFileParserTests, ParsePowerSequencerArray)
{
    // Test where works: Array is empty
    {
        const json element = R"(
            [
            ]
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        auto powerSequencers =
            parsePowerSequencerArray(element, variables, services);
        EXPECT_EQ(powerSequencers.size(), 0);
    }

    // Test where works: Array is not empty
    {
        const json element = R"(
            [
              {
                "type": "UCD90160",
                "i2c_interface": { "bus": 3, "address": "0x11" },
                "power_control_gpio_name": "power-chassis-control1",
                "power_good_gpio_name": "power-chassis-good1",
                "rails": []
              },
              {
                "type": "UCD90320",
                "i2c_interface": { "bus": 4, "address": "0x70" },
                "power_control_gpio_name": "power-chassis-control2",
                "power_good_gpio_name": "power-chassis-good2",
                "rails": []
              }
            ]
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        auto powerSequencers =
            parsePowerSequencerArray(element, variables, services);
        EXPECT_EQ(powerSequencers.size(), 2);
        EXPECT_EQ(powerSequencers[0]->getName(), "UCD90160");
        EXPECT_EQ(powerSequencers[0]->getBus(), 3);
        EXPECT_EQ(powerSequencers[0]->getAddress(), 0x11);
        EXPECT_EQ(powerSequencers[1]->getName(), "UCD90320");
        EXPECT_EQ(powerSequencers[1]->getBus(), 4);
        EXPECT_EQ(powerSequencers[1]->getAddress(), 0x70);
    }

    // Test where works: Variables specified
    {
        const json element = R"(
            [
              {
                "type": "UCD90160",
                "i2c_interface": { "bus": "${bus1}", "address": "${address1}" },
                "power_control_gpio_name": "power-chassis-control1",
                "power_good_gpio_name": "power-chassis-good1",
                "rails": []
              },
              {
                "type": "UCD90320",
                "i2c_interface": { "bus": "${bus2}", "address": "${address2}" },
                "power_control_gpio_name": "power-chassis-control2",
                "power_good_gpio_name": "power-chassis-good2",
                "rails": []
              }
            ]
        )"_json;
        std::map<std::string, std::string> variables{
            {"bus1", "5"},
            {"address1", "0x22"},
            {"bus2", "7"},
            {"address2", "0x49"}};
        MockServices services{};
        auto powerSequencers =
            parsePowerSequencerArray(element, variables, services);
        EXPECT_EQ(powerSequencers.size(), 2);
        EXPECT_EQ(powerSequencers[0]->getName(), "UCD90160");
        EXPECT_EQ(powerSequencers[0]->getBus(), 5);
        EXPECT_EQ(powerSequencers[0]->getAddress(), 0x22);
        EXPECT_EQ(powerSequencers[1]->getName(), "UCD90320");
        EXPECT_EQ(powerSequencers[1]->getBus(), 7);
        EXPECT_EQ(powerSequencers[1]->getAddress(), 0x49);
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
                "foo": "bar"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencerArray(element, variables, services);
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
              {
                "type": "UCD90160",
                "i2c_interface": { "bus": 3, "address": "0x11" },
                "power_control_gpio_name": "power-chassis-control1",
                "power_good_gpio_name": "power-chassis-good1",
                "rails": []
              },
              true
            ]
        )"_json;
        std::map<std::string, std::string> variables{};
        MockServices services{};
        parsePowerSequencerArray(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid variable value specified
    try
    {
        const json element = R"(
            [
              {
                "type": "UCD90320",
                "i2c_interface": { "bus": "${bus}", "address": "${address}" },
                "power_control_gpio_name": "power-chassis-control",
                "power_good_gpio_name": "power-chassis-good",
                "rails": []
              }
            ]
        )"_json;
        std::map<std::string, std::string> variables{{"bus", "7"},
                                                     {"address", "70"}};
        MockServices services{};
        parsePowerSequencerArray(element, variables, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
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
        std::map<std::string, std::string> variables{};
        auto rail = parseRail(element, variables);
        EXPECT_EQ(rail->getName(), "VDD_CPU0");
        EXPECT_FALSE(rail->getPresence().has_value());
        EXPECT_FALSE(rail->getPage().has_value());
        EXPECT_FALSE(rail->isPowerSupplyRail());
        EXPECT_FALSE(rail->getCheckStatusVout());
        EXPECT_FALSE(rail->getCompareVoltageToLimit());
        EXPECT_FALSE(rail->getGPIO().has_value());
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
                "name": "12.0VB",
                "presence": "/xyz/openbmc_project/inventory/system/chassis/powersupply1",
                "page": 11,
                "is_power_supply_rail": true,
                "check_status_vout": true,
                "compare_voltage_to_limit": true,
                "gpio": { "line": 60, "active_low": true }
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        auto rail = parseRail(element, variables);
        EXPECT_EQ(rail->getName(), "12.0VB");
        EXPECT_TRUE(rail->getPresence().has_value());
        EXPECT_EQ(rail->getPresence().value(),
                  "/xyz/openbmc_project/inventory/system/chassis/powersupply1");
        EXPECT_TRUE(rail->getPage().has_value());
        EXPECT_EQ(rail->getPage().value(), 11);
        EXPECT_TRUE(rail->isPowerSupplyRail());
        EXPECT_TRUE(rail->getCheckStatusVout());
        EXPECT_TRUE(rail->getCompareVoltageToLimit());
        EXPECT_TRUE(rail->getGPIO().has_value());
        EXPECT_EQ(rail->getGPIO().value().line, 60);
        EXPECT_TRUE(rail->getGPIO().value().activeLow);
    }

    // Test where works: Variables specified
    {
        const json element = R"(
            {
                "name": "${name}",
                "presence": "${presence}",
                "page": "${page}",
                "is_power_supply_rail": "${is_power_supply_rail}",
                "check_status_vout": "${check_status_vout}",
                "compare_voltage_to_limit": "${compare_voltage_to_limit}",
                "gpio": { "line": "${line}", "active_low": true }
            }
        )"_json;
        std::map<std::string, std::string> variables{
            {"name", "vdd"},
            {"presence",
             "/xyz/openbmc_project/inventory/system/chassis/powersupply2"},
            {"page", "9"},
            {"is_power_supply_rail", "true"},
            {"check_status_vout", "false"},
            {"compare_voltage_to_limit", "true"},
            {"line", "72"}};
        auto rail = parseRail(element, variables);
        EXPECT_EQ(rail->getName(), "vdd");
        EXPECT_TRUE(rail->getPresence().has_value());
        EXPECT_EQ(rail->getPresence().value(),
                  "/xyz/openbmc_project/inventory/system/chassis/powersupply2");
        EXPECT_TRUE(rail->getPage().has_value());
        EXPECT_EQ(rail->getPage().value(), 9);
        EXPECT_TRUE(rail->isPowerSupplyRail());
        EXPECT_FALSE(rail->getCheckStatusVout());
        EXPECT_TRUE(rail->getCompareVoltageToLimit());
        EXPECT_TRUE(rail->getGPIO().has_value());
        EXPECT_EQ(rail->getGPIO().value().line, 72);
        EXPECT_TRUE(rail->getGPIO().value().activeLow);
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "vdda", "vddb" ] )"_json;
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit unsigned integer");
    }

    // Test where fails: is_power_supply_rail value is invalid
    try
    {
        const json element = R"(
            {
                "name": "12.0VA",
                "is_power_supply_rail": "true"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
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
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }

    // Test where fails: compare_voltage_to_limit value is invalid
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "compare_voltage_to_limit": 23
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: page");
    }

    // Test where fails: compare_voltage_to_limit is true and page not
    // specified
    try
    {
        const json element = R"(
            {
                "name": "VCS_CPU1",
                "compare_voltage_to_limit": true
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseRail(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }

    // Test where fails: Undefined variable specified
    try
    {
        const json element = R"(
            {
                "name": "12.0VB",
                "presence": "/xyz/openbmc_project/inventory/system/chassis/powersupply${chassis}"
            }
        )"_json;
        std::map<std::string, std::string> variables{{"foo", "bar"}};
        parseRail(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Undefined variable: chassis");
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
        std::map<std::string, std::string> variables{};
        auto rails = parseRailArray(element, variables);
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
        std::map<std::string, std::string> variables{};
        auto rails = parseRailArray(element, variables);
        EXPECT_EQ(rails.size(), 2);
        EXPECT_EQ(rails[0]->getName(), "VDD_CPU0");
        EXPECT_EQ(rails[1]->getName(), "VCS_CPU1");
    }

    // Test where works: Variables specified
    {
        const json element = R"(
            [
                { "name": "${rail1}" },
                { "name": "${rail2}" },
                { "name": "${rail3}" }
            ]
        )"_json;
        std::map<std::string, std::string> variables{
            {"rail1", "foo"}, {"rail2", "bar"}, {"rail3", "baz"}};
        auto rails = parseRailArray(element, variables);
        EXPECT_EQ(rails.size(), 3);
        EXPECT_EQ(rails[0]->getName(), "foo");
        EXPECT_EQ(rails[1]->getName(), "bar");
        EXPECT_EQ(rails[2]->getName(), "baz");
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = R"(
            {
                "foo": "bar"
            }
        )"_json;
        std::map<std::string, std::string> variables{};
        parseRailArray(element, variables);
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
        std::map<std::string, std::string> variables{};
        parseRailArray(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Invalid variable value specified
    try
    {
        const json element = R"(
            [
                { "name": "VDD_CPU0", "page": "${page1}" },
                { "name": "VCS_CPU1", "page": "${page2}" }
            ]
        )"_json;
        std::map<std::string, std::string> variables{{"page1", "11"},
                                                     {"page2", "-1"}};
        parseRailArray(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit unsigned integer");
    }
}

TEST(ConfigFileParserTests, ParseRoot)
{
    // Test where works: Only required properties specified
    {
        const json element = R"(
            {
              "chassis": [
                {
                  "number": 1,
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis1",
                  "power_sequencers": []
                },
                {
                  "number": 2,
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis2",
                  "power_sequencers": []
                }
              ]
            }
        )"_json;
        MockServices services{};
        auto chassis = parseRoot(element, services);
        EXPECT_EQ(chassis.size(), 2);
        EXPECT_EQ(chassis[0]->getNumber(), 1);
        EXPECT_EQ(chassis[0]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis1");
        EXPECT_EQ(chassis[1]->getNumber(), 2);
        EXPECT_EQ(chassis[1]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
    }

    // Test where works: All properties specified
    {
        const json element = R"(
            {
              "comments": [ "Config file for a FooBar one-chassis system" ],
              "chassis_templates": [
                {
                  "id": "foo_chassis",
                  "number": "${chassis_number}",
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
                  "power_sequencers": []
                },
                {
                  "id": "bar_chassis",
                  "number": "${chassis_number}",
                  "inventory_path": "/xyz/openbmc_project/inventory/system/bar_chassis${chassis_number}",
                  "power_sequencers": []
                }
              ],
              "chassis": [
                {
                  "template_id": "foo_chassis",
                  "template_variable_values": { "chassis_number": "2" }
                },
                {
                  "template_id": "bar_chassis",
                  "template_variable_values": { "chassis_number": "3" }
                }
              ]
            }
        )"_json;
        MockServices services{};
        auto chassis = parseRoot(element, services);
        EXPECT_EQ(chassis.size(), 2);
        EXPECT_EQ(chassis[0]->getNumber(), 2);
        EXPECT_EQ(chassis[0]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/chassis2");
        EXPECT_EQ(chassis[1]->getNumber(), 3);
        EXPECT_EQ(chassis[1]->getInventoryPath(),
                  "/xyz/openbmc_project/inventory/system/bar_chassis3");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"( [ "VDD_CPU0", "VCS_CPU1" ] )"_json;
        MockServices services{};
        parseRoot(element, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Required chassis property not specified
    try
    {
        const json element = R"(
            {
              "comments": [ "Config file for a FooBar one-chassis system" ],
              "chassis_templates": [
                {
                  "id": "foo_chassis",
                  "number": "${chassis_number}",
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
                  "power_sequencers": []
                }
              ]
            }
        )"_json;
        MockServices services{};
        parseRoot(element, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: chassis");
    }

    // Test where fails: chassis_templates value is invalid
    try
    {
        const json element = R"(
            {
              "chassis_templates": true,
              "chassis": [
                {
                  "number": 1,
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis1",
                  "power_sequencers": []
                }
              ]
            }
        )"_json;
        MockServices services{};
        parseRoot(element, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }

    // Test where fails: chassis value is invalid
    try
    {
        const json element = R"(
            {
              "chassis": [
                {
                  "number": "one",
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis1",
                  "power_sequencers": []
                }
              ]
            }
        )"_json;
        MockServices services{};
        parseRoot(element, services);
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
              "chassis": [
                {
                  "number": 1,
                  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis1",
                  "power_sequencers": []
                }
              ],
              "foo": true
            }
        )"_json;
        MockServices services{};
        parseRoot(element, services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}

TEST(ConfigFileParserTests, ParseVariables)
{
    // Test where works: No variables specified
    {
        const json element = R"(
            {
            }
        )"_json;
        auto variables = parseVariables(element);
        EXPECT_EQ(variables.size(), 0);
    }

    // Test where works: Variables specified
    {
        const json element = R"(
            {
              "chassis_number": "2",
              "bus_number": "13"
            }
        )"_json;
        auto variables = parseVariables(element);
        EXPECT_EQ(variables.size(), 2);
        EXPECT_EQ(variables["chassis_number"], "2");
        EXPECT_EQ(variables["bus_number"], "13");
    }

    // Test where fails: Element is not an object
    try
    {
        const json element = R"(
            [
              "chassis_number", "2",
              "bus_number", "13"
            ]
        )"_json;
        parseVariables(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }

    // Test where fails: Key is not a string
    try
    {
        const json element = R"(
            {
              chassis_number: "2",
              "bus_number": "13"
            }
        )"_json;
        parseVariables(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const json::parse_error& e)
    {}

    // Test where fails: Value is not a string
    try
    {
        const json element = R"(
            {
              "chassis_number": "2",
              "bus_number": 13
            }
        )"_json;
        parseVariables(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Key is an empty string
    try
    {
        const json element = R"(
            {
              "chassis_number": "2",
              "": "13"
            }
        )"_json;
        parseVariables(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }

    // Test where fails: Value is an empty string
    try
    {
        const json element = R"(
            {
              "chassis_number": "",
              "bus_number": "13"
            }
        )"_json;
        parseVariables(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }
}
