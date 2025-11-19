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
#include "mock_services.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;

TEST(GPIOsOnlyDeviceTests, Constructor)
{
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

    EXPECT_EQ(device.getPowerControlGPIOName(), powerControlGPIOName);
    EXPECT_EQ(device.getPowerGoodGPIOName(), powerGoodGPIOName);
}

TEST(GPIOsOnlyDeviceTests, GetName)
{
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

    EXPECT_EQ(device.getName(), GPIOsOnlyDevice::deviceName);
}

TEST(GPIOsOnlyDeviceTests, GetBus)
{
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

    EXPECT_EQ(device.getBus(), 0);
}

TEST(GPIOsOnlyDeviceTests, GetAddress)
{
    std::string powerControlGPIOName{"power-chassis-control"};
    std::string powerGoodGPIOName{"power-chassis-good"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

    EXPECT_EQ(device.getAddress(), 0);
}

TEST(GPIOsOnlyDeviceTests, GetPowerControlGPIOName)
{
    std::string powerControlGPIOName{"power-on"};
    std::string powerGoodGPIOName{"chassis-pgood"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

    EXPECT_EQ(device.getPowerControlGPIOName(), powerControlGPIOName);
}

TEST(GPIOsOnlyDeviceTests, GetPowerGoodGPIOName)
{
    std::string powerControlGPIOName{"power-on"};
    std::string powerGoodGPIOName{"chassis-pgood"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

    EXPECT_EQ(device.getPowerGoodGPIOName(), powerGoodGPIOName);
}

TEST(GPIOsOnlyDeviceTests, GetRails)
{
    std::string powerControlGPIOName{"power-on"};
    std::string powerGoodGPIOName{"chassis-pgood"};
    GPIOsOnlyDevice device{powerControlGPIOName, powerGoodGPIOName};

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
    std::string error =
        device.findPgoodFault(services, powerSupplyError, additionalData);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(additionalData.size(), 0);
}
