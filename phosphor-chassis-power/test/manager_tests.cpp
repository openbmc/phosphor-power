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
#include "manager.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::chassis;

namespace fs = std::filesystem;

const fs::path testConfigDir{"/tmp/phosphor-chassis-power"};

/**
 * @class ManagerTests
 *
 * Test fixture for Manager class tests.
 */
class ManagerTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the D-Bus bus and event loop needed for Manager tests.
     */
    ManagerTests() :
        bus{sdbusplus::bus::new_default()},
        event{sdeventplus::Event::get_default()}
    {
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    }

    /**
     * Destructor.
     *
     * Cleans up any test configuration files.
     */
    virtual ~ManagerTests()
    {
        // Remove test config files if they exist
        fs::remove_all(testConfigDir);
    }

    /**
     * Write a config file to the specified path.
     *
     * @param pathName - The path to the file to write.
     * @param contents - The contents to write to the file.
     */
    void writeConfigFile(const fs::path& pathName, const std::string& contents)
    {
        std::ofstream file{pathName};

        file << contents;
    }

  protected:
    /**
     * Creates a test configuration file with the specified name.
     *
     * @param fileName name of the configuration file
     * @return path to the created file
     */
    fs::path createTestConfigFile(const std::string& fileName)
    {
        // Create directory if it doesn't exist
        fs::create_directories(testConfigDir);

        // Create test config file with minimal valid JSON
        fs::path configPath = testConfigDir / fileName;
        constexpr const char* configJson = R"([
            {
                "ChassisNumber": 1,
                "PresenceGpio": {
                    "Name": "presence-chassis1",
                    "Direction": "Input",
                    "Polarity": "Low"
                }
            }
        ])";

        writeConfigFile(configPath, configJson);

        return configPath;
    }

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t bus;

    /**
     * Event loop object.
     */
    sdeventplus::Event event;
};

/**
 * Test Manager constructor
 */
TEST_F(ManagerTests, Constructor)
{
    // Create Manager object
    Manager manager{bus, event};

    // Verify config file is not loaded initially
    EXPECT_FALSE(manager.isConfigFileLoaded());

    // Verify compatible system types list is empty initially
    EXPECT_TRUE(manager.getCompatibleSystemTypes().empty());
}

/**
 * Test compatibleSystemTypesFound()
 */
TEST_F(ManagerTests, CompatibleSystemTypesFound)
{
    Manager manager{bus, event};
    std::vector<std::string> types{"com.ibm.Hardware.Chassis.Model.Huygens"};
    createTestConfigFile("Huygens.json");

    // Call with types - should store types and load config file
    {
        // Verify compatibleSystemTypes is initially empty
        EXPECT_TRUE(manager.getCompatibleSystemTypes().empty());

        // Call compatibleSystemTypesFound()
        manager.compatibleSystemTypesFound(types);

        // Verify compatibleSystemTypes was stored
        EXPECT_EQ(manager.getCompatibleSystemTypes(), types);

        // Verify config file is loaded
        EXPECT_TRUE(manager.isConfigFileLoaded());
    }

    // Call with different types - should be ignored
    {
        std::vector<std::string> types2{
            "com.ibm.Hardware.Chassis.Model.Different"};

        // Second call with different types
        manager.compatibleSystemTypesFound(types2);

        // Verify original types are still stored (second call ignored)
        EXPECT_EQ(manager.getCompatibleSystemTypes(), types);
    }
}

/**
 * Test isConfigFileLoaded()
 */
TEST_F(ManagerTests, IsConfigFileLoaded)
{
    Manager manager{bus, event};
    createTestConfigFile("Huygens.json");

    // Initially should return false
    EXPECT_FALSE(manager.isConfigFileLoaded());

    std::vector<std::string> types{"com.ibm.Hardware.Chassis.Model.Huygens"};

    // Call compatibleSystemTypesFound with types, and load the config file
    manager.compatibleSystemTypesFound(types);

    // isConfigFileLoaded should now be true
    EXPECT_TRUE(manager.isConfigFileLoaded());
}

/**
 * Test findConfigFile() through compatibleSystemTypesFound()
 */
TEST_F(ManagerTests, FindConfigFile)
{
    Manager manager{bus, event};

    // Test config file not found
    {
        fs::remove_all(testConfigDir);

        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.NonExistent"};
        manager.compatibleSystemTypesFound(types);

        EXPECT_FALSE(manager.isConfigFileLoaded());
        EXPECT_EQ(manager.getCompatibleSystemTypes(), types);

        manager.clearCompatibleSystemTypes();
    }

    // Find config file with full system type name
    {
        createTestConfigFile("com.ibm.Hardware.Chassis.Model.Huygens.json");

        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypesFound(types);

        EXPECT_TRUE(manager.isConfigFileLoaded());
        EXPECT_EQ(manager.getCompatibleSystemTypes(), types);

        manager.clearCompatibleSystemTypes();
    }

    // Find config file with last node name
    {
        fs::remove_all(testConfigDir);
        createTestConfigFile("Huygens.json");

        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypesFound(types);

        EXPECT_TRUE(manager.isConfigFileLoaded());
        EXPECT_EQ(manager.getCompatibleSystemTypes(), types);

        manager.clearCompatibleSystemTypes();
    }

    // Multiple compatible types - should find first match
    {
        fs::remove_all(testConfigDir);
        createTestConfigFile("Huygens.json");

        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens",
            "com.ibm.Hardware.Chassis.Model.Rainier"};

        manager.compatibleSystemTypesFound(types);

        EXPECT_TRUE(manager.isConfigFileLoaded());
        EXPECT_EQ(manager.getCompatibleSystemTypes(), types);

        manager.clearCompatibleSystemTypes();
    }
}

/**
 * Test loadConfigFile() through compatibleSystemTypesFound()
 */
TEST_F(ManagerTests, LoadConfigFile)
{
    Manager manager{bus, event};

    // No config file found - system should remain false
    {
        fs::remove_all(testConfigDir);

        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.NonExistent"};
        manager.compatibleSystemTypesFound(types);

        EXPECT_FALSE(manager.isConfigFileLoaded());
        EXPECT_EQ(manager.getCompatibleSystemTypes(), types);

        manager.clearCompatibleSystemTypes();
    }

    // Config file found - system should be set to true
    {
        createTestConfigFile("Huygens.json");

        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypesFound(types);

        EXPECT_TRUE(manager.isConfigFileLoaded());
        EXPECT_EQ(manager.getCompatibleSystemTypes(), types);

        manager.clearCompatibleSystemTypes();
    }
}
