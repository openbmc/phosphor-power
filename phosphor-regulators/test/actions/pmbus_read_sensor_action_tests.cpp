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
#include "i2c_action.hpp"
#include "i2c_interface.hpp"
#include "id_map.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "pmbus_error.hpp"
#include "pmbus_read_sensor_action.hpp"
#include "pmbus_utils.hpp"

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

TEST(PMBusReadSensorActionTests, Constructor)
{
    // Test where works: exponent value is specified
    try
    {
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        uint8_t command = 0x8C;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_16};
        std::optional<int8_t> exponent{-8};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.getType(), pmbus_utils::SensorValueType::iout);
        EXPECT_EQ(action.getCommand(), 0x8C);
        EXPECT_EQ(action.getFormat(), pmbus_utils::SensorDataFormat::linear_16);
        EXPECT_EQ(action.getExponent().has_value(), true);
        EXPECT_EQ(action.getExponent().value(), -8);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: exponent value is not specified
    try
    {
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        uint8_t command = 0x8C;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.getType(), pmbus_utils::SensorValueType::iout);
        EXPECT_EQ(action.getCommand(), 0x8C);
        EXPECT_EQ(action.getFormat(), pmbus_utils::SensorDataFormat::linear_11);
        EXPECT_EQ(action.getExponent().has_value(), false);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(PMBusReadSensorActionTests, Execute)
{
    // Test where works: linear_11 defined in action
    try
    {
        // Create mock I2CInterface.
        // * will read 0xD2E0 from READ_IOUT (command/register 0x8C)
        // * will not read from VOUT_MODE (command/register 0x20)
        // assume output current is 11.5 amps,
        // exponent = -6 = 11010, mantissa = 736 = 010 1110 0000
        // linear data format = 1101 0010 1110 0000 = 0xD2E0
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x8C), A<uint16_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0xD2E0));
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint8_t&>())).Times(0);

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Create and execute action
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        uint8_t command = 0x8C;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(env.getSensorReadings().size(), 1);
        EXPECT_EQ(env.getSensorReadings()[0].type,
                  pmbus_utils::SensorValueType::iout);
        EXPECT_DOUBLE_EQ(env.getSensorReadings()[0].value, 11.5);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: linear_16 with exponent defined in action
    try
    {
        // Create mock I2CInterface.
        // * will read 0x0002 from READ_VOUT (command/register 0x8B)
        // * will not read from VOUT_MODE (command/register 0x20)
        // assume output voltage is 16 volts,
        // exponent = 3
        // linear data format = 0000 0000 0000 0010 = 0x0002 = 2
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x8B), A<uint16_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0x0002));
        EXPECT_CALL(*i2cInterface, read(A<uint8_t>(), A<uint8_t&>())).Times(0);

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Create and execute action
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::vout};
        uint8_t command = 0x8B;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_16};
        std::optional<int8_t> exponent{3};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(env.getSensorReadings().size(), 1);
        EXPECT_EQ(env.getSensorReadings()[0].type,
                  pmbus_utils::SensorValueType::vout);
        EXPECT_DOUBLE_EQ(env.getSensorReadings()[0].value, 16);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: linear_16 with no exponent defined in action
    try
    {
        // Create mock I2CInterface.
        // * will read 0xB877 from vout_peak (command/register 0xC6)
        // * will read 0b0001'0111 (linear format, -9 exponent) from VOUT_MODE
        // assume output voltage is 0.232421875 volts,
        // linear data format = 0000 0000 0111 0111 = 0x0077
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0xC6), A<uint16_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0x0077));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x20), A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0b0001'0111));
        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Create and execute action
        pmbus_utils::SensorValueType type{
            pmbus_utils::SensorValueType::vout_peak};
        uint8_t command = 0xC6;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_16};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.execute(env), true);
        EXPECT_EQ(env.getSensorReadings().size(), 1);
        EXPECT_EQ(env.getSensorReadings()[0].type,
                  pmbus_utils::SensorValueType::vout_peak);
        EXPECT_DOUBLE_EQ(env.getSensorReadings()[0].value, 0.232421875);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: Unable to get I2C interface to current device
    try
    {
        // Create IDMap, mock services, and ActionEnvironment
        IDMap idMap{};
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Create and execute action
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::pout};
        uint8_t command = 0x96;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
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

    // Test where fails: VOUT_MODE data format is not linear
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will read 0b0010'0000 (vid data format) from VOUT_MODE
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x8B), A<uint16_t&>()))
            .Times(1);
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x20), A<uint8_t&>()))
            .Times(1)
            .WillOnce(SetArgReferee<1>(0b0010'0000));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Create and execute action
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::vout};
        uint8_t command = 0x8B;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_16};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(), "ActionError: pmbus_read_sensor: { type: vout, "
                               "command: 0x8B, format: linear_16 }");
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

    // Test where fails: Reading VOUT_MODE fails
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will try to read VOUT_MODE; exception will be thrown
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0xC6), A<uint16_t&>()))
            .Times(1);
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x20), A<uint8_t&>()))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to read byte", "/dev/i2c-1", 0x70}));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Create and execute action
        pmbus_utils::SensorValueType type{
            pmbus_utils::SensorValueType::vout_peak};
        uint8_t command = 0xC6;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_16};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(),
                     "ActionError: pmbus_read_sensor: { type: vout_peak, "
                     "command: 0xC6, format: linear_16 }");
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

    // Test where fails: Reading PMBus command code with sensor value fails
    try
    {
        // Create mock I2CInterface.  Expect action to do the following:
        // * will try to read PMBus command(0x96); exception will be thrown
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, read(TypedEq<uint8_t>(0x96), A<uint16_t&>()))
            .Times(1)
            .WillOnce(Throw(
                i2c::I2CException{"Failed to read word", "/dev/i2c-1", 0x70}));

        // Create Device, IDMap, mock services, and ActionEnvironment
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};

        // Create and execute action
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::pout};
        uint8_t command = 0x96;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        action.execute(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const ActionError& e)
    {
        EXPECT_STREQ(e.what(), "ActionError: pmbus_read_sensor: { type: pout, "
                               "command: 0x96, format: linear_11 }");
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
                "I2CException: Failed to read word: bus /dev/i2c-1, addr 0x70");
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

