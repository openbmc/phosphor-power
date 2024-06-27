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

#include "mock_device.hpp"
#include "mock_services.hpp"
#include "rail.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

using ::testing::Return;
using ::testing::Throw;

TEST(GPIOTests, Initialization)
{
    // Default initialization
    {
        GPIO gpio;
        EXPECT_EQ(gpio.line, 0);
        EXPECT_FALSE(gpio.activeLow);
    }

    // Explicit initialization
    {
        GPIO gpio{48, true};
        EXPECT_EQ(gpio.line, 48);
        EXPECT_TRUE(gpio.activeLow);
    }
}

TEST(RailTests, Constructor)
{
    // Test where succeeds: No optional parameters have values
    {
        std::string name{"12.0V"};
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{};
        bool isPowerSupplyRail{true};
        bool checkStatusVout{false};
        bool compareVoltageToLimit{false};
        std::optional<GPIO> gpio{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        EXPECT_EQ(rail.getName(), "12.0V");
        EXPECT_FALSE(rail.getPresence().has_value());
        EXPECT_FALSE(rail.getPage().has_value());
        EXPECT_TRUE(rail.isPowerSupplyRail());
        EXPECT_FALSE(rail.getCheckStatusVout());
        EXPECT_FALSE(rail.getCompareVoltageToLimit());
        EXPECT_FALSE(rail.getGPIO().has_value());
    }

    // Test where succeeds: All optional parameters have values
    {
        std::string name{"VCS_CPU1"};
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1"};
        std::optional<uint8_t> page{11};
        bool isPowerSupplyRail{false};
        bool checkStatusVout{true};
        bool compareVoltageToLimit{true};
        std::optional<GPIO> gpio{GPIO(60, true)};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        EXPECT_EQ(rail.getName(), "VCS_CPU1");
        EXPECT_TRUE(rail.getPresence().has_value());
        EXPECT_EQ(
            rail.getPresence().value(),
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1");
        EXPECT_TRUE(rail.getPage().has_value());
        EXPECT_EQ(rail.getPage().value(), 11);
        EXPECT_FALSE(rail.isPowerSupplyRail());
        EXPECT_TRUE(rail.getCheckStatusVout());
        EXPECT_TRUE(rail.getCompareVoltageToLimit());
        EXPECT_TRUE(rail.getGPIO().has_value());
        EXPECT_EQ(rail.getGPIO().value().line, 60);
        EXPECT_TRUE(rail.getGPIO().value().activeLow);
    }

    // Test where fails: checkStatusVout is true and page has no value
    {
        std::string name{"VDD1"};
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{};
        bool isPowerSupplyRail{false};
        bool checkStatusVout{true};
        bool compareVoltageToLimit{false};
        std::optional<GPIO> gpio{};
        EXPECT_THROW((Rail{name, presence, page, isPowerSupplyRail,
                           checkStatusVout, compareVoltageToLimit, gpio}),
                     std::invalid_argument);
    }

    // Test where fails: compareVoltageToLimit is true and page has no value
    {
        std::string name{"VDD1"};
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{};
        bool isPowerSupplyRail{false};
        bool checkStatusVout{false};
        bool compareVoltageToLimit{true};
        std::optional<GPIO> gpio{};
        EXPECT_THROW((Rail{name, presence, page, isPowerSupplyRail,
                           checkStatusVout, compareVoltageToLimit, gpio}),
                     std::invalid_argument);
    }
}

TEST(RailTests, GetName)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};
    Rail rail{name,
              presence,
              page,
              isPowerSupplyRail,
              checkStatusVout,
              compareVoltageToLimit,
              gpio};

    EXPECT_EQ(rail.getName(), "VDD2");
}

TEST(RailTests, GetPresence)
{
    std::string name{"VDDR2"};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};

    // Test where presence has no value
    {
        std::optional<std::string> presence{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};
        EXPECT_FALSE(rail.getPresence().has_value());
    }

    // Test where presence has a value
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm2"};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};
        EXPECT_TRUE(rail.getPresence().has_value());
        EXPECT_EQ(
            rail.getPresence().value(),
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm2");
    }
}

