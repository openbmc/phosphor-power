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

#include <gmock/gmock.h>
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

// Test finds system types
TEST_F(ManagerTests, CompatibleSystemTypesFound)
{
    // Remove previous test files
    fs::remove_all("/tmp/phosphor-chassis-power");
    // Create Huygens config file
    createTestConfigFile("Huygens.json");
    // Create Manager object
    Manager manager{bus, event};

    // Simulate finding compatible system types for non-existent config
    std::vector<std::string> types{"com.ibm.Hardware.Chassis.Model.Huygens"};

    // Call compatibleSystemTypesFound()
    // loadConfigFile() will be called and will find the file
    manager.compatibleSystemTypesFound(types);
    // Config file should be loaded
    EXPECT_EQ(manager.isConfigFileLoaded(), true);
}
// Test finds compatible system types but not json file
TEST_F(ManagerTests, ConfigFileNotFound)
{
    // Remove previous test files
    fs::remove_all("/tmp/phosphor-chassis-power");
    // Create Manager object
    Manager manager{bus, event};

    // Simulate finding compatible system types for non-existent config
    std::vector<std::string> types{"com.ibm.Hardware.Chassis.Model.Huygens"};

    // Call compatibleSystemTypesFound()
    // loadConfigFile() will be called but won't find the file
    EXPECT_NO_THROW(manager.compatibleSystemTypesFound(types));
    // Config file should not be loaded
    EXPECT_EQ(manager.isConfigFileLoaded(), false);
}
