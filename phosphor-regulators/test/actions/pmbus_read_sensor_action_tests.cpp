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
#include "i2c_interface.hpp"
#include "id_map.hpp"
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
    // Test where works: exponent value are specified
    try
    {
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        std::string command = "0x8C";
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{-8};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.getType(), pmbus_utils::SensorValueType::iout);
        EXPECT_EQ(action.getCommand(), "0x8C");
        EXPECT_EQ(action.getFormat(), pmbus_utils::SensorDataFormat::linear_11);
        EXPECT_EQ(action.getExponent().has_value(), true);
        EXPECT_EQ(action.getExponent().value(), -8);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works:exponent value is not specified
    try
    {
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        std::string command = "0x8C";
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.getType(), pmbus_utils::SensorValueType::iout);
        EXPECT_EQ(action.getCommand(), "0x8C");
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
    // TODO: Not implemented yet
}

TEST(PMBusReadSensorActionTests, GetCommand)
{
    pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
    std::string command = "0x8C";
    pmbus_utils::SensorDataFormat format{
        pmbus_utils::SensorDataFormat::linear_11};
    std::optional<int8_t> exponent{-8};
    PMBusReadSensorAction action{type, command, format, exponent};
    EXPECT_EQ(action.getCommand(), "0x8C");
}

TEST(PMBusReadSensorActionTests, GetExponent)
{
    pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
    std::string command = "0x8C";
    pmbus_utils::SensorDataFormat format{
        pmbus_utils::SensorDataFormat::linear_11};

    // Exponent value was specified
    {
        std::optional<int8_t> exponent{-9};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.getExponent().has_value(), true);
        EXPECT_EQ(action.getExponent().value(), -9);
    }

    // Exponent value was not specified
    {
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.getExponent().has_value(), false);
    }
}

TEST(PMBusReadSensorActionTests, GetFormat)
{
    pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
    std::string command = "0x8C";
    pmbus_utils::SensorDataFormat format{
        pmbus_utils::SensorDataFormat::linear_11};
    std::optional<int8_t> exponent{-8};
    PMBusReadSensorAction action{type, command, format, exponent};
    EXPECT_EQ(action.getFormat(), pmbus_utils::SensorDataFormat::linear_11);
}

TEST(PMBusReadSensorActionTests, GetType)
{
    pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
    std::string command = "0x8C";
    pmbus_utils::SensorDataFormat format{
        pmbus_utils::SensorDataFormat::linear_11};
    std::optional<int8_t> exponent{-8};
    PMBusReadSensorAction action{type, command, format, exponent};
    EXPECT_EQ(action.getType(), pmbus_utils::SensorValueType::iout);
}

TEST(PMBusReadSensorActionTests, ToString)
{
    // Test where exponent value are specified
    {
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        std::string command = "0x8C";
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{-8};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.toString(),
                  "pmbus_read_sensor: { type: iout, command: 0x8C, format: "
                  "linear_11, exponent: -8 }");
    }

    // Test where exponent value are not specified
    {
        pmbus_utils::SensorValueType type{pmbus_utils::SensorValueType::iout};
        std::string command = "0x8C";
        pmbus_utils::SensorDataFormat format{
            pmbus_utils::SensorDataFormat::linear_11};
        std::optional<int8_t> exponent{};
        PMBusReadSensorAction action{type, command, format, exponent};
        EXPECT_EQ(action.toString(), "pmbus_read_sensor: { type: iout, "
                                     "command: 0x8C, format: linear_11 }");
    }
}