TEST(RailTests, GetPage)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};

    // Test where page has no value
    {
        std::optional<uint8_t> page{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};
        EXPECT_FALSE(rail.getPage().has_value());
    }

    // Test where page has a value
    {
        std::optional<uint8_t> page{7};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};
        EXPECT_TRUE(rail.getPage().has_value());
        EXPECT_EQ(rail.getPage().value(), 7);
    }
}

TEST(RailTests, IsPowerSupplyRail)
{
    std::string name{"12.0V"};
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{true};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};
    Rail rail{name,
              presence,
              page,
              isPowerSupplyRail,
              checkStatusVout,
              compareVoltageToLimit,
              gpio};

    EXPECT_TRUE(rail.isPowerSupplyRail());
}

TEST(RailTests, GetCheckStatusVout)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};
    Rail rail{name,
              presence,
              page,
              isPowerSupplyRail,
              checkStatusVout,
              compareVoltageToLimit,
              gpio};

    EXPECT_FALSE(rail.getCheckStatusVout());
}

TEST(RailTests, GetCompareVoltageToLimit)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{13};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{true};
    std::optional<GPIO> gpio{};
    Rail rail{name,
              presence,
              page,
              isPowerSupplyRail,
              checkStatusVout,
              compareVoltageToLimit,
              gpio};

    EXPECT_TRUE(rail.getCompareVoltageToLimit());
}

TEST(RailTests, GetGPIO)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};

    // Test where gpio has no value
    {
        std::optional<GPIO> gpio{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};
        EXPECT_FALSE(rail.getGPIO().has_value());
    }

    // Test where gpio has a value
    {
        std::optional<GPIO> gpio{GPIO(12, false)};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};
        EXPECT_TRUE(rail.getGPIO().has_value());
        EXPECT_EQ(rail.getGPIO().value().line, 12);
        EXPECT_FALSE(rail.getGPIO().value().activeLow);
    }
}

TEST(RailTests, IsPresent)
{
    std::string name{"VDD2"};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};

    // Test where inventory path not specified; always returns true
    {
        std::optional<std::string> presence{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockServices services{};
        EXPECT_CALL(services, isPresent).Times(0);

        EXPECT_TRUE(rail.isPresent(services));
    }

    // Test where inventory path is not present
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Return(false));

        EXPECT_FALSE(rail.isPresent(services));
    }

    // Test where inventory path is present
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_TRUE(rail.isPresent(services));
    }

    // Test where exception occurs trying to get presence
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"Invalid object path"}));

        try
        {
            rail.isPresent(services);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to determine presence of rail VDD2 using "
                "inventory path "
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2: "
                "Invalid object path");
        }
    }
}

TEST(RailTests, GetStatusWord)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};

    // Test where page was not specified: Throws exception
    {
        std::optional<uint8_t> page{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusWord).Times(0);

        try
        {
            rail.getStatusWord(device);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read STATUS_WORD value for rail VDD2: "
                         "No PAGE number defined for rail VDD2");
        }
    }

    // Test where value read successfully
    {
        std::optional<uint8_t> page{2};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusWord(2)).Times(1).WillOnce(Return(0xbeef));

        EXPECT_EQ(rail.getStatusWord(device), 0xbeef);
    }

    // Test where exception occurs trying to read value
    {
        std::optional<uint8_t> page{2};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusWord(2))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));

        try
        {
            rail.getStatusWord(device);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read STATUS_WORD value for rail VDD2: "
                         "File does not exist");
        }
    }
}

TEST(RailTests, GetStatusVout)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};

    // Test where page was not specified: Throws exception
    {
        std::optional<uint8_t> page{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout).Times(0);

        try
        {
            rail.getStatusVout(device);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read STATUS_VOUT value for rail VDD2: "
                         "No PAGE number defined for rail VDD2");
        }
    }

    // Test where value read successfully
    {
        std::optional<uint8_t> page{2};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(2)).Times(1).WillOnce(Return(0xad));

        EXPECT_EQ(rail.getStatusVout(device), 0xad);
    }

    // Test where exception occurs trying to read value
    {
        std::optional<uint8_t> page{2};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(2))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));

        try
        {
            rail.getStatusVout(device);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read STATUS_VOUT value for rail VDD2: "
                         "File does not exist");
        }
    }
}

