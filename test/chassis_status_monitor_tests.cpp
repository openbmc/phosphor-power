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

#include "chassis_status_monitor.hpp"

#include <stddef.h> // for size_t

#include <sdbusplus/bus.hpp>

#include <string>

#include <gtest/gtest.h>

namespace phosphor::power::util
{
bool operator==(const ChassisStatusMonitorOptions& lhs,
                const ChassisStatusMonitorOptions& rhs)
{
    return (
        (lhs.isPresentMonitored == rhs.isPresentMonitored) &&
        (lhs.isAvailableMonitored == rhs.isAvailableMonitored) &&
        (lhs.isEnabledMonitored == rhs.isEnabledMonitored) &&
        (lhs.isPowerStateMonitored == rhs.isPowerStateMonitored) &&
        (lhs.isPowerGoodMonitored == rhs.isPowerGoodMonitored) &&
        (lhs.isInputPowerStatusMonitored == rhs.isInputPowerStatusMonitored) &&
        (lhs.isPowerSuppliesStatusMonitored ==
         rhs.isPowerSuppliesStatusMonitored));
}
} // namespace phosphor::power::util

using namespace phosphor::power::util;

TEST(ChassisStatusMonitorOptionsTests, DefaultConstructor)
{
    ChassisStatusMonitorOptions options;
    EXPECT_FALSE(options.isPresentMonitored);
    EXPECT_FALSE(options.isAvailableMonitored);
    EXPECT_FALSE(options.isEnabledMonitored);
    EXPECT_FALSE(options.isPowerStateMonitored);
    EXPECT_FALSE(options.isPowerGoodMonitored);
    EXPECT_FALSE(options.isInputPowerStatusMonitored);
    EXPECT_FALSE(options.isPowerSuppliesStatusMonitored);
}

TEST(BMCChassisStatusMonitorTests, Constructor)
{
    auto bus = sdbusplus::bus::new_default();
    size_t number{2};
    std::string inventoryPath{
        "/xyz/openbmc_project/inventory/system/chassis_two"};
    ChassisStatusMonitorOptions options;
    options.isPresentMonitored = true;
    options.isAvailableMonitored = false;
    options.isEnabledMonitored = true;
    options.isPowerStateMonitored = true;
    options.isPowerGoodMonitored = true;
    options.isInputPowerStatusMonitored = false;
    options.isPowerSuppliesStatusMonitored = true;
    BMCChassisStatusMonitor monitor{bus, number, inventoryPath, options};

    EXPECT_EQ(monitor.getNumber(), number);
    EXPECT_EQ(monitor.getInventoryPath(), inventoryPath);
    EXPECT_EQ(monitor.getOptions(), options);
}

TEST(BMCChassisStatusMonitorTests, GetNumber)
{
    auto bus = sdbusplus::bus::new_default();
    size_t number{3};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis3"};
    ChassisStatusMonitorOptions options;
    BMCChassisStatusMonitor monitor{bus, number, inventoryPath, options};

    EXPECT_EQ(monitor.getNumber(), number);
}

TEST(BMCChassisStatusMonitorTests, GetInventoryPath)
{
    auto bus = sdbusplus::bus::new_default();
    size_t number{3};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis3"};
    ChassisStatusMonitorOptions options;
    BMCChassisStatusMonitor monitor{bus, number, inventoryPath, options};

    EXPECT_EQ(monitor.getInventoryPath(), inventoryPath);
}

TEST(BMCChassisStatusMonitorTests, GetOptions)
{
    auto bus = sdbusplus::bus::new_default();
    size_t number{1};
    std::string inventoryPath{"/xyz/openbmc_project/inventory/system/chassis"};
    ChassisStatusMonitorOptions options;
    options.isPresentMonitored = false;
    options.isAvailableMonitored = true;
    options.isEnabledMonitored = false;
    options.isPowerStateMonitored = false;
    options.isPowerGoodMonitored = false;
    options.isInputPowerStatusMonitored = true;
    options.isPowerSuppliesStatusMonitored = false;
    BMCChassisStatusMonitor monitor{bus, number, inventoryPath, options};

    EXPECT_EQ(monitor.getOptions(), options);
}
