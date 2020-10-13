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
#include "id_map.hpp"
#include "mocked_i2c_interface.hpp"
#include "pmbus_error.hpp"
#include "pmbus_utils.hpp"
#include "pmbus_write_vout_command_action.hpp"
#include "write_verification_error.hpp"

#include <cstdint>
#include <memory>
#include <optional>
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

TEST(PMBusWriteVoutCommandActionTests, Constructor)
{
    // Test where works: Volts value and exponent value are specified
    try
    {
        std::optional<double> volts{1.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{-8};
        bool isVerified{true};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.getVolts().has_value(), true);
        EXPECT_EQ(action.getVolts().value(), 1.3);
        EXPECT_EQ(action.getFormat(), pmbus_utils::VoutDataFormat::linear);
        EXPECT_EQ(action.getExponent().has_value(), true);
        EXPECT_EQ(action.getExponent().value(), -8);
        EXPECT_EQ(action.isVerified(), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Volts value and exponent value are not specified
    try
    {
        std::optional<double> volts{};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.getVolts().has_value(), false);
        EXPECT_EQ(action.getFormat(), pmbus_utils::VoutDataFormat::linear);
        EXPECT_EQ(action.getExponent().has_value(), false);
        EXPECT_EQ(action.isVerified(), false);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Data format is not linear
    try
    {
        std::optional<double> volts{};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::direct};
        std::optional<int8_t> exponent{};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Unsupported data format specified");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(PMBusWriteVoutCommandActionTests, Execute)
{
    // Test where works: Volts value and exponent value defined in action;
    // write is verified.
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will not read from VOUT_MODE (command/register 0x20)
        // * will write 0x014D to VOUT_COMMAND (command/register 0x21)
        // * will read 0x014D from VOUT_COMMAND
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint8_t&>())).Times(0);
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x21), TypedEq<uint16_t>(0x014D)))
            .Times(1);
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x21), A<uint16_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0x014D));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Create and execute action
        // Linear format volts value = (1.3 / 2^(-8)) = 332.8 = 333 = 0x014D
        std::optional<double> volts{1.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{-8};
        bool isVerified{true};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: Volts value defined in ActionEnvironment; exponent
    // value defined in VOUT_MODE; write is not verified.
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will read 0b0001'0111 (linear format, -9 exponent) from VOUT_MODE
        // * will write 0x069A to VOUT_COMMAND
        // * will not read from VOUT_COMMAND
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x20), A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0b0001'0111));
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x21), TypedEq<uint16_t>(0x069A)))
            .Times(1);
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint16_t&>())).Times(0);

        // Create Device, IDMap, and ActionEnvironment.  Set volts value to 3.3
        // in ActionEnvironment.
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};
        env.setVolts(3.3);

        // Create and execute action
        // Linear format volts value = (3.3 / 2^(-9)) = 1689.6 = 1690 = 0x069A
        std::optional<double> volts{};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.execute(env), true);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: No volts value defined
    try
    {
        // Create IDMap and ActionEnvironment
        IDMap idMap{};
        ActionEnvironment env{idMap, "reg1"};

        // Create and execute action
        std::optional<double> volts{};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{-8};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(
            e.what(),
            "ActionError: pmbus_write_vout_command: { format: linear, "
            "exponent: -8, is_verified: false }: No volts value defined");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Unable to get I2C interface to current device
    try
    {
        // Create IDMap and ActionEnvironment
        IDMap idMap{};
        ActionEnvironment env{idMap, "reg1"};

        // Create and execute action
        std::optional<double> volts{1.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{-8};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
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

    // Test where fails: Unable to read VOUT_MODE to get exponent
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will try to read VOUT_MODE; exception will be thrown
        // * will not write to VOUT_COMMAND due to exception
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x20), A<uint8_t&>()))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to read byte", "/dev/i2c-1", 0x70}));
        EXPECT_CALL(*i2cInterface, write(A<uint8_t>(), A<uint16_t>())).Times(0);

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Create and execute action
        std::optional<double> volts{3.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: pmbus_write_vout_command: { volts: 3.3, "
                     "format: linear, is_verified: false }");
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

    // Test where fails: VOUT_MODE data format is not linear
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will read 0b0010'0000 (vid data format) from VOUT_MODE
        // * will not write to VOUT_COMMAND due to data format error
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x20), A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0b0010'0000));
        EXPECT_CALL(*i2cInterface, write(A<uint8_t>(), A<uint16_t>())).Times(0);

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Create and execute action
        std::optional<double> volts{3.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: pmbus_write_vout_command: { volts: 3.3, "
                     "format: linear, is_verified: false }");
        try
        {
            // Re-throw inner PMBusError
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const PMBusError& pe)
        {
            EXPECT_STREQ(
                pe.what(),
                "PMBusError: VOUT_MODE contains unsupported data format");
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

    // Test where fails: Unable to write VOUT_COMMAND
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will not read from VOUT_MODE
        // * will try to write 0x014D to VOUT_COMMAND; exception will be thrown
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint8_t&>())).Times(0);
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x21), TypedEq<uint16_t>(0x014D)))
            .Times(1)
            .WillOnce(Throw(i2c::I2CException{"Failed to write word data",
                                              "/dev/i2c-1", 0x70}));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Create and execute action
        // Linear format volts value = (1.3 / 2^(-8)) = 332.8 = 333 = 0x014D
        std::optional<double> volts{1.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{-8};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: pmbus_write_vout_command: { volts: 1.3, "
                     "format: linear, exponent: -8, is_verified: false }");
        try
        {
            // Re-throw inner I2CException
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const i2c::I2CException& ie)
        {
            EXPECT_STREQ(ie.what(), "I2CException: Failed to write word data: "
                                    "bus /dev/i2c-1, addr 0x70");
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

    // Test where fails: Unable to read VOUT_COMMAND
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will not read from VOUT_MODE
        // * will write 0x014D to VOUT_COMMAND
        // * will try to read from VOUT_COMMAND; exception will be thrown
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint8_t&>())).Times(0);
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x21), TypedEq<uint16_t>(0x014D)))
            .Times(1);
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x21), A<uint16_t&>()))
            .Times(1)
            .WillOnce(Throw(i2c::I2CException{"Failed to read word data",
                                              "/dev/i2c-1", 0x70}));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Create and execute action
        // Linear format volts value = (1.3 / 2^(-8)) = 332.8 = 333 = 0x014D
        std::optional<double> volts{1.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{-8};
        bool isVerified{true};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: pmbus_write_vout_command: { volts: 1.3, "
                     "format: linear, exponent: -8, is_verified: true }");
        try
        {
            // Re-throw inner I2CException
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const i2c::I2CException& ie)
        {
            EXPECT_STREQ(ie.what(), "I2CException: Failed to read word data: "
                                    "bus /dev/i2c-1, addr 0x70");
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

    // Test where fails: Write verification error
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will not read from VOUT_MODE
        // * will write 0x014D to VOUT_COMMAND
        // * will read 0x014C from VOUT_COMMAND (not equal to 0x014D)
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint8_t&>())).Times(0);
        EXPECT_CALL(*i2cInterface,
                    write(TypedEq<uint8_t>(0x21), TypedEq<uint16_t>(0x014D)))
            .Times(1);
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x21), A<uint16_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0x014C));

        // Create Device, IDMap, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        ActionEnvironment env{idMap, "reg1"};

        // Create and execute action
        // Linear format volts value = (1.3 / 2^(-8)) = 332.8 = 333 = 0x014D
        std::optional<double> volts{1.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{-8};
        bool isVerified{true};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: pmbus_write_vout_command: { volts: 1.3, "
                     "format: linear, exponent: -8, is_verified: true }");
        try
        {
            // Re-throw inner WriteVerificationError
            std::rethrow_if_nested(e);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const WriteVerificationError& we)
        {
            EXPECT_STREQ(
                we.what(),
                "WriteVerificationError: device: reg1, register: VOUT_COMMAND, "
                "value_written: 0x14D, value_read: 0x14C");
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

TEST(PMBusWriteVoutCommandActionTests, GetExponent)
{
    std::optional<double> volts{1.3};
    pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
    bool isVerified{true};

    // Exponent value was specified
    {
        std::optional<int8_t> exponent{-9};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.getExponent().has_value(), true);
        EXPECT_EQ(action.getExponent().value(), -9);
    }

    // Exponent value was not specified
    {
        std::optional<int8_t> exponent{};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.getExponent().has_value(), false);
    }
}

