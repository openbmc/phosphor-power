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

#include "manager.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::chassis;

namespace fs = std::filesystem;

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
        fs::path testConfigDir{"/etc/phosphor-chassis-power"};
        if (fs::exists(testConfigDir))
        {
            fs::remove_all(testConfigDir);
        }
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
        // Create test config directory if it doesn't exist
        fs::path testConfigDir{"/etc/phosphor-chassis-power"};
        if (!fs::exists(testConfigDir))
        {
            fs::create_directories(testConfigDir);
        }

        // Create test config file with minimal valid JSON
        fs::path configPath = testConfigDir / fileName;
        std::ofstream file{configPath};
        file << R"([
    {
        "ChassisNumber": 1,
        "PresenceGpio": {
            "Name": "presence-chassis1",
            "Direction": "Input",
            "Polarity": "Low"
        }
    }
])";
        file.close();
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

TEST_F(ManagerTests, Configure)
{
    // Test where config file does not exist
    {
        // Ensure clean state
        fs::remove_all("/etc/phosphor-chassis-power");

        // Create Manager object
        Manager manager{bus, event};

        // Simulate finding compatible system types for non-existent config
        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypesFound(types);

        // Call configure() - should throw exception since config file not found
        EXPECT_THROW(manager.configure(), sdbusplus::exception_t);
    }

    // Test where config file exists with full system type name
    {
        // Remove previous test files
        fs::remove_all("/etc/phosphor-chassis-power");

        // Create test config file with full system type name
        createTestConfigFile("com.ibm.Hardware.Chassis.Model.Huygens.json");

        // Create Manager object
        Manager manager{bus, event};

        // Simulate finding compatible system types
        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypesFound(types);

        // Call configure() - should succeed
        EXPECT_NO_THROW(manager.configure());
    }

    // Test where config file exists with last node name only
    {
        // Remove previous test files
        fs::remove_all("/etc/phosphor-chassis-power");

        // Create test config file with short name (last node only)
        createTestConfigFile("Huygens.json");

        // Create Manager object
        Manager manager{bus, event};

        // Simulate finding compatible system types with full dotted name
        // findConfigFile() should find "Huygens.json" by extracting last node
        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        manager.compatibleSystemTypesFound(types);

        // Call configure() - should succeed
        EXPECT_NO_THROW(manager.configure());
    }
}

TEST_F(ManagerTests, CompatibleSystemTypesFound)
{
    // Test where compatible system types are found
    {
        // Remove previous test files
        fs::remove_all("/etc/phosphor-chassis-power");

        // Create test config file
        createTestConfigFile("Huygens.json");

        // Create Manager object
        Manager manager{bus, event};

        // Simulate finding compatible system types
        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};

        // Call compatibleSystemTypesFound()
        // This should store types and call loadConfigFile()
        EXPECT_NO_THROW(manager.compatibleSystemTypesFound(types));

        // Verify config file was loaded by calling configure()
        EXPECT_NO_THROW(manager.configure());
    }

    // Test where compatible system types are empty
    {
        // Create Manager object
        Manager manager{bus, event};

        // Call with empty vector - should do nothing (early return)
        std::vector<std::string> types{};
        EXPECT_NO_THROW(manager.compatibleSystemTypesFound(types));
    }

    // Test where compatible system types are found multiple times
    {
        // Remove previous test files
        fs::remove_all("/etc/phosphor-chassis-power");

        // Create test config file
        createTestConfigFile("Huygens.json");

        // Create Manager object
        Manager manager{bus, event};

        // Call compatibleSystemTypesFound() first time
        std::vector<std::string> types1{
            "com.ibm.Hardware.Chassis.Model.Huygens"};
        EXPECT_NO_THROW(manager.compatibleSystemTypesFound(types1));

        // Call compatibleSystemTypesFound() second time with different types
        // Should be ignored since compatibleSystemTypes is no longer empty
        std::vector<std::string> types2{
            "com.ibm.Hardware.Chassis.Model.Different"};
        EXPECT_NO_THROW(manager.compatibleSystemTypesFound(types2));

        // Verify first types were used (Huygens.json should be loaded)
        EXPECT_NO_THROW(manager.configure());
    }

    // Test where config file does not exist for found system types
    {
        // Remove previous test files
        fs::remove_all("/etc/phosphor-chassis-power");

        // Create Manager object
        Manager manager{bus, event};

        // Simulate finding compatible system types for non-existent config
        std::vector<std::string> types{
            "com.ibm.Hardware.Chassis.Model.Huygens"};

        // Call compatibleSystemTypesFound()
        // loadConfigFile() will be called but won't find the file
        // No exception thrown here - error happens in configure()
        EXPECT_NO_THROW(manager.compatibleSystemTypesFound(types));

        // Verify config was not loaded
        EXPECT_THROW(manager.configure(), sdbusplus::exception_t);
    }
}
