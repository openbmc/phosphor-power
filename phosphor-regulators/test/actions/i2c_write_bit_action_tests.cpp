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
#include "i2c_interface.hpp"
#include "i2c_write_bit_action.hpp"
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
using ::testing::TypedEq;

TEST(I2CWriteBitActionTests, Constructor)
{
    // Test where works
    try
    {
        I2CWriteBitAction action{0x7C, 2, 0};
        EXPECT_EQ(action.getRegister(), 0x7C);
        EXPECT_EQ(action.getPosition(), 2);
        EXPECT_EQ(action.getValue(), 0);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Invalid bit position > 7
    try
    {
        I2CWriteBitAction action{0x7C, 8, 0};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid bit position: 8");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Invalid bit value > 1
    try
    {
        I2CWriteBitAction action{0x7C, 2, 2};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid bit value: 2");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(I2CWriteBitActionTests, Execute)
{
    // Test where works: Value is 0
    try
    {
        // Create mock I2CInterface: read() returns value 0xB6
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0xB6));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0xA0), TypedEq<uint8_t>(0x96)))
            .Times(1);

        // Create Device, IDMap, and ActionEnvironment
        Device device{"reg1", true, "/system/chassis/motherboard/reg1",
                      std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Register value    : 0xB6 = 1011 0110
        // 0 in position 5   : 0x00 = --0- ----
        // New register value: 0x96 = 1001 0110
        I2CWriteBitAction action{0xA0, 5, 0};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Value is 1
    try
    {
        // Create mock I2CInterface: read() returns value 0x96
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0x7C, A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0x96));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x7C), TypedEq<uint8_t>(0xB6)))
            .Times(1);

        // Create Device, IDMap, and ActionEnvironment
        Device device{"reg1", true, "/system/chassis/motherboard/reg1",
                      std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Register value    : 0x96 = 1001 0110
        // 1 in position 5   : 0x20 = 0010 0000
        // New register value: 0xB6 = 1011 0110
        I2CWriteBitAction action{0x7C, 5, 1};
        EXPECT_EQ(action.execute(env), true);
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

        I2CWriteBitAction action{0x7C, 5, 1};
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
        EXPECT_CALL(*i2cInterface, read(0x7C, A<uint8_t&>()))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to read byte", "/dev/i2c-1", 0x70}));
        EXPECT_CALL(*i2cInterface, write(A<uint8_t>(), A<uint8_t>())).Times(0);

        // Create Device, IDMap, and ActionEnvironment
        Device device{"reg1", true, "/system/chassis/motherboard/reg1",
                      std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        I2CWriteBitAction action{0x7C, 5, 1};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(), "ActionError: i2c_write_bit: { register: "
                               "0x7C, position: 5, value: 1 }");
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

    // Test where fails: Writing byte fails
    try
    {
        // Create mock I2CInterface: read() returns value 0xB6, write() throws
        // an I2CException
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0xB6));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0xA0), TypedEq<uint8_t>(0x96)))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to write byte", "/dev/i2c-1", 0x70}));

        // Create Device, IDMap, and ActionEnvironment
        Device device{"reg1", true, "/system/chassis/motherboard/reg1",
                      std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Register value    : 0xB6 = 1011 0110
        // 0 in position 5   : 0x00 = --0- ----
        // New register value: 0x96 = 1001 0110
        I2CWriteBitAction action{0xA0, 5, 0};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(), "ActionError: i2c_write_bit: { register: "
                               "0xA0, position: 5, value: 0 }");
        try
        {
            // Re-throw inner I2CException
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const i2c::I2CException& ie)
        {
            EXPECT_STREQ(ie.what(), "I2CException: Failed to write byte: bus "
                                    "/dev/i2c-1, addr 0x70");
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

TEST(I2CWriteBitActionTests, GetRegister)
{
    I2CWriteBitAction action{0x7C, 5, 1};
    EXPECT_EQ(action.getRegister(), 0x7C);
}

TEST(I2CWriteBitActionTests, GetPosition)
{
    I2CWriteBitAction action{0x7C, 5, 1};
    EXPECT_EQ(action.getPosition(), 5);
}

TEST(I2CWriteBitActionTests, GetValue)
{
    I2CWriteBitAction action{0x7C, 5, 1};
    EXPECT_EQ(action.getValue(), 1);
}

TEST(I2CWriteBitActionTests, ToString)
{
    I2CWriteBitAction action{0x7C, 5, 1};
    EXPECT_EQ(action.toString(),
              "i2c_write_bit: { register: 0x7C, position: 5, value: 1 }");
}
