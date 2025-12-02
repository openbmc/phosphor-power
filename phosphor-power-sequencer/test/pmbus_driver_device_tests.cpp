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

#include "mock_pmbus.hpp"
#include "mock_services.hpp"
#include "pmbus.hpp"
#include "pmbus_driver_device.hpp"
#include "rail.hpp"
#include "temporary_subdirectory.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;
using namespace phosphor::pmbus;
using namespace phosphor::power::util;
namespace fs = std::filesystem;

using ::testing::Return;
using ::testing::Throw;

class PMBusDriverDeviceTests : public ::testing::Test
{
  protected:
    /**
     * Constructor.
     *
     * Creates a temporary directory to use in simulating sysfs files.
     */
    PMBusDriverDeviceTests() : ::testing::Test{}
    {
        tempDirPath = tempDir.getPath();
    }

    /**
     * Creates a Rail object that checks for a pgood fault using STATUS_VOUT.
     *
     * @param name Unique name for the rail
     * @param pageNum PMBus PAGE number of the rail
     * @return Rail object
     */
    std::unique_ptr<Rail> createRail(const std::string& name, uint8_t pageNum)
    {
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{pageNum};
        bool isPowerSupplyRail{false};
        bool checkStatusVout{true};
        bool compareVoltageToLimit{false};
        std::optional<PgoodGPIO> gpio{};
        return std::make_unique<Rail>(name, presence, page, isPowerSupplyRail,
                                      checkStatusVout, compareVoltageToLimit,
                                      gpio);
    }

    /**
     * Creates a file with the specified contents within the temporary
     * directory.
     *
     * @param name File name
     * @param contents File contents
     */
    void createFile(const std::string& name, const std::string& contents = "")
    {
        fs::path path{tempDirPath / name};
        std::ofstream out{path};
        out << contents;
        out.close();
    }

    /**
     * Temporary subdirectory used to create simulated sysfs / hmmon files.
     */
    TemporarySubDirectory tempDir;

    /**
     * Path to temporary subdirectory.
     */
    fs::path tempDirPath;
};

TEST_F(PMBusDriverDeviceTests, Constructor)
{
    // Test where works; optional parameters not specified
    {
        std::string name{"XYZ_PSEQ"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        rails.emplace_back(createRail("VDD", 5));
        rails.emplace_back(createRail("VIO", 7));
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        EXPECT_EQ(device.getName(), name);
        EXPECT_EQ(device.getBus(), bus);
        EXPECT_EQ(device.getAddress(), address);
        EXPECT_EQ(device.getPowerControlGPIOName(), powerControlGPIOName);
        EXPECT_EQ(device.getPowerGoodGPIOName(), powerGoodGPIOName);
        EXPECT_EQ(device.getRails().size(), 2);
        EXPECT_EQ(device.getRails()[0]->getName(), "VDD");
        EXPECT_EQ(device.getRails()[1]->getName(), "VIO");
        EXPECT_EQ(device.getDriverName(), "");
        EXPECT_EQ(device.getInstance(), 0);
    }

    // Test where works; optional parameters specified
    {
        std::string name{"XYZ_PSEQ"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"pgood"};
        std::vector<std::unique_ptr<Rail>> rails;
        rails.emplace_back(createRail("VDD", 5));
        rails.emplace_back(createRail("VIO", 7));
        std::string driverName{"xyzdev"};
        size_t instance{3};
        PMBusDriverDevice device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            driverName,
            instance};

        EXPECT_EQ(device.getName(), name);
        EXPECT_EQ(device.getBus(), bus);
        EXPECT_EQ(device.getAddress(), address);
        EXPECT_EQ(device.getPowerControlGPIOName(), powerControlGPIOName);
        EXPECT_EQ(device.getPowerGoodGPIOName(), powerGoodGPIOName);
        EXPECT_EQ(device.getRails().size(), 2);
        EXPECT_EQ(device.getRails()[0]->getName(), "VDD");
        EXPECT_EQ(device.getRails()[1]->getName(), "VIO");
        EXPECT_EQ(device.getDriverName(), driverName);
        EXPECT_EQ(device.getInstance(), instance);
    }
}

TEST_F(PMBusDriverDeviceTests, GetDriverName)
{
    std::string name{"XYZ_PSEQ"};
    uint8_t bus{3};
    uint16_t address{0x72};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails;
    std::string driverName{"xyzdev"};
    PMBusDriverDevice device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        driverName};

    EXPECT_EQ(device.getDriverName(), driverName);
}

TEST_F(PMBusDriverDeviceTests, GetInstance)
{
    std::string name{"XYZ_PSEQ"};
    uint8_t bus{3};
    uint16_t address{0x72};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails;
    std::string driverName{"xyzdev"};
    size_t instance{3};
    PMBusDriverDevice device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        driverName,
        instance};

    EXPECT_EQ(device.getInstance(), instance);
}

