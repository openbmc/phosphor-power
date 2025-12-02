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

#include "gpios_only_device.hpp"
#include "mock_gpio.hpp"
#include "mock_services.hpp"
#include "rail.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

using ::testing::Return;
using ::testing::Throw;

TEST(GPIOsOnlyDeviceTests, Constructor)
{
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

    EXPECT_EQ(device.getName(), GPIOsOnlyDevice::deviceName);
    EXPECT_EQ(device.getBus(), 0);
    EXPECT_EQ(device.getAddress(), 0x00);
    EXPECT_EQ(device.getPowerControlGPIOName(), powerControlGPIOName);
    EXPECT_EQ(device.getPowerGoodGPIOName(), powerGoodGPIOName);
    EXPECT_TRUE(device.getRails().empty());
}

TEST(GPIOsOnlyDeviceTests, GetGPIOValues)
{
    try
    {
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

        MockServices services{};
        device.open(services);
        device.getGPIOValues(services);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "getGPIOValues() is not supported");
    }
}

TEST(GPIOsOnlyDeviceTests, GetStatusWord)
{
    try
    {
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

        MockServices services{};
        device.open(services);
        device.getStatusWord(0);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "getStatusWord() is not supported");
    }
}

TEST(GPIOsOnlyDeviceTests, GetStatusVout)
{
    try
    {
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

        MockServices services{};
        device.open(services);
        device.getStatusVout(0);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "getStatusVout() is not supported");
    }
}

TEST(GPIOsOnlyDeviceTests, GetReadVout)
{
    try
    {
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

        MockServices services{};
        device.open(services);
        device.getReadVout(0);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "getReadVout() is not supported");
    }
}

TEST(GPIOsOnlyDeviceTests, GetVoutUVFaultLimit)
{
    try
    {
        std::string powerControlGPIOName{"power-on"};
        std::string powerGoodGPIOName{"chassis-pgood"};
        GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

        MockServices services{};
        device.open(services);
        device.getVoutUVFaultLimit(0);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "getVoutUVFaultLimit() is not supported");
    }
}

TEST(GPIOsOnlyDeviceTests, FindPgoodFault)
{
    std::string powerControlGPIOName{"power-on"};
    std::string powerGoodGPIOName{"chassis-pgood"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

    MockServices services{};
    std::string powerSupplyError{};
    std::map<std::string, std::string> additionalData{};

    // Test where fails: Device not open
    try
    {
        device.findPgoodFault(services, powerSupplyError, additionalData);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ(e.what(), "Device not open: gpios_only_device");
    }

    // Test where works
    {
        device.open(services);
        std::string error =
            device.findPgoodFault(services, powerSupplyError, additionalData);
        EXPECT_TRUE(error.empty());
        EXPECT_EQ(additionalData.size(), 0);
    }
}