TEST(PMBusReadSensorActionTests, GetCommand)
{
    pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
    uint8_t command = 0x8C;
    pmbus_utils::SensorDataFormat format{
        pmbus_utils::SensorDataFormat::linear_16};
    std::optional<int8_t> exponent{-8};
    PMBusReadSensorAction action{type, command, format, exponent};
    EXPECT_EQ(action.getCommand(), 0x8C);
}

TEST(PMBusReadSensorActionTests, GetExponent)
{
    pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
    uint8_t command = 0x8C;
    pmbus_utils::SensorDataFormat format{
        pmbus_utils::SensorDataFormat::linear_16};

    // Exponent value is specified
    {
        std::optional<int8_t> exponent{-9};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.getExponent().has_value(), true);
        EXPECT_EQ(action.getExponent().value(), -9);
    }

    // Exponent value is not specified
    {
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.getExponent().has_value(), false);
    }
}

TEST(PMBusReadSensorActionTests, GetFormat)
{
    pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
    uint8_t command = 0x8C;
    pmbus_utils::SensorDataFormat format{
        pmbus_utils::SensorDataFormat::linear_16};
    std::optional<int8_t> exponent{-8};
    PMBusReadSensorAction action{type, command, format, exponent};
    EXPECT_EQ(action.getFormat(), pmbus_utils::SensorDataFormat::linear_16);
}

TEST(PMBusReadSensorActionTests, GetType)
{
    pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::pout};
    uint8_t command = 0x8C;
    pmbus_utils::SensorDataFormat format{
        pmbus_utils::SensorDataFormat::linear_16};
    std::optional<int8_t> exponent{-8};
    PMBusReadSensorAction action{type, command, format, exponent};
    EXPECT_EQ(action.getType(), pmbus_utils::SensorValueType::pout);
}

TEST(PMBusReadSensorActionTests, ToString)
{
    // Test where exponent value is specified
    {
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::vout};
        uint8_t command = 0x8B;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_16};
        std::optional<int8_t> exponent{-8};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.toString(), "pmbus_read_sensor: { type: "
                                     "vout, command: 0x8B, format: "
                                     "linear_16, exponent: -8 }");
    }

    // Test where exponent value is not specified
    {
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        uint8_t command = 0x8C;
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.toString(), "pmbus_read_sensor: { type: iout, "
                                     "command: 0x8C, format: linear_11 }");
    }
}