TEST_F(PMBusDriverDeviceTests, Open)
{
    std::string name{"XYZ_PSEQ"};
    uint8_t bus{3};
    uint16_t address{0x72};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails;
    PMBusDriverDevice device{name,
                             bus,
                             address,
                             powerControlGPIOName,
                             powerGoodGPIOName,
                             std::move(rails)};

    // Test where works
    EXPECT_FALSE(device.isOpen());
    MockServices services;
    device.open(services);
    EXPECT_TRUE(device.isOpen());

    MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
    EXPECT_CALL(pmbus, read("status13_vout", Type::Debug, true))
        .Times(1)
        .WillOnce(Return(0xde));
    uint8_t page{13};
    EXPECT_EQ(device.getStatusVout(page), 0xde);

    MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerControlGPIO());
    EXPECT_CALL(gpio, setValue(1)).Times(1);
    device.powerOn();

    // Test where does nothing because device is already open
    device.open(services);
    EXPECT_TRUE(device.isOpen());
}

TEST_F(PMBusDriverDeviceTests, Close)
{
    // Test where works
    {
        std::string name{"XYZ_PSEQ"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        EXPECT_TRUE(device.isOpen());

        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerGoodGPIO());
        EXPECT_CALL(gpio, release).Times(1);
        device.close();
        EXPECT_FALSE(device.isOpen());

        // Test where does nothing because device already closed
        device.close();
        EXPECT_FALSE(device.isOpen());
    }

    // Test where fails: Exception thrown
    try
    {
        std::string name{"XYZ_PSEQ"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        EXPECT_TRUE(device.isOpen());
        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerGoodGPIO());
        // Note: release() called twice. Once directly and once by destructor.
        EXPECT_CALL(gpio, release)
            .Times(2)
            .WillOnce(Throw(std::runtime_error{"Unable to release GPIO"}))
            .WillOnce(Return());
        device.close();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "Unable to release GPIO");
    }
}

TEST_F(PMBusDriverDeviceTests, GetPMBusInterface)
{
    std::string name{"XYZ_PSEQ"};
    uint8_t bus{3};
    uint16_t address{0x72};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails;
    PMBusDriverDevice device{name,
                             bus,
                             address,
                             powerControlGPIOName,
                             powerGoodGPIOName,
                             std::move(rails)};

    // Test where works
    MockServices services;
    device.open(services);
    MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
    EXPECT_CALL(pmbus, read("status13_vout", Type::Debug, true))
        .Times(1)
        .WillOnce(Return(0xde));
    uint8_t page{13};
    EXPECT_EQ(device.getStatusVout(page), 0xde);

    // Test where fails: Device not open
    try
    {
        device.close();
        device.getPMBusInterface();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "Device not open: XYZ_PSEQ");
    }
}

TEST_F(PMBusDriverDeviceTests, GetGPIOValues)
{
    // Test where works
    {
        std::string name{"ABC_382%#, ZY"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        std::vector<int> gpioValues{1, 1, 1};
        EXPECT_CALL(services, getGPIOValues("abc_382%#, zy"))
            .Times(1)
            .WillOnce(Return(gpioValues));

        device.open(services);
        EXPECT_TRUE(device.getGPIOValues(services) == gpioValues);
    }

    // Test where fails
    {
        std::string name{"XYZ_PSEQ"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        EXPECT_CALL(services, getGPIOValues("xyz_pseq"))
            .Times(1)
            .WillOnce(
                Throw(std::runtime_error{"libgpiod: Unable to open chip"}));

        // Device not open
        try
        {
            device.getGPIOValues(services);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Device not open: XYZ_PSEQ");
        }

        // Exception thrown
        try
        {
            device.open(services);
            device.getGPIOValues(services);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read GPIO values from device XYZ_PSEQ "
                         "using label xyz_pseq: "
                         "libgpiod: Unable to open chip");
        }
    }
}

TEST_F(PMBusDriverDeviceTests, GetStatusWord)
{
    // Test where works
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, read("status13", Type::Debug, true))
            .Times(1)
            .WillOnce(Return(0x1234));

        uint8_t page{13};
        EXPECT_EQ(device.getStatusWord(page), 0x1234);
    }

    // Test where fails
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        // Device not open
        try
        {
            uint8_t page{0};
            device.getStatusWord(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Device not open: xyz_pseq");
        }

        // Exception thrown
        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, read("status0", Type::Debug, true))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));
        try
        {
            uint8_t page{0};
            device.getStatusWord(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to read STATUS_WORD for PAGE 0 of device xyz_pseq: "
                "File does not exist");
        }
    }
}

