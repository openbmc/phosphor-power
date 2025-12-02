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
#include "rail.hpp"
#include "ucd90x_device.hpp"

#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;
using namespace phosphor::pmbus;

using ::testing::Return;
using ::testing::Throw;

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

TEST(UCD90xDeviceTests, Constructor)
{
    std::string name{"ucd90320"};
    uint8_t bus{3};
    uint16_t address{0x72};
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    std::vector<std::unique_ptr<Rail>> rails;
    rails.emplace_back(createRail("VDD", 5));
    rails.emplace_back(createRail("VIO", 7));
    UCD90xDevice device{name,
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
    EXPECT_EQ(device.getDriverName(), "ucd9000");
    EXPECT_EQ(device.getInstance(), 0);
}

TEST(UCD90xDeviceTests, GetMfrStatus)
{
    // Test where works
    {
        std::string name{"ucd90320"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        UCD90xDevice device{name,
                            bus,
                            address,
                            powerControlGPIOName,
                            powerGoodGPIOName,
                            std::move(rails)};

        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        uint64_t mfrStatus{0x123456789abcull};
        EXPECT_CALL(pmbus, read("mfr_status", Type::HwmonDeviceDebug, true))
            .Times(1)
            .WillOnce(Return(mfrStatus));

        EXPECT_EQ(device.getMfrStatus(), mfrStatus);
    }

    // Test where fails
    {
        std::string name{"ucd90320"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        UCD90xDevice device{name,
                            bus,
                            address,
                            powerControlGPIOName,
                            powerGoodGPIOName,
                            std::move(rails)};

        // Device not open
        try
        {
            device.getMfrStatus();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Device not open: ucd90320");
        }

        // Exception thrown
        MockServices services;
        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, read("mfr_status", Type::HwmonDeviceDebug, true))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));
        try
        {
            device.getMfrStatus();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read MFR_STATUS for device ucd90320: "
                         "File does not exist");
        }
    }
}

TEST(UCD90xDeviceTests, StorePgoodFaultDebugData)
{
    // This is a protected method and cannot be called directly from a gtest.
    // Call findPgoodFault() which calls storePgoodFaultDebugData().

    // Test where works
    {
        MockServices services;
        std::vector<int> gpioValues{1, 1, 0};
        EXPECT_CALL(services, getGPIOValues("ucd90320"))
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(services,
                    logInfoMsg("Device ucd90320 GPIO values: [1, 1, 0]"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("Device ucd90320 MFR_STATUS: 0x123456789abc"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device ucd90320"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD pgood GPIO line offset 2 has inactive value 0"))
            .Times(1);

        std::string name{"ucd90320"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        rails.emplace_back(createRail("VDD", 2));
        UCD90xDevice device{name,
                            bus,
                            address,
                            powerControlGPIOName,
                            powerGoodGPIOName,
                            std::move(rails)};

        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return("/tmp"));
        EXPECT_CALL(pmbus, read("mfr_status", Type::HwmonDeviceDebug, true))
            .Times(1)
            .WillOnce(Return(0x123456789abcull));

        // Call findPgoodFault() which calls storePgoodFaultDebugData()
        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        std::string error =
            device.findPgoodFault(services, powerSupplyError, additionalData);
        EXPECT_EQ(error,
                  "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault");
        EXPECT_EQ(additionalData.size(), 6);
        EXPECT_EQ(additionalData["MFR_STATUS"], "0x123456789abc");
        EXPECT_EQ(additionalData["DEVICE_NAME"], "ucd90320");
        EXPECT_EQ(additionalData["GPIO_VALUES"], "[1, 1, 0]");
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD");
        EXPECT_EQ(additionalData["GPIO_LINE"], "2");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "0");
    }

    // Test where exception thrown trying to get MFR_STATUS
    {
        MockServices services;
        std::vector<int> gpioValues{1, 1, 0};
        EXPECT_CALL(services, getGPIOValues("ucd90320"))
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(services,
                    logInfoMsg("Device ucd90320 GPIO values: [1, 1, 0]"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device ucd90320"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD pgood GPIO line offset 2 has inactive value 0"))
            .Times(1);

        std::string name{"ucd90320"};
        uint8_t bus{3};
        uint16_t address{0x72};
        std::string powerControlGPIOName{"power-chassis-control"};
        std::string powerGoodGPIOName{"power-chassis-good"};
        std::vector<std::unique_ptr<Rail>> rails;
        rails.emplace_back(createRail("VDD", 2));
        UCD90xDevice device{name,
                            bus,
                            address,
                            powerControlGPIOName,
                            powerGoodGPIOName,
                            std::move(rails)};

        device.open(services);
        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return("/tmp"));
        EXPECT_CALL(pmbus, read("mfr_status", Type::HwmonDeviceDebug, true))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));

        // Call findPgoodFault() which calls storePgoodFaultDebugData()
        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        std::string error =
            device.findPgoodFault(services, powerSupplyError, additionalData);
        EXPECT_EQ(error,
                  "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault");
        EXPECT_EQ(additionalData.size(), 5);
        EXPECT_EQ(additionalData["DEVICE_NAME"], "ucd90320");
        EXPECT_EQ(additionalData["GPIO_VALUES"], "[1, 1, 0]");
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD");
        EXPECT_EQ(additionalData["GPIO_LINE"], "2");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "0");
    }
}