TEST(RailTests, GetReadVout)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};

    // Test where page was not specified: Throws exception
    {
        std::optional<uint8_t> page{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout).Times(0);

        try
        {
            rail.getReadVout(device);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read READ_VOUT value for rail VDD2: "
                         "No PAGE number defined for rail VDD2");
        }
    }

    // Test where value read successfully
    {
        std::optional<uint8_t> page{2};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2)).Times(1).WillOnce(Return(1.23));

        EXPECT_EQ(rail.getReadVout(device), 1.23);
    }

    // Test where exception occurs trying to read value
    {
        std::optional<uint8_t> page{2};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));

        try
        {
            rail.getReadVout(device);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read READ_VOUT value for rail VDD2: "
                         "File does not exist");
        }
    }
}

TEST(RailTests, GetVoutUVFaultLimit)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};

    // Test where page was not specified: Throws exception
    {
        std::optional<uint8_t> page{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getVoutUVFaultLimit).Times(0);

        try
        {
            rail.getVoutUVFaultLimit(device);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to read VOUT_UV_FAULT_LIMIT value for rail VDD2: "
                "No PAGE number defined for rail VDD2");
        }
    }

    // Test where value read successfully
    {
        std::optional<uint8_t> page{2};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getVoutUVFaultLimit(2))
            .Times(1)
            .WillOnce(Return(0.9));

        EXPECT_EQ(rail.getVoutUVFaultLimit(device), 0.9);
    }

    // Test where exception occurs trying to read value
    {
        std::optional<uint8_t> page{2};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getVoutUVFaultLimit(2))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));

        try
        {
            rail.getVoutUVFaultLimit(device);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(
                e.what(),
                "Unable to read VOUT_UV_FAULT_LIMIT value for rail VDD2: "
                "File does not exist");
        }
    }
}