TEST_F(PMBusDriverDeviceTests, GetStatusVout)
{
    // Test where works
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, read("status13_vout", Type::Debug, true))
            .Times(1)
            .WillOnce(Return(0xde));

        uint8_t page{13};
        EXPECT_EQ(device.getStatusVout(page), 0xde);
    }

    // Test where fails
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        // Device not open
        try
        {
            uint8_t page{0};
            device.getStatusVout(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Device not open: xyz_pseq");
        }

        // Exception thrown
        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, read("status0_vout", Type::Debug, true))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));
        try
        {
            uint8_t page{0};
            device.getStatusVout(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to read STATUS_VOUT for PAGE 0 of device xyz_pseq: "
                "File does not exist");
        }
    }
}

TEST_F(PMBusDriverDeviceTests, GetReadVout)
{
    // Test where works
    {
        // Create simulated hwmon voltage label file
        createFile("in13_label"); // PAGE 9 -> file number 13

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString("in13_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout10")); // PAGE number 9 + 1
        EXPECT_CALL(pmbus, readString("in13_input", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("851"));

        uint8_t page{9};
        EXPECT_EQ(device.getReadVout(page), 0.851);
    }

    // Test where fails
    {
        // Create simulated hwmon voltage label file
        createFile("in13_label"); // PAGE 8 -> file number 13

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        // Device not open
        try
        {
            uint8_t page{9};
            device.getReadVout(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Device not open: xyz_pseq");
        }

        // Exception thrown
        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString("in13_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout9")); // PAGE number 8 + 1
        try
        {
            uint8_t page{9};
            device.getReadVout(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to read READ_VOUT for PAGE 9 of device xyz_pseq: "
                "Unable to find hwmon file number for PAGE 9 of device xyz_pseq");
        }
    }
}

TEST_F(PMBusDriverDeviceTests, GetVoutUVFaultLimit)
{
    // Test where works
    {
        // Create simulated hwmon voltage label file
        createFile("in1_label"); // PAGE 6 -> file number 1

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString("in1_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout7")); // PAGE number 6 + 1
        EXPECT_CALL(pmbus, readString("in1_lcrit", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("1329"));

        uint8_t page{6};
        EXPECT_EQ(device.getVoutUVFaultLimit(page), 1.329);
    }

    // Test where fails
    {
        // Create simulated hwmon voltage label file
        createFile("in1_label"); // PAGE 7 -> file number 1

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        // Device not open
        try
        {
            uint8_t page{6};
            device.getVoutUVFaultLimit(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Device not open: xyz_pseq");
        }

        // Exception thrown
        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString("in1_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout8")); // PAGE number 7 + 1
        try
        {
            uint8_t page{6};
            device.getVoutUVFaultLimit(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to read VOUT_UV_FAULT_LIMIT for PAGE 6 of device xyz_pseq: "
                "Unable to find hwmon file number for PAGE 6 of device xyz_pseq");
        }
    }
}

TEST_F(PMBusDriverDeviceTests, GetPageToFileNumberMap)
{
    // Test where works: No voltage label files/mappings found
    {
        // Create simulated hwmon files.  None are valid voltage label files.
        createFile("in1_input");   // Not a label file
        createFile("in9_lcrit");   // Not a label file
        createFile("in_label");    // Invalid voltage label file name
        createFile("in9a_label");  // Invalid voltage label file name
        createFile("fan3_label");  // Not a voltage label file
        createFile("temp8_label"); // Not a voltage label file

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString).Times(0);

        const std::map<uint8_t, unsigned int>& map =
            device.getPageToFileNumberMap();
        EXPECT_TRUE(map.empty());
    }

    // Test where works: Multiple voltage label files/mappings found
    {
        // Create simulated hwmon files
        createFile("in9_label");   // PAGE 3 -> file number 9
        createFile("in13_label");  // PAGE 7 -> file number 13
        createFile("in0_label");   // PAGE 12 -> file number 0
        createFile("in11_label");  // No mapping; invalid contents
        createFile("in12_label");  // No mapping; invalid contents
        createFile("in1_input");   // Not a label file
        createFile("in7_lcrit");   // Not a label file
        createFile("fan3_label");  // Not a voltage label file
        createFile("temp8_label"); // Not a voltage label file

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString("in9_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout4")); // PAGE number 3 + 1
        EXPECT_CALL(pmbus, readString("in13_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout8")); // PAGE number 7 + 1
        EXPECT_CALL(pmbus, readString("in0_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout13")); // PAGE number 12 + 1
        EXPECT_CALL(pmbus, readString("in11_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout")); // Invalid format
        EXPECT_CALL(pmbus, readString("in12_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout13a")); // Invalid format

        const std::map<uint8_t, unsigned int>& map =
            device.getPageToFileNumberMap();
        EXPECT_EQ(map.size(), 3);
        EXPECT_EQ(map.at(uint8_t{3}), 9);
        EXPECT_EQ(map.at(uint8_t{7}), 13);
        EXPECT_EQ(map.at(uint8_t{12}), 0);
    }

    // Test where fails: Device not open
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        try
        {
            device.getPageToFileNumberMap();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Device not open: xyz_pseq");
        }
    }

    // Test where fails: hwmon directory path is actually a file
    {
        // Create file that will be returned as the hwmon directory path
        createFile("in9_label");

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath / "in9_label"));
        EXPECT_CALL(pmbus, readString).Times(0);

        const std::map<uint8_t, unsigned int>& map =
            device.getPageToFileNumberMap();
        EXPECT_TRUE(map.empty());
    }

    // Test where fails: hwmon directory path does not exist
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath / "does_not_exist"));
        EXPECT_CALL(pmbus, readString).Times(0);

        const std::map<uint8_t, unsigned int>& map =
            device.getPageToFileNumberMap();
        EXPECT_TRUE(map.empty());
    }

    // Test where fails: hwmon directory path is not readable
    {
        // Create simulated hwmon files
        createFile("in9_label");
        createFile("in13_label");
        createFile("in0_label");

        // Change temporary directory to be unreadable
        fs::permissions(tempDirPath, fs::perms::none);

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString).Times(0);

        try
        {
            device.getPageToFileNumberMap();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            // Error message varies
        }

        // Change temporary directory to be readable/writable
        fs::permissions(tempDirPath, fs::perms::owner_all);
    }
}

