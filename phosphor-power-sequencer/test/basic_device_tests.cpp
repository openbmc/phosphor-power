/**
 * Copyright © 2025 IBM Corporation
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

#include "basic_device.hpp"
#include "mock_gpio.hpp"
#include "mock_services.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

using ::testing::Return;
using ::testing::Throw;

/**
 * @class BasicDeviceImpl
 *
 * Concrete subclass of the BasicDevice abstract class.
 *
 * This subclass is required because BasicDevice does not implement all the pure
 * virtual methods inherited for PowerSequencerDevice, meaning it cannot be
 * instantiated.
 *
 * This class is not intended to be used outside of this file.  It is
 * implementation detail for testing the BasicDevice class.
 */
class BasicDeviceImpl : public BasicDevice
{
  public:
    BasicDeviceImpl() = delete;
    BasicDeviceImpl(const BasicDeviceImpl&) = delete;
    BasicDeviceImpl(BasicDeviceImpl&&) = delete;
    BasicDeviceImpl& operator=(const BasicDeviceImpl&) = delete;
    BasicDeviceImpl& operator=(BasicDeviceImpl&&) = delete;
    virtual ~BasicDeviceImpl() = default;

    explicit BasicDeviceImpl(
        const std::string& name, uint8_t bus, uint16_t address,
        const std::string& powerControlGPIOName,
        const std::string& powerGoodGPIOName,
        std::vector<std::unique_ptr<Rail>> rails, Services& services) :
        BasicDevice(name, bus, address, powerControlGPIOName, powerGoodGPIOName,
                    std::move(rails), services)
    {}

    // Mock pure virtual methods
    MOCK_METHOD(std::vector<int>, getGPIOValues, (Services & services),
                (override));
    MOCK_METHOD(uint16_t, getStatusWord, (uint8_t page), (override));
    MOCK_METHOD(uint8_t, getStatusVout, (uint8_t page), (override));
    MOCK_METHOD(double, getReadVout, (uint8_t page), (override));
    MOCK_METHOD(double, getVoutUVFaultLimit, (uint8_t page), (override));
    MOCK_METHOD(std::string, findPgoodFault,
                (Services & services, const std::string& powerSupplyError,
                 (std::map<std::string, std::string> & additionalData)),
                (override));
};

/**
 * Creates a Rail object that checks for a pgood fault using a GPIO.
 *
 * @param name Unique name for the rail
 * @param gpio GPIO line to read to determine the pgood status of the rail
 * @return Rail object
 */
static std::unique_ptr<Rail> createRail(const std::string& name,
                                        unsigned int gpioLine)
{
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    bool activeLow{false};
    std::optional<PgoodGPIO> gpio{PgoodGPIO{gpioLine, activeLow}};
    return std::make_unique<Rail>(name, presence, page, isPowerSupplyRail,
                                  checkStatusVout, compareVoltageToLimit, gpio);
}

TEST(BasicDeviceTests, Constructor)
{
    // Test where works: Empty vector of rails
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        EXPECT_EQ(device.getName(), name);
        EXPECT_EQ(device.getBus(), bus);
        EXPECT_EQ(device.getAddress(), address);
        EXPECT_EQ(device.getPowerControlGPIOName(), powerControlGPIOName);
        EXPECT_EQ(device.getPowerGoodGPIOName(), powerGoodGPIOName);
        EXPECT_TRUE(device.getRails().empty());
    }

    // Test where works: Non-empty vector of rails
    {
        std::string name{"abc_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRail("VDD", 5));
        rails.emplace_back(createRail("VIO", 7));
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        EXPECT_EQ(device.getName(), name);
        EXPECT_EQ(device.getBus(), bus);
        EXPECT_EQ(device.getAddress(), address);
        EXPECT_EQ(device.getPowerControlGPIOName(), powerControlGPIOName);
        EXPECT_EQ(device.getPowerGoodGPIOName(), powerGoodGPIOName);
        EXPECT_EQ(device.getRails().size(), 2);
        EXPECT_EQ(device.getRails()[0]->getName(), "VDD");
        EXPECT_EQ(device.getRails()[1]->getName(), "VIO");
    }
}

TEST(BasicDeviceTests, GetName)
{
    std::string name{"xyz_pseq"};
    uint8_t bus{0};
    uint16_t address{0x23};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails{};
    MockServices services;
    BasicDeviceImpl device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        services};

    EXPECT_EQ(device.getName(), name);
}

TEST(BasicDeviceTests, GetBus)
{
    std::string name{"abc_pseq"};
    uint8_t bus{1};
    uint16_t address{0x23};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails{};
    MockServices services;
    BasicDeviceImpl device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        services};

    EXPECT_EQ(device.getBus(), bus);
}

TEST(BasicDeviceTests, GetAddress)
{
    std::string name{"abc_pseq"};
    uint8_t bus{1};
    uint16_t address{0x24};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails{};
    MockServices services;
    BasicDeviceImpl device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        services};

    EXPECT_EQ(device.getAddress(), address);
}

