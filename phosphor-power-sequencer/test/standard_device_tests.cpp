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

#include "mock_services.hpp"
#include "rail.hpp"
#include "standard_device.hpp"

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
 * @class StandardDeviceImpl
 *
 * Concrete subclass of the StandardDevice abstract class.
 *
 * This subclass is required for two reasons:
 * - StandardDevice has some pure virtual methods so it cannot be instantiated.
 * - The pure virtual methods provide the PMBus and GPIO information.  Mocking
 *   these makes it possible to test the pgood fault detection algorithm.
 *
 * This class is not intended to be used outside of this file.  It is
 * implementation detail for testing the the StandardDevice class.
 */
class StandardDeviceImpl : public StandardDevice
{
  public:
    // Specify which compiler-generated methods we want
    StandardDeviceImpl() = delete;
    StandardDeviceImpl(const StandardDeviceImpl&) = delete;
    StandardDeviceImpl(StandardDeviceImpl&&) = delete;
    StandardDeviceImpl& operator=(const StandardDeviceImpl&) = delete;
    StandardDeviceImpl& operator=(StandardDeviceImpl&&) = delete;
    virtual ~StandardDeviceImpl() = default;

    // Constructor just calls StandardDevice constructor
    explicit StandardDeviceImpl(const std::string& name,
                                std::vector<std::unique_ptr<Rail>> rails) :
        StandardDevice(name, std::move(rails))
    {}

    // Mock pure virtual methods
    MOCK_METHOD(std::vector<int>, getGPIOValues, (), (override));
    MOCK_METHOD(uint16_t, getStatusWord, (uint8_t page), (override));
    MOCK_METHOD(uint8_t, getStatusVout, (uint8_t page), (override));
    MOCK_METHOD(double, getReadVout, (uint8_t page), (override));
    MOCK_METHOD(double, getVoutUVFaultLimit, (uint8_t page), (override));
};

/**
 * Creates a Rail object that checks for a pgood fault using STATUS_VOUT.
 *
 * @param name Unique name for the rail
 * @param isPowerSupplyRail Specifies whether the rail is produced by a
                            power supply
 * @param pageNum PMBus PAGE number of the rail
 * @return Rail object
 */
std::unique_ptr<Rail> createRailStatusVout(const std::string& name,
                                           bool isPowerSupplyRail,
                                           uint8_t pageNum)
{
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{pageNum};
    bool checkStatusVout{true};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};
    return std::make_unique<Rail>(name, presence, page, isPowerSupplyRail,
                                  checkStatusVout, compareVoltageToLimit, gpio);
}

/**
 * Creates a Rail object that checks for a pgood fault using a GPIO.
 *
 * @param name Unique name for the rail
 * @param isPowerSupplyRail Specifies whether the rail is produced by a
                            power supply
 * @param gpio GPIO line to read to determine the pgood status of the rail
 * @return Rail object
 */
std::unique_ptr<Rail> createRailGPIO(const std::string& name,
                                     bool isPowerSupplyRail,
                                     unsigned int gpioLine)
{
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    bool activeLow{false};
    std::optional<GPIO> gpio{GPIO{gpioLine, activeLow}};
    return std::make_unique<Rail>(name, presence, page, isPowerSupplyRail,
                                  checkStatusVout, compareVoltageToLimit, gpio);
}

TEST(StandardDeviceTests, Constructor)
{
    // Empty vector of rails
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        StandardDeviceImpl device{"xyz_pseq", std::move(rails)};

        EXPECT_EQ(device.getName(), "xyz_pseq");
        EXPECT_TRUE(device.getRails().empty());
    }

    // Non-empty vector of rails
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailGPIO("PSU", true, 3));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        EXPECT_EQ(device.getName(), "abc_pseq");
        EXPECT_EQ(device.getRails().size(), 3);
        EXPECT_EQ(device.getRails()[0]->getName(), "PSU");
        EXPECT_EQ(device.getRails()[1]->getName(), "VDD");
        EXPECT_EQ(device.getRails()[2]->getName(), "VIO");
    }
}

TEST(StandardDeviceTests, GetName)
{
    std::vector<std::unique_ptr<Rail>> rails{};
    StandardDeviceImpl device{"xyz_pseq", std::move(rails)};

    EXPECT_EQ(device.getName(), "xyz_pseq");
}

