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

#include "rail.hpp"

#include <cstdint>
#include <optional>
#include <string>

#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

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
