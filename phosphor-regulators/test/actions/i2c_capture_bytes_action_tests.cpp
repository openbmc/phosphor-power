/**
 * Copyright © 2021 IBM Corporation
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
#include "i2c_capture_bytes_action.hpp"
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

using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArrayArgument;
using ::testing::Throw;
using ::testing::TypedEq;

TEST(I2CCaptureBytesActionTests, Constructor)
{
    // Test where works
    try
    {
        I2CCaptureBytesAction action{0x2A, 2};
        EXPECT_EQ(action.getRegister(), 0x2A);
        EXPECT_EQ(action.getCount(), 2);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Count < 1
    try
    {
        I2CCaptureBytesAction action{0x2A, 0};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid byte count: Less than 1");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(I2CCaptureBytesActionTests, Execute)
{
    // Test where works: One byte captured
    try
    {
        // Create mock I2CInterface: read() returns value 0xD7
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t values[] = {0xD7};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, TypedEq<uint8_t&>(1), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(values, values + 1));

        // Create Device, IDMap, MockServices, and ActionEnvironment
        Device device{
            "vdd1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/vdd1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "vdd1", services};

        I2CCaptureBytesAction action{0xA0, 1};
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(env.getAdditionalErrorData().size(), 1);
        EXPECT_EQ(env.getAdditionalErrorData().at("vdd1_register_0xA0"),
                  "[ 0xD7 ]");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Multiple bytes captured
    try
    {
        // Create mock I2CInterface: read() returns values 0x56, 0x14, 0xDA
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t values[] = {0x56, 0x14, 0xDA};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0x7C, TypedEq<uint8_t&>(3), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(values, values + 3));

        // Create Device, IDMap, MockServices, and ActionEnvironment
        Device device{
            "vdd1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/vdd1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "vdd1", services};

        I2CCaptureBytesAction action{0x7C, 3};
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(env.getAdditionalErrorData().size(), 1);
        EXPECT_EQ(env.getAdditionalErrorData().at("vdd1_register_0x7C"),
                  "[ 0x56, 0x14, 0xDA ]");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Same device + register captured multiple times
    try
    {
        // Create mock I2CInterface: read() will be called 3 times and will
        // return values 0xD7, 0x13, and 0xFB
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t values_1[] = {0xD7};
        uint8_t values_2[] = {0x13};
        uint8_t values_3[] = {0xFB};
        EXPECT_CALL(*i2cInterface, isOpen)
            .Times(3)
            .WillRepeatedly(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xCA, TypedEq<uint8_t&>(1), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(3)
            .WillOnce(SetArrayArgument<2>(values_1, values_1 + 1))
            .WillOnce(SetArrayArgument<2>(values_2, values_2 + 1))
            .WillOnce(SetArrayArgument<2>(values_3, values_3 + 1));

        // Create Device, IDMap, MockServices, and ActionEnvironment
        Device device{
            "vdd1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/vdd1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "vdd1", services};

        I2CCaptureBytesAction action{0xCA, 1};
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(env.getAdditionalErrorData().size(), 3);
        EXPECT_EQ(env.getAdditionalErrorData().at("vdd1_register_0xCA"),
                  "[ 0xD7 ]");
        EXPECT_EQ(env.getAdditionalErrorData().at("vdd1_register_0xCA_2"),
                  "[ 0x13 ]");
        EXPECT_EQ(env.getAdditionalErrorData().at("vdd1_register_0xCA_3"),
                  "[ 0xFB ]");
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
        ActionEnvironment env{idMap, "vdd1", services};

        I2CCaptureBytesAction action{0x7C, 2};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Unable to find device with ID \"vdd1\"");
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

        // Create Device, IDMap, MockServices, and ActionEnvironment
        Device device{
            "vdd1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/vdd1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "vdd1", services};

        I2CCaptureBytesAction action{0x7C, 2};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(
            e.what(),
            "ActionError: i2c_capture_bytes: { register: 0x7C, count: 2 }");
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

TEST(I2CCaptureBytesActionTests, GetCount)
{
    I2CCaptureBytesAction action{0xA0, 3};
    EXPECT_EQ(action.getCount(), 3);
}

TEST(I2CCaptureBytesActionTests, GetRegister)
{
    I2CCaptureBytesAction action{0xA0, 3};
    EXPECT_EQ(action.getRegister(), 0xA0);
}

TEST(I2CCaptureBytesActionTests, ToString)
{
    I2CCaptureBytesAction action{0xA0, 3};
    EXPECT_EQ(action.toString(),
              "i2c_capture_bytes: { register: 0xA0, count: 3 }");
}
