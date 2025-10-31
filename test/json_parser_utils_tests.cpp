/**
 * Copyright © 2025 IBM Corporation
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
#include "json_parser_utils.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::json_parser_utils;
using json = nlohmann::json;

TEST(JSONParserUtilsTests, GetRequiredProperty)
{
    // Test where property exists
    {
        const json element = R"( { "format": "linear" } )"_json;
        const json& propertyElement = getRequiredProperty(element, "format");
        EXPECT_EQ(propertyElement.get<std::string>(), "linear");
    }

    // Test where property does not exist
    try
    {
        const json element = R"( { "volts": 1.03 } )"_json;
        getRequiredProperty(element, "format");
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Required property missing: format");
    }
}

TEST(JSONParserUtilsTests, ParseBitPosition)
{
    // Test where works: 0
    {
        const json element = R"( 0 )"_json;
        uint8_t value = parseBitPosition(element);
        EXPECT_EQ(value, 0);
    }

    // Test where works: 7
    {
        const json element = R"( 7 )"_json;
        uint8_t value = parseBitPosition(element);
        EXPECT_EQ(value, 7);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.03 )"_json;
        parseBitPosition(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Value < 0
    try
    {
        const json element = R"( -1 )"_json;
        parseBitPosition(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit position");
    }

    // Test where fails: Value > 7
    try
    {
        const json element = R"( 8 )"_json;
        parseBitPosition(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit position");
    }
}

TEST(JSONParserUtilsTests, ParseBitValue)
{
    // Test where works: 0
    {
        const json element = R"( 0 )"_json;
        uint8_t value = parseBitValue(element);
        EXPECT_EQ(value, 0);
    }

    // Test where works: 1
    {
        const json element = R"( 1 )"_json;
        uint8_t value = parseBitValue(element);
        EXPECT_EQ(value, 1);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 0.5 )"_json;
        parseBitValue(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Value < 0
    try
    {
        const json element = R"( -1 )"_json;
        parseBitValue(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit value");
    }

    // Test where fails: Value > 1
    try
    {
        const json element = R"( 2 )"_json;
        parseBitValue(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a bit value");
    }
}

TEST(JSONParserUtilsTests, ParseBoolean)
{
    // Test where works: true
    {
        const json element = R"( true )"_json;
        bool value = parseBoolean(element);
        EXPECT_EQ(value, true);
    }

    // Test where works: false
    {
        const json element = R"( false )"_json;
        bool value = parseBoolean(element);
        EXPECT_EQ(value, false);
    }

    // Test where fails: Element is not a boolean
    try
    {
        const json element = R"( 1 )"_json;
        parseBoolean(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }
}

TEST(JSONParserUtilsTests, ParseDouble)
{
    // Test where works: floating point value
    {
        const json element = R"( 1.03 )"_json;
        double value = parseDouble(element);
        EXPECT_EQ(value, 1.03);
    }

    // Test where works: integer value
    {
        const json element = R"( 24 )"_json;
        double value = parseDouble(element);
        EXPECT_EQ(value, 24.0);
    }

    // Test where fails: Element is not a number
    try
    {
        const json element = R"( true )"_json;
        parseDouble(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a number");
    }
}

TEST(JSONParserUtilsTests, ParseHexByte)
{
    // Test where works: "0xFF"
    {
        const json element = R"( "0xFF" )"_json;
        uint8_t value = parseHexByte(element);
        EXPECT_EQ(value, 0xFF);
    }

    // Test where works: "0xff"
    {
        const json element = R"( "0xff" )"_json;
        uint8_t value = parseHexByte(element);
        EXPECT_EQ(value, 0xff);
    }

    // Test where works: "0xf"
    {
        const json element = R"( "0xf" )"_json;
        uint8_t value = parseHexByte(element);
        EXPECT_EQ(value, 0xf);
    }

    // Test where fails: "0xfff"
    try
    {
        const json element = R"( "0xfff" )"_json;
        parseHexByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "0xAG"
    try
    {
        const json element = R"( "0xAG" )"_json;
        parseHexByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "ff"
    try
    {
        const json element = R"( "ff" )"_json;
        parseHexByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: ""
    try
    {
        const json element = "";
        parseHexByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "f"
    try
    {
        const json element = R"( "f" )"_json;
        parseHexByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "0x"
    try
    {
        const json element = R"( "0x" )"_json;
        parseHexByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }

    // Test where fails: "0Xff"
    try
    {
        const json element = R"( "0XFF" )"_json;
        parseHexByte(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
    }
}

TEST(JSONParserUtilsTests, ParseHexByteArray)
{
    // Test where works
    {
        const json element = R"( [ "0xCC", "0xFF" ] )"_json;
        std::vector<uint8_t> hexBytes = parseHexByteArray(element);
        std::vector<uint8_t> expected = {0xcc, 0xff};
        EXPECT_EQ(hexBytes, expected);
    }

    // Test where fails: Element is not an array
    try
    {
        const json element = 0;
        parseHexByteArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(JSONParserUtilsTests, ParseInt8)
{
    // Test where works: INT8_MIN
    {
        const json element = R"( -128 )"_json;
        int8_t value = parseInt8(element);
        EXPECT_EQ(value, -128);
    }

    // Test where works: INT8_MAX
    {
        const json element = R"( 127 )"_json;
        int8_t value = parseInt8(element);
        EXPECT_EQ(value, 127);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.03 )"_json;
        parseInt8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Value < INT8_MIN
    try
    {
        const json element = R"( -129 )"_json;
        parseInt8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit signed integer");
    }

    // Test where fails: Value > INT8_MAX
    try
    {
        const json element = R"( 128 )"_json;
        parseInt8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit signed integer");
    }
}

TEST(JSONParserUtilsTests, ParseString)
{
    // Test where works: Empty string
    {
        const json element = "";
        std::string value = parseString(element, true);
        EXPECT_EQ(value, "");
    }

    // Test where works: Non-empty string
    {
        const json element = "vdd_regulator";
        std::string value = parseString(element, false);
        EXPECT_EQ(value, "vdd_regulator");
    }

    // Test where fails: Element is not a string
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        parseString(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a string");
    }

    // Test where fails: Empty string
    try
    {
        const json element = "";
        parseString(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }
}

TEST(JSONParserUtilsTests, ParseUint8)
{
    // Test where works: 0
    {
        const json element = R"( 0 )"_json;
        uint8_t value = parseUint8(element);
        EXPECT_EQ(value, 0);
    }

    // Test where works: UINT8_MAX
    {
        const json element = R"( 255 )"_json;
        uint8_t value = parseUint8(element);
        EXPECT_EQ(value, 255);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.03 )"_json;
        parseUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Value < 0
    try
    {
        const json element = R"( -1 )"_json;
        parseUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit unsigned integer");
    }

    // Test where fails: Value > UINT8_MAX
    try
    {
        const json element = R"( 256 )"_json;
        parseUint8(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit unsigned integer");
    }
}

TEST(JSONParserUtilsTests, ParseUnsignedInteger)
{
    // Test where works: 1
    {
        const json element = R"( 1 )"_json;
        unsigned int value = parseUnsignedInteger(element);
        EXPECT_EQ(value, 1);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.5 )"_json;
        parseUnsignedInteger(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an unsigned integer");
    }

    // Test where fails: Value < 0
    try
    {
        const json element = R"( -1 )"_json;
        parseUnsignedInteger(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an unsigned integer");
    }
}

TEST(JSONParserUtilsTests, VerifyIsArray)
{
    // Test where element is an array
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        verifyIsArray(element);
    }

    // Test where element is not an array
    try
    {
        const json element = R"( { "foo": "bar" } )"_json;
        verifyIsArray(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an array");
    }
}

TEST(JSONParserUtilsTests, VerifyIsObject)
{
    // Test where element is an object
    {
        const json element = R"( { "foo": "bar" } )"_json;
        verifyIsObject(element);
    }

    // Test where element is not an object
    try
    {
        const json element = R"( [ "foo", "bar" ] )"_json;
        verifyIsObject(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an object");
    }
}

TEST(JSONParserUtilsTests, VerifyPropertyCount)
{
    // Test where element has expected number of properties
    {
        const json element = R"(
            {
              "comments": [ "Set voltage rule" ],
              "id": "set_voltage_rule"
            }
        )"_json;
        verifyPropertyCount(element, 2);
    }

    // Test where element has unexpected number of properties
    try
    {
        const json element = R"(
            {
              "comments": [ "Set voltage rule" ],
              "id": "set_voltage_rule",
              "foo": 1.3
            }
        )"_json;
        verifyPropertyCount(element, 2);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an invalid property");
    }
}
