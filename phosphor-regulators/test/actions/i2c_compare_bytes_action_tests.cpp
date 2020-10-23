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
#include "i2c_compare_bytes_action.hpp"
#include "i2c_interface.hpp"
#include "id_map.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArrayArgument;
using ::testing::Throw;
using ::testing::TypedEq;

// Test for I2CCompareBytesAction(uint8_t reg,
//                                const std::vector<uint8_t>& values)
TEST(I2CCompareBytesActionTests, Constructor1)
{
    // Test where works
    try
    {
        std::vector<uint8_t> values{0x56, 0x14, 0xDA};
        I2CCompareBytesAction action{0x7C, values};

        EXPECT_EQ(action.getRegister(), 0x7C);

        EXPECT_EQ(action.getValues().size(), 3);
        EXPECT_EQ(action.getValues()[0], 0x56);
        EXPECT_EQ(action.getValues()[1], 0x14);
        EXPECT_EQ(action.getValues()[2], 0xDA);

        EXPECT_EQ(action.getMasks().size(), 3);
        EXPECT_EQ(action.getMasks()[0], 0xFF);
        EXPECT_EQ(action.getMasks()[1], 0xFF);
        EXPECT_EQ(action.getMasks()[2], 0xFF);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Values vector is empty
    try
    {
        std::vector<uint8_t> values{};
        I2CCompareBytesAction action{0x7C, values};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Values vector is empty");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

// Test for I2CCompareBytesAction(uint8_t reg,
//                                const std::vector<uint8_t>& values,
//                                const std::vector<uint8_t>& masks)
TEST(I2CCompareBytesActionTests, Constructor2)
{
    // Test where works
    try
    {
        std::vector<uint8_t> values{0x56, 0x14};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        I2CCompareBytesAction action{0xA0, values, masks};

        EXPECT_EQ(action.getRegister(), 0xA0);

        EXPECT_EQ(action.getValues().size(), 2);
        EXPECT_EQ(action.getValues()[0], 0x56);
        EXPECT_EQ(action.getValues()[1], 0x14);

        EXPECT_EQ(action.getMasks().size(), 2);
        EXPECT_EQ(action.getMasks()[0], 0x7E);
        EXPECT_EQ(action.getMasks()[1], 0x3C);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Values vector is empty
    try
    {
        std::vector<uint8_t> values{};
        std::vector<uint8_t> masks{};
        I2CCompareBytesAction action{0xA0, values, masks};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Values vector is empty");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Masks vector different size than values vector
    try
    {
        std::vector<uint8_t> values{0x56, 0x14, 0xFE};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        I2CCompareBytesAction action{0x7C, values, masks};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Masks vector has invalid size");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(I2CCompareBytesActionTests, Execute)
{
    // Test where works: Equal: Mask specified
    try
    {
        // Create mock I2CInterface: read() returns values 0xD7, 0x96
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t actualValues[] = {0xD7, 0x96};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, TypedEq<uint8_t&>(2), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(actualValues, actualValues + 2));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Actual values: 0xD7 = 1101 0111   0x96 = 1001 0110
        // Masks        : 0x7E = 0111 1110   0x3C = 0011 1100
        // Results      : 0x56 = 0101 0110   0x14 = 0001 0100
        const std::vector<uint8_t> values{0x56, 0x14};
        const std::vector<uint8_t> masks{0x7E, 0x3C};
        I2CCompareBytesAction action{0xA0, values, masks};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Equal: Mask not specified
    try
    {
        // Create mock I2CInterface: read() returns values 0x56, 0x14, 0xDA
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t actualValues[] = {0x56, 0x14, 0xDA};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0x7C, TypedEq<uint8_t&>(3), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(actualValues, actualValues + 3));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        std::vector<uint8_t> values{0x56, 0x14, 0xDA};
        I2CCompareBytesAction action{0x7C, values};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Not equal: Mask specified
    try
    {
        // Create mock I2CInterface: read() returns values 0xD7, 0x96
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t actualValues[] = {0xD7, 0x96};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, TypedEq<uint8_t&>(2), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(actualValues, actualValues + 2));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Actual values: 0xD7 = 1101 0111   0x96 = 1001 0110
        // Masks        : 0x7E = 0111 1110   0x3C = 0011 1100
        // Results      : 0x56 = 0101 0110   0x14 = 0001 0100
        const std::vector<uint8_t> values{0x56, 0x13};
        const std::vector<uint8_t> masks{0x7E, 0x3C};
        I2CCompareBytesAction action{0xA0, values, masks};
        EXPECT_EQ(action.execute(env), false);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Not equal: Mask not specified
    try
    {
        // Create mock I2CInterface: read() returns values 0x56, 0x14, 0xDA
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t actualValues[] = {0x56, 0x14, 0xDA};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0x7C, TypedEq<uint8_t&>(3), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(actualValues, actualValues + 3));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        std::vector<uint8_t> values{0x56, 0x14, 0xDB};
        I2CCompareBytesAction action{0x7C, values};
        EXPECT_EQ(action.execute(env), false);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Single byte
    try
    {
        // Create mock I2CInterface: read() returns value 0xD7
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t actualValues[] = {0xD7};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, TypedEq<uint8_t&>(1), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(actualValues, actualValues + 1));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Actual values: 0xD7 = 1101 0111
        // Masks        : 0x7E = 0111 1110
        // Results      : 0x56 = 0101 0110
        std::vector<uint8_t> values{0x56};
        std::vector<uint8_t> masks{0x7E};
        I2CCompareBytesAction action{0xA0, values, masks};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Getting I2CInterface fails
    try
    {
        // Create IDMap, mock services, and ActionEnvironment
        IDMap idMap{};
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        std::vector<uint8_t> values{0x56, 0x14, 0xDB};
        I2CCompareBytesAction action{0x7C, values};
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

    // Test where fails: Reading bytes fails
    try
    {
        // Create mock I2CInterface: read() throws an I2CException
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0x7C, TypedEq<uint8_t&>(2), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(Throw(i2c::I2CException{"Failed to read i2c block data",
                                              "/dev/i2c-1", 0x70}));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        std::vector<uint8_t> values{0x56, 0x14};
        I2CCompareBytesAction action{0x7C, values};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: i2c_compare_bytes: { register: "
                     "0x7C, values: [ 0x56, 0x14 ], masks: [ 0xFF, 0xFF ] }");
        try
        {
            // Re-throw inner I2CException
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const i2c::I2CException& ie)
        {
            EXPECT_STREQ(ie.what(),
                         "I2CException: Failed to read i2c block data: bus "
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

TEST(I2CCompareBytesActionTests, GetRegister)
{
    std::vector<uint8_t> values{0x56, 0x14};
    I2CCompareBytesAction action{0xA0, values};
    EXPECT_EQ(action.getRegister(), 0xA0);
}

TEST(I2CCompareBytesActionTests, GetValues)
{
    std::vector<uint8_t> values{0x56, 0x14};
    std::vector<uint8_t> masks{0x7E, 0x3C};
    I2CCompareBytesAction action{0xA0, values, masks};
    EXPECT_EQ(action.getValues().size(), 2);
    EXPECT_EQ(action.getValues()[0], 0x56);
    EXPECT_EQ(action.getValues()[1], 0x14);
}

TEST(I2CCompareBytesActionTests, GetMasks)
{
    // Test where masks were not specified
    {
        std::vector<uint8_t> values{0x56, 0x14, 0xDA};
        I2CCompareBytesAction action{0x7C, values};
        EXPECT_EQ(action.getMasks().size(), 3);
        EXPECT_EQ(action.getMasks()[0], 0xFF);
        EXPECT_EQ(action.getMasks()[1], 0xFF);
        EXPECT_EQ(action.getMasks()[2], 0xFF);
    }

    // Test where masks were specified
    {
        std::vector<uint8_t> values{0x56, 0x14};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        I2CCompareBytesAction action{0xA0, values, masks};
        EXPECT_EQ(action.getMasks().size(), 2);
        EXPECT_EQ(action.getMasks()[0], 0x7E);
        EXPECT_EQ(action.getMasks()[1], 0x3C);
    }
}

TEST(I2CCompareBytesActionTests, ToString)
{
    std::vector<uint8_t> values{0x56, 0x14};
    std::vector<uint8_t> masks{0x7E, 0x3C};
    I2CCompareBytesAction action{0xA0, values, masks};
    EXPECT_EQ(action.toString(), "i2c_compare_bytes: { register: 0xA0, values: "
                                 "[ 0x56, 0x14 ], masks: [ 0x7E, 0x3C ] }");
}