TEST(RailTests, HasPgoodFault)
{
    std::string name{"VDD2"};
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{2};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{true};
    bool compareVoltageToLimit{true};
    bool activeLow{true};
    std::optional<GPIO> gpio{GPIO(3, activeLow)};
    Rail rail{name,
              presence,
              page,
              isPowerSupplyRail,
              checkStatusVout,
              compareVoltageToLimit,
              gpio};

    // No fault detected
    {
        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(2)).Times(1).WillOnce(Return(0x00));
        EXPECT_CALL(device, getReadVout(2)).Times(1).WillOnce(Return(1.1));
        EXPECT_CALL(device, getVoutUVFaultLimit(2))
            .Times(1)
            .WillOnce(Return(1.0));

        MockServices services{};

        std::vector<int> gpioValues{0, 0, 0, 0, 0, 0};
        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFault(device, services, gpioValues, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Fault detected via STATUS_VOUT
    {
        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(2)).Times(1).WillOnce(Return(0x10));
        EXPECT_CALL(device, getReadVout(2)).Times(0);
        EXPECT_CALL(device, getStatusWord(2)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(services, logInfoMsg("Rail VDD2 STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD2"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg("Rail VDD2 has fault bits set in STATUS_VOUT: 0x10"))
            .Times(1);

        std::vector<int> gpioValues{0, 0, 0, 0, 0, 0};
        std::map<std::string, std::string> additionalData{};
        EXPECT_TRUE(
            rail.hasPgoodFault(device, services, gpioValues, additionalData));
        EXPECT_EQ(additionalData.size(), 3);
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD2");
        EXPECT_EQ(additionalData["STATUS_VOUT"], "0x10");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }

    // Fault detected via GPIO
    {
        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(2)).Times(1).WillOnce(Return(0x00));
        EXPECT_CALL(device, getReadVout(2)).Times(0);
        EXPECT_CALL(device, getStatusWord(2)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(services, logInfoMsg("Rail VDD2 STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD2"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD2 pgood GPIO line offset 3 has inactive value 1"))
            .Times(1);

        std::vector<int> gpioValues{0, 0, 0, 1, 0, 0};
        std::map<std::string, std::string> additionalData{};
        EXPECT_TRUE(
            rail.hasPgoodFault(device, services, gpioValues, additionalData));
        EXPECT_EQ(additionalData.size(), 4);
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD2");
        EXPECT_EQ(additionalData["GPIO_LINE"], "3");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "1");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }

    // Fault detected via output voltage
    {
        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(2)).Times(1).WillOnce(Return(0x00));
        EXPECT_CALL(device, getReadVout(2)).Times(1).WillOnce(Return(1.1));
        EXPECT_CALL(device, getVoutUVFaultLimit(2))
            .Times(1)
            .WillOnce(Return(1.1));
        EXPECT_CALL(device, getStatusWord(2)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(services, logInfoMsg("Rail VDD2 STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD2"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD2 output voltage 1.1V is <= UV fault limit 1.1V"))
            .Times(1);

        std::vector<int> gpioValues{0, 0, 0, 0, 0, 0};
        std::map<std::string, std::string> additionalData{};
        EXPECT_TRUE(
            rail.hasPgoodFault(device, services, gpioValues, additionalData));
        EXPECT_EQ(additionalData.size(), 4);
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD2");
        EXPECT_EQ(additionalData["READ_VOUT"], "1.1");
        EXPECT_EQ(additionalData["VOUT_UV_FAULT_LIMIT"], "1.1");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }
}

TEST(RailTests, HasPgoodFaultStatusVout)
{
    std::string name{"VDD2"};
    std::optional<uint8_t> page{3};
    bool isPowerSupplyRail{false};
    bool compareVoltageToLimit{false};
    std::optional<GPIO> gpio{};

    // Test where presence check defined: Rail is not present
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        bool checkStatusVout{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(3)).Times(0);

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Return(false));

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultStatusVout(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where presence check defined: Rail is present: No fault detected
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        bool checkStatusVout{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(3)).Times(1).WillOnce(Return(0x00));

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Return(true));

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultStatusVout(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where STATUS_VOUT check is not defined
    {
        std::optional<std::string> presence{};
        bool checkStatusVout{false};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(3)).Times(0);

        MockServices services{};

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultStatusVout(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where no fault detected: No warning bits set
    {
        std::optional<std::string> presence{};
        bool checkStatusVout{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(3)).Times(1).WillOnce(Return(0x00));

        MockServices services{};
        EXPECT_CALL(services, logInfoMsg).Times(0);

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultStatusVout(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where no fault detected: Warning bits set
    {
        std::optional<std::string> presence{};
        bool checkStatusVout{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(3)).Times(1).WillOnce(Return(0x6a));

        MockServices services{};
        EXPECT_CALL(
            services,
            logInfoMsg("Rail VDD2 has warning bits set in STATUS_VOUT: 0x6a"))
            .Times(1);

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultStatusVout(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where fault detected
    // STATUS_WORD captured in additional data
    {
        std::optional<std::string> presence{};
        bool checkStatusVout{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(3)).Times(1).WillOnce(Return(0x10));
        EXPECT_CALL(device, getStatusWord(3)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(services, logInfoMsg("Rail VDD2 STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD2"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg("Rail VDD2 has fault bits set in STATUS_VOUT: 0x10"))
            .Times(1);

        std::map<std::string, std::string> additionalData{};
        EXPECT_TRUE(
            rail.hasPgoodFaultStatusVout(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 3);
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD2");
        EXPECT_EQ(additionalData["STATUS_VOUT"], "0x10");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }

    // Test where exception thrown
    {
        std::optional<std::string> presence{};
        bool checkStatusVout{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusVout(3))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));

        MockServices services{};

        std::map<std::string, std::string> additionalData{};
        try
        {
            rail.hasPgoodFaultStatusVout(device, services, additionalData);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read STATUS_VOUT value for rail VDD2: "
                         "File does not exist");
        }
    }
}

TEST(RailTests, HasPgoodFaultGPIO)
{
    std::string name{"VDD2"};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};

    // Test where presence check defined: Rail is not present
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        std::optional<uint8_t> page{3};
        bool activeLow{false};
        std::optional<GPIO> gpio{GPIO(3, activeLow)};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Return(false));

        std::vector<int> gpioValues{1, 1, 1, 0, 1, 1};
        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(rail.hasPgoodFaultGPIO(device, services, gpioValues,
                                            additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where presence check defined: Rail is present: No fault detected
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        std::optional<uint8_t> page{3};
        bool activeLow{false};
        std::optional<GPIO> gpio{GPIO(3, activeLow)};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Return(true));

        std::vector<int> gpioValues{1, 1, 1, 1, 1, 1};
        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(rail.hasPgoodFaultGPIO(device, services, gpioValues,
                                            additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where GPIO check not defined
    {
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{3};
        std::optional<GPIO> gpio{};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};

        MockServices services{};

        std::vector<int> gpioValues{0, 0, 0, 0, 0, 0};
        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(rail.hasPgoodFaultGPIO(device, services, gpioValues,
                                            additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where no fault detected
    // GPIO value is 1 and GPIO is active high
    {
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{};
        bool activeLow{false};
        std::optional<GPIO> gpio{GPIO(3, activeLow)};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};

        MockServices services{};

        std::vector<int> gpioValues{1, 1, 1, 1, 1, 1};
        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(rail.hasPgoodFaultGPIO(device, services, gpioValues,
                                            additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where fault detected
    // GPIO value is 0 and GPIO is active high
    // STATUS_WORD not captured since no PMBus page defined
    {
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{};
        bool activeLow{false};
        std::optional<GPIO> gpio{GPIO(3, activeLow)};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};

        MockServices services{};
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD2"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD2 pgood GPIO line offset 3 has inactive value 0"))
            .Times(1);

        std::vector<int> gpioValues{1, 1, 1, 0, 1, 1};
        std::map<std::string, std::string> additionalData{};
        EXPECT_TRUE(rail.hasPgoodFaultGPIO(device, services, gpioValues,
                                           additionalData));
        EXPECT_EQ(additionalData.size(), 3);
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD2");
        EXPECT_EQ(additionalData["GPIO_LINE"], "3");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "0");
    }

    // Test where fault detected
    // GPIO value is 1 and GPIO is active low
    {
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{2};
        bool activeLow{true};
        std::optional<GPIO> gpio{GPIO(3, activeLow)};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getStatusWord(2)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(services, logInfoMsg("Rail VDD2 STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD2"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD2 pgood GPIO line offset 3 has inactive value 1"))
            .Times(1);

        std::vector<int> gpioValues{0, 0, 0, 1, 0, 0};
        std::map<std::string, std::string> additionalData{};
        EXPECT_TRUE(rail.hasPgoodFaultGPIO(device, services, gpioValues,
                                           additionalData));
        EXPECT_EQ(additionalData.size(), 4);
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD2");
        EXPECT_EQ(additionalData["GPIO_LINE"], "3");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "1");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }

    // Test where exception thrown
    {
        std::optional<std::string> presence{};
        std::optional<uint8_t> page{};
        bool activeLow{false};
        std::optional<GPIO> gpio{GPIO(6, activeLow)};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};

        MockServices services{};

        std::vector<int> gpioValues{1, 1, 1, 1, 1, 1};
        std::map<std::string, std::string> additionalData{};
        try
        {
            rail.hasPgoodFaultGPIO(device, services, gpioValues,
                                   additionalData);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "Invalid GPIO line offset 6 for rail VDD2: "
                                   "Device only has 6 GPIO values");
        }
    }
}

TEST(RailTests, HasPgoodFaultOutputVoltage)
{
    std::string name{"VDD2"};
    std::optional<uint8_t> page{2};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    std::optional<GPIO> gpio{};

    // Test where presence check defined: Rail is not present
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        bool compareVoltageToLimit{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2)).Times(0);
        EXPECT_CALL(device, getVoutUVFaultLimit(2)).Times(0);

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Return(false));

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultOutputVoltage(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where presence check defined: Rail is present: No fault detected
    {
        std::optional<std::string> presence{
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2"};
        bool compareVoltageToLimit{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2)).Times(1).WillOnce(Return(1.1));
        EXPECT_CALL(device, getVoutUVFaultLimit(2))
            .Times(1)
            .WillOnce(Return(1.0));

        MockServices services{};
        EXPECT_CALL(services, isPresent(*presence))
            .Times(1)
            .WillOnce(Return(true));

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultOutputVoltage(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where voltage output check not specified
    {
        std::optional<std::string> presence{};
        bool compareVoltageToLimit{false};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2)).Times(0);
        EXPECT_CALL(device, getVoutUVFaultLimit(2)).Times(0);

        MockServices services{};

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultOutputVoltage(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where no fault detected: Output voltage > UV limit
    {
        std::optional<std::string> presence{};
        bool compareVoltageToLimit{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2)).Times(1).WillOnce(Return(1.1));
        EXPECT_CALL(device, getVoutUVFaultLimit(2))
            .Times(1)
            .WillOnce(Return(1.0));

        MockServices services{};

        std::map<std::string, std::string> additionalData{};
        EXPECT_FALSE(
            rail.hasPgoodFaultOutputVoltage(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 0);
    }

    // Test where fault detected: Output voltage < UV limit
    {
        std::optional<std::string> presence{};
        bool compareVoltageToLimit{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2)).Times(1).WillOnce(Return(1.1));
        EXPECT_CALL(device, getVoutUVFaultLimit(2))
            .Times(1)
            .WillOnce(Return(1.2));
        EXPECT_CALL(device, getStatusWord(2)).Times(1).WillOnce(Return(0xbeef));

        MockServices services{};
        EXPECT_CALL(services, logInfoMsg("Rail VDD2 STATUS_WORD: 0xbeef"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD2"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD2 output voltage 1.1V is <= UV fault limit 1.2V"))
            .Times(1);

        std::map<std::string, std::string> additionalData{};
        EXPECT_TRUE(
            rail.hasPgoodFaultOutputVoltage(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 4);
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD2");
        EXPECT_EQ(additionalData["READ_VOUT"], "1.1");
        EXPECT_EQ(additionalData["VOUT_UV_FAULT_LIMIT"], "1.2");
        EXPECT_EQ(additionalData["STATUS_WORD"], "0xbeef");
    }

    // Test where fault detected: Output voltage == UV limit
    // STATUS_WORD not captured because reading it caused an exception
    {
        std::optional<std::string> presence{};
        bool compareVoltageToLimit{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2)).Times(1).WillOnce(Return(1.1));
        EXPECT_CALL(device, getVoutUVFaultLimit(2))
            .Times(1)
            .WillOnce(Return(1.1));
        EXPECT_CALL(device, getStatusWord(2))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));

        MockServices services{};
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD2"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD2 output voltage 1.1V is <= UV fault limit 1.1V"))
            .Times(1);

        std::map<std::string, std::string> additionalData{};
        EXPECT_TRUE(
            rail.hasPgoodFaultOutputVoltage(device, services, additionalData));
        EXPECT_EQ(additionalData.size(), 3);
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD2");
        EXPECT_EQ(additionalData["READ_VOUT"], "1.1");
        EXPECT_EQ(additionalData["VOUT_UV_FAULT_LIMIT"], "1.1");
    }

    // Test where exception thrown
    {
        std::optional<std::string> presence{};
        bool compareVoltageToLimit{true};
        Rail rail{name,
                  presence,
                  page,
                  isPowerSupplyRail,
                  checkStatusVout,
                  compareVoltageToLimit,
                  gpio};

        MockDevice device{};
        EXPECT_CALL(device, getReadVout(2))
            .Times(1)
            .WillOnce(Throw(std::runtime_error{"File does not exist"}));

        MockServices services{};

        std::map<std::string, std::string> additionalData{};
        try
        {
            rail.hasPgoodFaultOutputVoltage(device, services, additionalData);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_STREQ(e.what(),
                         "Unable to read READ_VOUT value for rail VDD2: "
                         "File does not exist");
        }
    }
}