TEST(PMBusWriteVoutCommandActionTests, GetFormat)
{
    std::optional<double> volts{};
    pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
    std::optional<int8_t> exponent{};
    bool isVerified{false};
    PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
    EXPECT_EQ(action.getFormat(), pmbus_utils::VoutDataFormat::linear);
}

TEST(PMBusWriteVoutCommandActionTests, GetVolts)
{
    pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
    std::optional<int8_t> exponent{-8};
    bool isVerified{true};

    // Volts value was specified
    {
        std::optional<double> volts{1.3};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.getVolts().has_value(), true);
        EXPECT_EQ(action.getVolts().value(), 1.3);
    }

    // Volts value was not specified
    {
        std::optional<double> volts{};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.getVolts().has_value(), false);
    }
}

TEST(PMBusWriteVoutCommandActionTests, IsVerified)
{
    std::optional<double> volts{1.3};
    pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
    std::optional<int8_t> exponent{-8};
    bool isVerified{true};
    PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
    EXPECT_EQ(action.isVerified(), true);
}

TEST(PMBusWriteVoutCommandActionTests, ToString)
{
    // Test where volts value and exponent value are specified
    {
        std::optional<double> volts{1.3};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{-8};
        bool isVerified{true};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(action.toString(),
                  "pmbus_write_vout_command: { volts: 1.3, format: linear, "
                  "exponent: -8, is_verified: true }");
    }

    // Test where volts value and exponent value are not specified
    {
        std::optional<double> volts{};
        pmbus_utils::VoutDataFormat format{pmbus_utils::VoutDataFormat::linear};
        std::optional<int8_t> exponent{};
        bool isVerified{false};
        PMBusWriteVoutCommandAction action{volts, format, exponent, isVerified};
        EXPECT_EQ(
            action.toString(),
            "pmbus_write_vout_command: { format: linear, is_verified: false }");
    }
}
