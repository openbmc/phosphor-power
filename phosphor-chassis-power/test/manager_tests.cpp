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
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::chassis;

namespace fs = std::filesystem;

const fs::path testConfigDir{"/tmp/phosphor-chassis-power"};

/**
 * @class TestManager
 *
 * Derived Manager class that exposes protected members for testing.
 */
class TestManager : public Manager
{
  public:
    TestManager(sdbusplus::bus_t& bus, const sdeventplus::Event& event) :
        Manager(bus, event)
    {}

    // Expose protected methods for testing
    using Manager::compatibleSystemTypesNotFoundCallback;
    using Manager::findConfigFile;
    using Manager::loadConfigFile;

    // Expose protected members for testing
    using Manager::bus;
    using Manager::compatibleSystemsTimer;
    using Manager::compatibleSystemTypes;
    using Manager::compatSysTypesFinder;
    using Manager::eventLoop;
    using Manager::system;
};

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
     * @brief Write a config file to the specified path.
     *
     * @param[in] pathName - The path to the file to write.
     * @param[in] contents - The contents to write to the file.
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
 * Test compatibleSystemTypesFound()
 */
TEST_F(ManagerTests, compatibleSystemTypesFound)
{
    TestManager manager{bus, event};
    std::vector<std::string> types{"com.ibm.Hardware.Chassis.Model.Huygens"};

    // Call with types - should store types and disable timer
    {
        // Verify timer is initially enabled
        EXPECT_TRUE(manager.compatibleSystemsTimer.isEnabled());

        // Verify compatibleSystemTypes is initially empty
        EXPECT_TRUE(manager.compatibleSystemTypes.empty());

        // Call compatibleSystemTypesFound()
        manager.compatibleSystemTypesFound(types);

        // Verify timer is now disabled
        EXPECT_FALSE(manager.compatibleSystemsTimer.isEnabled());

        // Verify compatibleSystemTypes was stored
        EXPECT_EQ(manager.compatibleSystemTypes, types);
    }

    // Call with different types - should be ignored
    {
        std::vector<std::string> types2{
            "com.ibm.Hardware.Chassis.Model.Different"};

        // Second call with different types
        manager.compatibleSystemTypesFound(types2);

        // Verify original types are still stored (second call ignored)
        EXPECT_EQ(manager.compatibleSystemTypes, types);
    }
}

/**
 * Test findConfigFile()
 */
TEST_F(ManagerTests, findConfigFile)
{
    TestManager manager{bus, event};

    // No compatible system types - should return empty path
    {
        fs::path result = manager.findConfigFile();
        EXPECT_TRUE(result.empty());
    }

    // Compatible types but no config file - should return empty path
    {
        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypes = types;

        fs::path result = manager.findConfigFile();
        EXPECT_TRUE(result.empty());
    }

    // Find config file with full system type name
    {
        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypes = types;

        // Create config file with full name
        createTestConfigFile("com.ibm.Hardware.Chassis.Model.Huygens.json");

        fs::path result = manager.findConfigFile();
        EXPECT_FALSE(result.empty());
        EXPECT_EQ(result.filename(),
                  "com.ibm.Hardware.Chassis.Model.Huygens.json");
    }

    // Find config file with last node name
    {
        // Clean up previous test files
        fs::remove_all(testConfigDir);

        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypes = types;

        // Create config file with last node name only
        createTestConfigFile("Huygens.json");

        fs::path result = manager.findConfigFile();
        EXPECT_FALSE(result.empty());
        EXPECT_EQ(result.filename(), "Huygens.json");
    }

    // Multiple compatible types - should find first match
    {
        // Clean up previous test files
        fs::remove_all(testConfigDir);

        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens",
            "com.ibm.Hardware.Chassis.Model.Rainier"};
        manager.compatibleSystemTypes = types;

        // Create config files
        createTestConfigFile("Huygens.json");
        createTestConfigFile("Rainier.json");

        fs::path result = manager.findConfigFile();
        EXPECT_FALSE(result.empty());
        EXPECT_EQ(result.filename(), "Huygens.json");
    }
}

/**
 * Test loadConfigFile()
 */
TEST_F(ManagerTests, loadConfigFile)
{
    TestManager manager{bus, event};
    std::vector<std::string> types{"com.ibm.Hardware.Chassis.Model.Huygens"};
    manager.compatibleSystemTypes = types;
    // No config file found - system should remain false
    {
        EXPECT_FALSE(manager.system);

        manager.loadConfigFile();

        // Verify system is still false
        EXPECT_FALSE(manager.system);
        EXPECT_FALSE(manager.isConfigFileLoaded());
    }

    // Config file found - system should be set to true
    {
        // Create config file
        createTestConfigFile("Huygens.json");

        EXPECT_FALSE(manager.system);

        // Load config file
        manager.loadConfigFile();

        // Verify system is now true
        EXPECT_TRUE(manager.system);
        EXPECT_TRUE(manager.isConfigFileLoaded());
    }
}

/**
 * Test isConfigFileLoaded()
 */
TEST_F(ManagerTests, isConfigFileLoaded)
{
    TestManager manager{bus, event};

    // Initially should return false
    EXPECT_FALSE(manager.isConfigFileLoaded());

    // After setting system to true, should return true
    manager.system = true;
    EXPECT_TRUE(manager.isConfigFileLoaded());
}
