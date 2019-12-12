/**
 * Copyright © 2019 IBM Corporation
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
#include "device.hpp"
#include "i2c_interface.hpp"
#include "mocked_i2c_interface.hpp"

#include <memory>
#include <utility>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

/**
 * Create an I2CInterface object with hard-coded bus and address values.
 *
 * @return I2CInterface object wrapped in a unique_ptr
 */
std::unique_ptr<i2c::I2CInterface> createI2CInterface()
{
    std::unique_ptr<i2c::I2CInterface> i2cInterface =
        i2c::create(1, 0x70, i2c::I2CInterface::InitialState::CLOSED);
    return i2cInterface;
}

TEST(DeviceTests, Constructor)
{
    std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
    i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();
    Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                  std::move(i2cInterface)};
    EXPECT_EQ(device.getID(), "vdd_reg");
    EXPECT_EQ(device.isRegulator(), true);
    EXPECT_EQ(device.getFRU(), "/system/chassis/motherboard/reg2");
    EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
}

TEST(DeviceTests, GetID)
{
    Device device{"vdd_reg", false, "/system/chassis/motherboard/reg2",
                  std::move(createI2CInterface())};
    EXPECT_EQ(device.getID(), "vdd_reg");
}

TEST(DeviceTests, IsRegulator)
{
    Device device{"vdd_reg", false, "/system/chassis/motherboard/reg2",
                  std::move(createI2CInterface())};
    EXPECT_EQ(device.isRegulator(), false);
}

TEST(DeviceTests, GetFRU)
{
    Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                  std::move(createI2CInterface())};
    EXPECT_EQ(device.getFRU(), "/system/chassis/motherboard/reg2");
}

TEST(DeviceTests, GetI2CInterface)
{
    std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
    i2c::I2CInterface* i2cInterfacePtr = i2cInterface.get();
    Device device{"vdd_reg", true, "/system/chassis/motherboard/reg2",
                  std::move(i2cInterface)};
    EXPECT_EQ(&(device.getI2CInterface()), i2cInterfacePtr);
}