TEST(StandardDeviceTests, GetRails)
{
    // Empty vector of rails
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        StandardDeviceImpl device{"xyz_pseq", std::move(rails)};

        EXPECT_TRUE(device.getRails().empty());
    }

    // Non-empty vector of rails
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailGPIO("PSU", true, 3));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        EXPECT_EQ(device.getRails().size(), 3);
        EXPECT_EQ(device.getRails()[0]->getName(), "PSU");
        EXPECT_EQ(device.getRails()[1]->getName(), "VDD");
        EXPECT_EQ(device.getRails()[2]->getName(), "VIO");
    }
}

TEST(StandardDeviceTests, FindPgoodFault)
{
    // No rail has a pgood fault
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailGPIO("PSU", true, 2));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        std::vector<int> gpioValues{1, 1, 1};
        EXPECT_CALL(device, getGPIOValues)
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(device, getStatusVout(5)).Times(1).WillOnce(Return(0x00));
        EXPECT_CALL(device, getStatusVout(7)).Times(1).WillOnce(Return(0x00));

        MockServices services{};

        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        std::string error = device.findPgoodFault(services, powerSupplyError,
                                                  additionalData);
        EXPECT_TRUE(error.empty());
        EXPECT_EQ(additionalData.size(), 0);
    }

    // First rail has a pgood fault
    // Is a PSU rail: No PSU error specified
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailGPIO("PSU", true, 2));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        std::vector<int> gpioValues{1, 1, 0};
        EXPECT_CALL(device, getGPIOValues)
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(device, getStatusVout).Times(0);

        MockServices services{};
        EXPECT_CALL(services,
                    logInfoMsg("Device abc_pseq GPIO values: [1, 1, 0]"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device abc_pseq"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail PSU"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail PSU pgood GPIO line offset 2 has inactive value 0"))
            .Times(1);

        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        std::string error = device.findPgoodFault(services, powerSupplyError,
                                                  additionalData);
        EXPECT_EQ(error,
                  "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault");
        EXPECT_EQ(additionalData.size(), 5);
        EXPECT_EQ(additionalData["DEVICE_NAME"], "abc_pseq");
        EXPECT_EQ(additionalData["GPIO_VALUES"], "[1, 1, 0]");
        EXPECT_EQ(additionalData["RAIL_NAME"], "PSU");
        EXPECT_EQ(additionalData["GPIO_LINE"], "2");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "0");
    }

    // First rail has a pgood fault
    // Is a PSU rail: PSU error specified
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailGPIO("PSU", true, 2));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        std::vector<int> gpioValues{1, 1, 0};
        EXPECT_CALL(device, getGPIOValues)
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(device, getStatusVout).Times(0);

        MockServices services{};
        EXPECT_CALL(services,
                    logInfoMsg("Device abc_pseq GPIO values: [1, 1, 0]"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device abc_pseq"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail PSU"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail PSU pgood GPIO line offset 2 has inactive value 0"))
            .Times(1);

        std::string powerSupplyError{"Undervoltage fault: PSU1"};
        std::map<std::string, std::string> additionalData{};
        std::string error = device.findPgoodFault(services, powerSupplyError,
                                                  additionalData);
        EXPECT_EQ(error, powerSupplyError);
        EXPECT_EQ(additionalData.size(), 5);
        EXPECT_EQ(additionalData["DEVICE_NAME"], "abc_pseq");
        EXPECT_EQ(additionalData["GPIO_VALUES"], "[1, 1, 0]");
        EXPECT_EQ(additionalData["RAIL_NAME"], "PSU");
        EXPECT_EQ(additionalData["GPIO_LINE"], "2");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "0");
    }

    // Middle rail has a pgood fault
    // Not a PSU rail: PSU error specified
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailGPIO("PSU", true, 2));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        std::vector<int> gpioValues{1, 1, 1};
        EXPECT_CALL(device, getGPIOValues)
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(device, getStatusVout(5)).Times(1).WillOnce(Return(0x10));
        EXPECT_CALL(device, getStatusVout(7)).Times(0);
        EXPECT_CALL(device, getStatusWord(5)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(services,
                    logInfoMsg("Device abc_pseq GPIO values: [1, 1, 1]"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device abc_pseq"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Rail VDD STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg("Rail VDD has fault bits set in STATUS_VOUT: 0x10"))
            .Times(1);

        std::string powerSupplyError{"Undervoltage fault: PSU1"};
        std::map<std::string, std::string> additionalData{};
        std::string error = device.findPgoodFault(services, powerSupplyError,
                                                  additionalData);
        EXPECT_EQ(error,
                  "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault");
        EXPECT_EQ(additionalData.size(), 5);
        EXPECT_EQ(additionalData["DEVICE_NAME"], "abc_pseq");
        EXPECT_EQ(additionalData["GPIO_VALUES"], "[1, 1, 1]");
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD");
        EXPECT_EQ(additionalData["STATUS_VOUT"], "0x10");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }

    // Last rail has a pgood fault
    // Device returns 0 GPIO values
    // Does not halt pgood fault detection because GPIO values not used by rails
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailStatusVout("PSU", true, 3));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        std::vector<int> gpioValues{};
        EXPECT_CALL(device, getGPIOValues)
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(device, getStatusVout(3)).Times(1).WillOnce(Return(0x00));
        EXPECT_CALL(device, getStatusVout(5)).Times(1).WillOnce(Return(0x00));
        EXPECT_CALL(device, getStatusVout(7)).Times(1).WillOnce(Return(0x11));
        EXPECT_CALL(device, getStatusWord(7)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device abc_pseq"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Rail VIO STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VIO"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg("Rail VIO has fault bits set in STATUS_VOUT: 0x11"))
            .Times(1);

        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        std::string error = device.findPgoodFault(services, powerSupplyError,
                                                  additionalData);
        EXPECT_EQ(error,
                  "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault");
        EXPECT_EQ(additionalData.size(), 4);
        EXPECT_EQ(additionalData["DEVICE_NAME"], "abc_pseq");
        EXPECT_EQ(additionalData["RAIL_NAME"], "VIO");
        EXPECT_EQ(additionalData["STATUS_VOUT"], "0x11");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }

    // Last rail has a pgood fault
    // Exception occurs trying to obtain GPIO values from device
    // Does not halt pgood fault detection because GPIO values not used by rails
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailStatusVout("PSU", true, 3));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        EXPECT_CALL(device, getGPIOValues)
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"Unable to acquire GPIO line"}));
        EXPECT_CALL(device, getStatusVout(3)).Times(1).WillOnce(Return(0x00));
        EXPECT_CALL(device, getStatusVout(5)).Times(1).WillOnce(Return(0x00));
        EXPECT_CALL(device, getStatusVout(7)).Times(1).WillOnce(Return(0x11));
        EXPECT_CALL(device, getStatusWord(7)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device abc_pseq"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("Rail VIO STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VIO"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg("Rail VIO has fault bits set in STATUS_VOUT: 0x11"))
            .Times(1);

        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        std::string error = device.findPgoodFault(services, powerSupplyError,
                                                  additionalData);
        EXPECT_EQ(error,
                  "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault");
        EXPECT_EQ(additionalData.size(), 4);
        EXPECT_EQ(additionalData["DEVICE_NAME"], "abc_pseq");
        EXPECT_EQ(additionalData["RAIL_NAME"], "VIO");
        EXPECT_EQ(additionalData["STATUS_VOUT"], "0x11");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }

    // Exception is thrown during pgood fault detection
    {
        std::vector<std::unique_ptr<Rail>> rails{};
        rails.emplace_back(createRailGPIO("PSU", true, 2));
        rails.emplace_back(createRailStatusVout("VDD", false, 5));
        rails.emplace_back(createRailStatusVout("VIO", false, 7));
        StandardDeviceImpl device{"abc_pseq", std::move(rails)};

        std::vector<int> gpioValues{1, 1, 1};
        EXPECT_CALL(device, getGPIOValues)
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(device, getStatusVout(5))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));
        EXPECT_CALL(device, getStatusVout(7)).Times(0);

        MockServices services{};

        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        try
        {
            device.findPgoodFault(services, powerSupplyError, additionalData);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to determine if a pgood fault occurred in device abc_pseq: "
                "Unable to read STATUS_VOUT value for rail VDD: "
                "File does not exist");
        }
    }
}