TEST_F(PMBusDriverDeviceTests, GetFileNumber)
{
    // Test where works
    {
        // Create simulated hwmon voltage label files
        createFile("in0_label");  // PAGE 6 -> file number 0
        createFile("in13_label"); // PAGE 9 -> file number 13

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString("in0_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout7")); // PAGE number 6 + 1
        EXPECT_CALL(pmbus, readString("in13_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout10")); // PAGE number 9 + 1

        // Map was empty and needs to be built
        uint8_t page{6};
        EXPECT_EQ(device.getFileNumber(page), 0);

        // Map had already been built
        page = 9;
        EXPECT_EQ(device.getFileNumber(page), 13);
    }

    // Test where fails: Device not open
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        try
        {
            uint8_t page{13};
            device.getFileNumber(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Device not open: xyz_pseq");
        }
    }

    // Test where fails: No mapping for specified PMBus PAGE
    {
        // Create simulated hwmon voltage label files
        createFile("in0_label");  // PAGE 6 -> file number 0
        createFile("in13_label"); // PAGE 9 -> file number 13

        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        PMBusDriverDevice device{name,
                                 bus,
                                 address,
                                 powerControlGPIOName,
                                 powerGoodGPIOName,
                                 std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return(tempDirPath));
        EXPECT_CALL(pmbus, readString("in0_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout7")); // PAGE number 6 + 1
        EXPECT_CALL(pmbus, readString("in13_label", Type::Hwmon))
            .Times(1)
            .WillOnce(Return("vout10")); // PAGE number 9 + 1

        try
        {
            uint8_t page{13};
            device.getFileNumber(page);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to find hwmon file number for PAGE 13 of device xyz_pseq");
        }
    }
}

TEST_F(PMBusDriverDeviceTests, PrepareForPgoodFaultDetection)
{
    // This is a protected method and cannot be called directly from a gtest.
    // Call findPgoodFault() which calls prepareForPgoodFaultDetection().

    // Create simulated hwmon voltage label file
    createFile("in1_label"); // PAGE 6 -> file number 1

    std::string name{"xyz_pseq"};
    uint8_t bus{3};
    uint16_t address{0x72};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails;
    PMBusDriverDevice device{name,
                             bus,
                             address,
                             powerControlGPIOName,
                             powerGoodGPIOName,
                             std::move(rails)};

    MockServices services;
    std::vector<int> gpioValues{1, 1, 1};
    EXPECT_CALL(services, getGPIOValues("xyz_pseq"))
        .Times(1)
        .WillOnce(Return(gpioValues));

    device.open(services);

    // Methods that get hwmon file info should be called twice
    MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
    EXPECT_CALL(pmbus, getPath(Type::Hwmon))
        .Times(2)
        .WillRepeatedly(Return(tempDirPath));
    EXPECT_CALL(pmbus, readString("in1_label", Type::Hwmon))
        .Times(2)
        .WillRepeatedly(Return("vout7")); // PAGE number 6 + 1

    // Map was empty and needs to be built
    uint8_t page{6};
    EXPECT_EQ(device.getFileNumber(page), 1);

    // Call findPgoodFault() which calls prepareForPgoodFaultDetection() which
    // rebuilds the map.
    std::string powerSupplyError{};
    std::map<std::string, std::string> additionalData{};
    std::string error =
        device.findPgoodFault(services, powerSupplyError, additionalData);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(additionalData.size(), 0);

    EXPECT_EQ(device.getFileNumber(page), 1);
}
