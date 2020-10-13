/**
 * Copyright © 2020 IBM Corporation
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
#include "action_environment.hpp"
#include "action_error.hpp"
#include "device.hpp"
#include "i2c_compare_byte_action.hpp"
#include "i2c_interface.hpp"
#include "id_map.hpp"
#include "mocked_i2c_interface.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::A;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::Throw;

TEST(I2CCompareByteActionTests, Constructor)
{
    // Test where mask is not specified
    {
        I2CCompareByteAction action{0x7C, 0xDE};
        EXPECT_EQ(action.getRegister(), 0x7C);
        EXPECT_EQ(action.getValue(), 0xDE);
        EXPECT_EQ(action.getMask(), 0xFF);
    }

    // Test where mask is specified
    {
        I2CCompareByteAction action{0xA0, 0x03, 0x47};
        EXPECT_EQ(action.getRegister(), 0xA0);
        EXPECT_EQ(action.getValue(), 0x03);
        EXPECT_EQ(action.getMask(), 0x47);
    }
}

TEST(I2CCompareByteActionTests, Execute)
{
    // Test where works: Equal: Mask specified
    try
    {
        // Create mock I2CInterface: read() returns value 0xD7
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0xD7));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Actual value: 0xD7 = 1101 0111
        // Mask        : 0x7E = 0111 1110
        // Result      : 0x56 = 0101 0110
        I2CCompareByteAction action{0xA0, 0x56, 0x7E};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Equal: Mask not specified
    try
    {
        // Create mock I2CInterface: read() returns value 0xD7
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0xD7));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        I2CCompareByteAction action{0xA0, 0xD7};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Not equal: Mask specified
    try
    {
        // Create mock I2CInterface: read() returns value 0xD7
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0xD7));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Actual value: 0xD7 = 1101 0111
        // Mask        : 0x7E = 0111 1110
        // Result      : 0x56 = 0101 0110
        I2CCompareByteAction action{0xA0, 0x57, 0x7E};
        EXPECT_EQ(action.execute(env), false);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Not equal: Mask not specified
    try
    {
        // Create mock I2CInterface: read() returns value 0xD7
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0xD7));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        I2CCompareByteAction action{0xA0, 0xD6};
        EXPECT_EQ(action.execute(env), false);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Getting I2CInterface fails
    try
    {
        // Create IDMap and ActionEnvironment
        IDMap idMap{};
        ActionEnvironment env{idMap, "reg1"};

        I2CCompareByteAction action{0xA0, 0xD6};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Unable to find device with ID \"reg1\"");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Reading byte fails
    try
    {
        // Create mock I2CInterface: read() throws an I2CException
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, A<uint8_t&>()))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to read byte", "/dev/i2c-1", 0x70}));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        I2CCompareByteAction action{0xA0, 0xD6};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(), "ActionError: i2c_compare_byte: { register: "
                               "0xA0, value: 0xD6, mask: 0xFF }");
        try
        {
            // Re-throw inner I2CException
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const i2c::I2CException& ie)
        {
            EXPECT_STREQ(
                ie.what(),
                "I2CException: Failed to read byte: bus /dev/i2c-1, addr 0x70");
        }
        catch (...)
        {
            ADD_FAILURE() << "Should not have caught exception.";
        }
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(I2CCompareByteActionTests, GetRegister)
{
    I2CCompareByteAction action{0x7C, 0xDE};
    EXPECT_EQ(action.getRegister(), 0x7C);
}

TEST(I2CCompareByteActionTests, GetValue)
{
    I2CCompareByteAction action{0xA0, 0x03, 0x47};
    EXPECT_EQ(action.getValue(), 0x03);
}

TEST(I2CCompareByteActionTests, GetMask)
{
    // Test where mask is not specified
    {
        I2CCompareByteAction action{0x7C, 0xDE};
        EXPECT_EQ(action.getMask(), 0xFF);
    }

    // Test where mask is specified
    {
        I2CCompareByteAction action{0xA0, 0x03, 0x47};
        EXPECT_EQ(action.getMask(), 0x47);
    }
}

TEST(I2CCompareByteActionTests, ToString)
{
    I2CCompareByteAction action{0x7C, 0xDE, 0xFE};
    EXPECT_EQ(action.toString(),
              "i2c_compare_byte: { register: 0x7C, value: 0xDE, mask: 0xFE }");
}
