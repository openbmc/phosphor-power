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
#include "i2c_compare_bit_action.hpp"
#include "i2c_interface.hpp"
#include "id_map.hpp"
#include "mock_services.hpp"
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

TEST(I2CCompareBitActionTests, Constructor)
{
    // Test where works
    try
    {
        I2CCompareBitAction action{0x7C, 2, 0};
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
        I2CCompareBitAction action{0x7C, 8, 0};
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
        I2CCompareBitAction action{0x7C, 2, 2};
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

TEST(I2CCompareBitActionTests, Execute)
{
    // Test where works
    try
    {
        // Create mock I2CInterface: read() returns value 0x96 (1001 0110)
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(*i2cInterface, read(0x7C, A<uint8_t&>()))
            .WillRepeatedly(SetArgReferee<1>(0x96));

        // Create Device, IDMap, MockServices, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Test where actual bit value is equal to expected bit value.
        // Test all bits in register value 0x96 == 1001 0110).
        {
            I2CCompareBitAction actions[] = {
                I2CCompareBitAction{0x7C, 7, 1},
                I2CCompareBitAction{0x7C, 6, 0},
                I2CCompareBitAction{0x7C, 5, 0},
                I2CCompareBitAction{0x7C, 4, 1},
                I2CCompareBitAction{0x7C, 3, 0},
                I2CCompareBitAction{0x7C, 2, 1},
                I2CCompareBitAction{0x7C, 1, 1},
                I2CCompareBitAction{0x7C, 0, 0}};
            for (I2CCompareBitAction& action : actions)
            {
                EXPECT_EQ(action.execute(env), true);
            }
        }

        // Test where actual bit value is not equal to expected bit value.
        // Test all bits in register value 0x96 == 1001 0110).
        {
            I2CCompareBitAction actions[] = {
                I2CCompareBitAction{0x7C, 7, 0},
                I2CCompareBitAction{0x7C, 6, 1},
                I2CCompareBitAction{0x7C, 5, 1},
                I2CCompareBitAction{0x7C, 4, 0},
                I2CCompareBitAction{0x7C, 3, 1},
                I2CCompareBitAction{0x7C, 2, 0},
                I2CCompareBitAction{0x7C, 1, 0},
                I2CCompareBitAction{0x7C, 0, 1}};
            for (I2CCompareBitAction& action : actions)
            {
                EXPECT_EQ(action.execute(env), false);
            }
        }
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Getting I2CInterface fails
    try
    {
        // Create IDMap, MockServices, and ActionEnvironment
        IDMap idMap{};
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        I2CCompareBitAction action{0x7C, 5, 1};
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

        // Create Device, IDMap, MockServices, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        I2CCompareBitAction action{0x7C, 5, 1};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(), "ActionError: i2c_compare_bit: { register: "
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
}

TEST(I2CCompareBitActionTests, GetRegister)
{
    I2CCompareBitAction action{0x7C, 5, 1};
    EXPECT_EQ(action.getRegister(), 0x7C);
}

TEST(I2CCompareBitActionTests, GetPosition)
{
    I2CCompareBitAction action{0x7C, 5, 1};
    EXPECT_EQ(action.getPosition(), 5);
}

TEST(I2CCompareBitActionTests, GetValue)
{
    I2CCompareBitAction action{0x7C, 5, 1};
    EXPECT_EQ(action.getValue(), 1);
}

TEST(I2CCompareBitActionTests, ToString)
{
    I2CCompareBitAction action{0x7C, 5, 1};
    EXPECT_EQ(action.toString(),
              "i2c_compare_bit: { register: 0x7C, position: 5, value: 1 }");
}