TEST(BasicDeviceTests, GetPowerControlGPIOName)
{
    std::string name{"xyz_pseq"};
    uint8_t bus{0};
    uint16_t address{0x23};
    std::string powerControlGPIOName{"power-on"};
    std::string powerGoodGPIOName{"chassis-pgood"};
    std::vector<std::unique_ptr<Rail>> rails{};
    MockServices services;
    BasicDeviceImpl device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        services};

    EXPECT_EQ(device.getPowerControlGPIOName(), powerControlGPIOName);
}

TEST(BasicDeviceTests, GetPowerGoodGPIOName)
{
    std::string name{"xyz_pseq"};
    uint8_t bus{0};
    uint16_t address{0x23};
    std::string powerControlGPIOName{"power-on"};
    std::string powerGoodGPIOName{"chassis-pgood"};
    std::vector<std::unique_ptr<Rail>> rails{};
    MockServices services;
    BasicDeviceImpl device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        services};

    EXPECT_EQ(device.getPowerGoodGPIOName(), powerGoodGPIOName);
}

TEST(BasicDeviceTests, GetRails)
{
    // Empty vector of rails
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        EXPECT_TRUE(device.getRails().empty());
    }

    // Non-empty vector of rails
    {
        std::string name{"abc_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRail("VDD", 5));
        rails.emplace_back(createRail("VIO", 7));
        rails.emplace_back(createRail("VDDR", 9));
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        EXPECT_EQ(device.getRails().size(), 3);
        EXPECT_EQ(device.getRails()[0]->getName(), "VDD");
        EXPECT_EQ(device.getRails()[1]->getName(), "VIO");
        EXPECT_EQ(device.getRails()[2]->getName(), "VDDR");
    }
}

TEST(BasicDeviceTests, GetPowerControlGPIO)
{
    std::string name{"xyz_pseq"};
    uint8_t bus{0};
    uint16_t address{0x23};
    std::string powerControlGPIOName{"power-on"};
    std::string powerGoodGPIOName{"chassis-pgood"};
    std::vector<std::unique_ptr<Rail>> rails{};
    MockServices services;
    BasicDeviceImpl device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        services};

    MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerControlGPIO());
    EXPECT_CALL(gpio, setValue(1)).Times(1);
    device.powerOn();
}

TEST(BasicDeviceTests, GetPowerGoodGPIO)
{
    std::string name{"xyz_pseq"};
    uint8_t bus{0};
    uint16_t address{0x23};
    std::string powerControlGPIOName{"power-on"};
    std::string powerGoodGPIOName{"chassis-pgood"};
    std::vector<std::unique_ptr<Rail>> rails{};
    MockServices services;
    BasicDeviceImpl device{
        name,
        bus,
        address,
        powerControlGPIOName,
        powerGoodGPIOName,
        std::move(rails),
        services};

    MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerGoodGPIO());
    EXPECT_CALL(gpio, getValue()).Times(1).WillOnce(Return(0));
    EXPECT_FALSE(device.getPowerGood());
}

TEST(BasicDeviceTests, PowerOn)
{
    // Test where works
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerControlGPIO());
        EXPECT_CALL(gpio, requestWrite(1)).Times(1);
        EXPECT_CALL(gpio, setValue(1)).Times(1);
        EXPECT_CALL(gpio, release()).Times(1);
        device.powerOn();
    }

    // Test where fails with exception
    try
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerControlGPIO());
        EXPECT_CALL(gpio, requestWrite(1))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
        device.powerOn();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "Unable to write GPIO");
    }
}

TEST(BasicDeviceTests, PowerOff)
{
    // Test where works
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerControlGPIO());
        EXPECT_CALL(gpio, requestWrite(0)).Times(1);
        EXPECT_CALL(gpio, setValue(0)).Times(1);
        EXPECT_CALL(gpio, release()).Times(1);
        device.powerOff();
    }

    // Test where fails with exception
    try
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerControlGPIO());
        EXPECT_CALL(gpio, requestWrite(0)).Times(1);
        EXPECT_CALL(gpio, setValue(0))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"Unable to write GPIO"}));
        device.powerOff();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "Unable to write GPIO");
    }
}

TEST(BasicDeviceTests, GetPowerGood)
{
    // Test where works: Value is false
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerGoodGPIO());
        EXPECT_CALL(gpio, getValue()).Times(1).WillOnce(Return(0));
        EXPECT_FALSE(device.getPowerGood());
    }

    // Test where works: Value is true
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerGoodGPIO());
        EXPECT_CALL(gpio, getValue()).Times(1).WillOnce(Return(1));
        EXPECT_TRUE(device.getPowerGood());
    }

    // Test where fails with exception
    try
    {
        std::string name{"xyz_pseq"};
        uint8_t bus{0};
        uint16_t address{0x23};
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        std::vector<std::unique_ptr<Rail>> rails{};
        MockServices services;
        BasicDeviceImpl device{
            name,
            bus,
            address,
            powerControlGPIOName,
            powerGoodGPIOName,
            std::move(rails),
            services};

        MockGPIO& gpio = static_cast<MockGPIO&>(device.getPowerGoodGPIO());
        EXPECT_CALL(gpio, getValue())
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"Unable to read GPIO"}));
        device.getPowerGood();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "Unable to read GPIO");
    }
}
