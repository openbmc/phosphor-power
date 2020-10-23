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
#include "i2c_write_bytes_action.hpp"
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

using ::testing::A;
using ::testing::Args;
using ::testing::ElementsAre;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArrayArgument;
using ::testing::Throw;
using ::testing::TypedEq;

// Test for I2CWriteBytesAction(uint8_t reg,
//                              const std::vector<uint8_t>& values)
TEST(I2CWriteBytesActionTests, Constructor1)
{
    // Test where works
    try
    {
        std::vector<uint8_t> values{0x56, 0x14, 0xDA};
        I2CWriteBytesAction action{0x7C, values};

        EXPECT_EQ(action.getRegister(), 0x7C);

        EXPECT_EQ(action.getValues().size(), 3);
        EXPECT_EQ(action.getValues()[0], 0x56);
        EXPECT_EQ(action.getValues()[1], 0x14);
        EXPECT_EQ(action.getValues()[2], 0xDA);

        EXPECT_EQ(action.getMasks().size(), 0);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Values vector is empty
    try
    {
        std::vector<uint8_t> values{};
        I2CWriteBytesAction action{0x7C, values};
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

// Test for I2CWriteBytesAction(uint8_t reg,
//                              const std::vector<uint8_t>& values,
//                              const std::vector<uint8_t>& masks)
TEST(I2CWriteBytesActionTests, Constructor2)
{
    // Test where works
    try
    {
        std::vector<uint8_t> values{0x56, 0x14};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        I2CWriteBytesAction action{0xA0, values, masks};

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
        I2CWriteBytesAction action{0xA0, values, masks};
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
        I2CWriteBytesAction action{0x7C, values, masks};
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

TEST(I2CWriteBytesActionTests, Execute)
{
    // Test where works: Masks not specified
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface,
                    read(A<uint8_t>(), A<uint8_t&>(), A<uint8_t*>(),
                         A<i2c::I2CInterface::Mode>()))
            .Times(0);
        EXPECT_CALL(*i2cInterface,
                    write(0x7C, 3, NotNull(), i2c::I2CInterface::Mode::I2C))
            .With(Args<2, 1>(ElementsAre(0x56, 0x14, 0xDA)))
            .Times(1);

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
        I2CWriteBytesAction action{0x7C, values};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Masks specified
    try
    {
        // Create mock I2CInterface: read() returns values 0x69, 0xA5
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t actualValues[] = {0x69, 0xA5};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, TypedEq<uint8_t&>(2), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(actualValues, actualValues + 2));
        EXPECT_CALL(*i2cInterface,
                    write(0xA0, 2, NotNull(), i2c::I2CInterface::Mode::I2C))
            .With(Args<2, 1>(ElementsAre(0xEA, 0xB3)))
            .Times(1);

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        //                        Byte 1             Byte 2
        // Value to write       : 0xD6 = 1101 0110 : 0xD2 = 1101 0010
        // Mask                 : 0xC3 = 1100 0011 : 0x96 = 1001 0110
        // Current value        : 0x69 = 0110 1001 : 0xA5 = 1010 0101
        // Value to write & mask: 0xC2 = 1100 0010 : 0x92 = 1001 0010
        // ~Mask                : 0x3C = 0011 1100 : 0x69 = 0110 1001
        // Current value & ~mask: 0x28 = 0010 1000 : 0x21 = 0010 0001
        // Final value to write : 0xEA = 1110 1010 : 0xB3 = 1011 0011
        std::vector<uint8_t> values{0xD6, 0xD2};
        std::vector<uint8_t> masks{0xC3, 0x96};
        I2CWriteBytesAction action{0xA0, values, masks};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Single byte
    try
    {
        // Create mock I2CInterface: read() returns value 0x69
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        uint8_t actualValues[] = {0x69};
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(0xA0, TypedEq<uint8_t&>(1), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(SetArrayArgument<2>(actualValues, actualValues + 1));
        EXPECT_CALL(*i2cInterface,
                    write(0xA0, 1, NotNull(), i2c::I2CInterface::Mode::I2C))
            .With(Args<2, 1>(ElementsAre(0xEA)))
            .Times(1);

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Value to write       : 0xD6 = 1101 0110
        // Mask                 : 0xC3 = 1100 0011
        // Current value        : 0x69 = 0110 1001
        // Value to write & mask: 0xC2 = 1100 0010
        // ~Mask                : 0x3C = 0011 1100
        // Current value & ~mask: 0x28 = 0010 1000
        // Final value to write : 0xEA = 1110 1010
        std::vector<uint8_t> values{0xD6};
        std::vector<uint8_t> masks{0xC3};
        I2CWriteBytesAction action{0xA0, values, masks};
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
        I2CWriteBytesAction action{0x7C, values};
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
        EXPECT_CALL(*i2cInterface, read(0xA0, TypedEq<uint8_t&>(2), NotNull(),
                                        i2c::I2CInterface::Mode::I2C))
            .Times(1)
            .WillOnce(Throw(i2c::I2CException{"Failed to read i2c block data",
                                              "/dev/i2c-1", 0x70}));
        EXPECT_CALL(*i2cInterface,
                    write(A<uint8_t>(), A<uint8_t>(), A<const uint8_t*>(),
                          A<i2c::I2CInterface::Mode>()))
            .Times(0);

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        std::vector<uint8_t> values{0xD6, 0xD2};
        std::vector<uint8_t> masks{0xC3, 0x96};
        I2CWriteBytesAction action{0xA0, values, masks};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: i2c_write_bytes: { register: "
                     "0xA0, values: [ 0xD6, 0xD2 ], masks: [ 0xC3, 0x96 ] }");
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

    // Test where fails: Writing bytes fails
    try
    {
        // Create mock I2CInterface: write() throws an I2CException
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface,
                    read(A<uint8_t>(), A<uint8_t&>(), A<uint8_t*>(),
                         A<i2c::I2CInterface::Mode>()))
            .Times(0);
        EXPECT_CALL(*i2cInterface,
                    write(0x7C, 3, NotNull(), i2c::I2CInterface::Mode::I2C))
            .With(Args<2, 1>(ElementsAre(0x56, 0x14, 0xDA)))
            .Times(1)
            .WillOnce(Throw(i2c::I2CException{"Failed to write i2c block data",
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

        std::vector<uint8_t> values{0x56, 0x14, 0xDA};
        I2CWriteBytesAction action{0x7C, values};
        EXPECT_EQ(action.execute(env), true);
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: i2c_write_bytes: { register: "
                     "0x7C, values: [ 0x56, 0x14, 0xDA ], masks: [  ] }");
        try
        {
            // Re-throw inner I2CException
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const i2c::I2CException& ie)
        {
            EXPECT_STREQ(ie.what(),
                         "I2CException: Failed to write i2c block data: bus "
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

TEST(I2CWriteBytesActionTests, GetRegister)
{
    std::vector<uint8_t> values{0x56, 0x14};
    I2CWriteBytesAction action{0xA0, values};
    EXPECT_EQ(action.getRegister(), 0xA0);
}

TEST(I2CWriteBytesActionTests, GetValues)
{
    std::vector<uint8_t> values{0x56, 0x14};
    std::vector<uint8_t> masks{0x7E, 0x3C};
    I2CWriteBytesAction action{0xA0, values, masks};
    EXPECT_EQ(action.getValues().size(), 2);
    EXPECT_EQ(action.getValues()[0], 0x56);
    EXPECT_EQ(action.getValues()[1], 0x14);
}

TEST(I2CWriteBytesActionTests, GetMasks)
{
    // Test where masks were not specified
    {
        std::vector<uint8_t> values{0x56, 0x14, 0xDA};
        I2CWriteBytesAction action{0x7C, values};
        EXPECT_EQ(action.getMasks().size(), 0);
    }

    // Test where masks were specified
    {
        std::vector<uint8_t> values{0x56, 0x14};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        I2CWriteBytesAction action{0xA0, values, masks};
        EXPECT_EQ(action.getMasks().size(), 2);
        EXPECT_EQ(action.getMasks()[0], 0x7E);
        EXPECT_EQ(action.getMasks()[1], 0x3C);
    }
}

TEST(I2CWriteBytesActionTests, ToString)
{
    // Test where masks were not specified
    {
        std::vector<uint8_t> values{0x56, 0x14, 0xDA};
        I2CWriteBytesAction action{0x7C, values};
        EXPECT_EQ(action.toString(),
                  "i2c_write_bytes: { register: 0x7C, values: "
                  "[ 0x56, 0x14, 0xDA ], masks: [  ] }");
    }

    // Test where masks were specified
    {
        std::vector<uint8_t> values{0x56, 0x14};
        std::vector<uint8_t> masks{0x7E, 0x3C};
        I2CWriteBytesAction action{0xA0, values, masks};
        EXPECT_EQ(action.toString(),
                  "i2c_write_bytes: { register: 0xA0, values: "
                  "[ 0x56, 0x14 ], masks: [ 0x7E, 0x3C ] }");
    }
}
