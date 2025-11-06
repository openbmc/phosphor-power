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
using namespace phosphor::power::json_parser_utils::internal;
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

    // Test where works: Variable specified
    {
        std::map<std::string, std::string> variables{{"bit_pos", "3"}};
        const json element = R"( "${bit_pos}" )"_json;
        uint8_t value = parseBitPosition(element, variables);
        EXPECT_EQ(value, 3);
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

    // Test where fails: Variable specified: Value < 0
    try
    {
        std::map<std::string, std::string> variables{{"bit_pos", "-1"}};
        const json element = R"( "${bit_pos}" )"_json;
        parseBitPosition(element, variables);
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

    // Test where works: Variable specified
    {
        std::map<std::string, std::string> variables{{"bit_val", "1"}};
        const json element = R"( "${bit_val}" )"_json;
        uint8_t value = parseBitValue(element, variables);
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

    // Test where fails: Variable specified: Not an integer
    try
    {
        std::map<std::string, std::string> variables{{"bit_val", "one"}};
        const json element = R"( "${bit_val}" )"_json;
        parseBitValue(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
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

    // Test where works: Variable specified: true
    {
        std::map<std::string, std::string> variables{{"bool_val", "true"}};
        const json element = R"( "${bool_val}" )"_json;
        bool value = parseBoolean(element, variables);
        EXPECT_EQ(value, true);
    }

    // Test where works: Variable specified: false
    {
        std::map<std::string, std::string> variables{{"bool_val", "false"}};
        const json element = R"( "${bool_val}" )"_json;
        bool value = parseBoolean(element, variables);
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

    // Test where fails: Variable specified: Variables map not specified
    try
    {
        const json element = R"( "${bool_val}" )"_json;
        parseBoolean(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }

    // Test where fails: Variable specified: Value is not a boolean
    try
    {
        std::map<std::string, std::string> variables{{"bool_val", "3.2"}};
        const json element = R"( "${bool_val}" )"_json;
        parseBoolean(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a boolean");
    }
}

TEST(JSONParserUtilsTests, ParseDouble)
{
    // Test where works: Floating point value
    {
        const json element = R"( 1.03 )"_json;
        double value = parseDouble(element);
        EXPECT_EQ(value, 1.03);
    }

    // Test where works: Integer value
    {
        const json element = R"( -24 )"_json;
        double value = parseDouble(element);
        EXPECT_EQ(value, -24.0);
    }

    // Test where works: Variable specified: Floating point value
    {
        std::map<std::string, std::string> variables{{"var", "-1.03"}};
        const json element = R"( "${var}" )"_json;
        double value = parseDouble(element, variables);
        EXPECT_EQ(value, -1.03);
    }

    // Test where works: Variable specified: Integer value
    {
        std::map<std::string, std::string> variables{{"var", "24"}};
        const json element = R"( "${var}" )"_json;
        double value = parseDouble(element, variables);
        EXPECT_EQ(value, 24.0);
    }

    // Test where fails: Element is not a double
    try
    {
        const json element = R"( true )"_json;
        parseDouble(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a double");
    }

    // Test where fails: Variable specified: Variables map not specified
    try
    {
        const json element = R"( "${var}" )"_json;
        parseDouble(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a double");
    }

    // Test where fails: Variable specified: Leading whitespace
    try
    {
        std::map<std::string, std::string> variables{{"var", " -1.03"}};
        const json element = R"( "${var}" )"_json;
        parseDouble(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a double");
    }

    // Test where fails: Variable specified: Trailing whitespace
    try
    {
        std::map<std::string, std::string> variables{{"var", "-1.03 "}};
        const json element = R"( "${var}" )"_json;
        parseDouble(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a double");
    }

    // Test where fails: Variable specified: Starts with non-number character
    try
    {
        std::map<std::string, std::string> variables{{"var", "x-1.03"}};
        const json element = R"( "${var}" )"_json;
        parseDouble(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a double");
    }

    // Test where fails: Variable specified: Ends with non-number character
    try
    {
        std::map<std::string, std::string> variables{{"var", "-1.03x"}};
        const json element = R"( "${var}" )"_json;
        parseDouble(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a double");
    }

    // Test where fails: Variable specified: Not a double
    try
    {
        std::map<std::string, std::string> variables{{"var", "foo"}};
        const json element = R"( "${var}" )"_json;
        parseDouble(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a double");
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

    // Test where works: Variable specified
    {
        std::map<std::string, std::string> variables{{"var", "ed"}};
        const json element = R"( "0x${var}" )"_json;
        uint8_t value = parseHexByte(element, variables);
        EXPECT_EQ(value, 0xed);
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

    // Test where fails: Variable specified: Not a hex string
    try
    {
        std::map<std::string, std::string> variables{{"var", "0xsz"}};
        const json element = R"( "${var}" )"_json;
        parseHexByte(element, variables);
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

    // Test where works: Variables specified
    {
        std::map<std::string, std::string> variables{{"var1", "0xCC"},
                                                     {"var2", "0xFF"}};
        const json element = R"( [ "${var1}", "${var2}" ] )"_json;
        std::vector<uint8_t> hexBytes = parseHexByteArray(element, variables);
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

    // Test where fails: Variables specified: Invalid byte value
    try
    {
        std::map<std::string, std::string> variables{{"var1", "0xCC"},
                                                     {"var2", "99"}};
        const json element = R"( [ "${var1}", "${var2}" ] )"_json;
        parseHexByteArray(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not hexadecimal string");
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

    // Test where works: Variable specified
    {
        std::map<std::string, std::string> variables{{"var", "-23"}};
        const json element = R"( "${var}" )"_json;
        int8_t value = parseInt8(element, variables);
        EXPECT_EQ(value, -23);
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

    // Test where fails: Variable specified: Value > INT8_MAX
    try
    {
        std::map<std::string, std::string> variables{{"var", "128"}};
        const json element = R"( "${var}" )"_json;
        parseInt8(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit signed integer");
    }
}

TEST(JSONParserUtilsTests, ParseInteger)
{
    // Test where works: Zero
    {
        const json element = R"( 0 )"_json;
        int value = parseInteger(element);
        EXPECT_EQ(value, 0);
    }

    // Test where works: Positive value
    {
        const json element = R"( 103 )"_json;
        int value = parseInteger(element);
        EXPECT_EQ(value, 103);
    }

    // Test where works: Negative value
    {
        const json element = R"( -24 )"_json;
        int value = parseInteger(element);
        EXPECT_EQ(value, -24);
    }

    // Test where works: Variable specified: Positive value
    {
        std::map<std::string, std::string> variables{{"var", "1024"}};
        const json element = R"( "${var}" )"_json;
        int value = parseInteger(element, variables);
        EXPECT_EQ(value, 1024);
    }

    // Test where works: Variable specified: Negative value
    {
        std::map<std::string, std::string> variables{{"var", "-9924"}};
        const json element = R"( "${var}" )"_json;
        int value = parseInteger(element, variables);
        EXPECT_EQ(value, -9924);
    }

    // Test where fails: Element is not a integer
    try
    {
        const json element = R"( true )"_json;
        parseInteger(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Variable specified: Variables map not specified
    try
    {
        const json element = R"( "${var}" )"_json;
        parseInteger(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Variable specified: Leading whitespace
    try
    {
        std::map<std::string, std::string> variables{{"var", " -13"}};
        const json element = R"( "${var}" )"_json;
        parseInteger(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Variable specified: Trailing whitespace
    try
    {
        std::map<std::string, std::string> variables{{"var", "-13 "}};
        const json element = R"( "${var}" )"_json;
        parseInteger(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Variable specified: Starts with non-number character
    try
    {
        std::map<std::string, std::string> variables{{"var", "x-13"}};
        const json element = R"( "${var}" )"_json;
        parseInteger(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Variable specified: Ends with non-number character
    try
    {
        std::map<std::string, std::string> variables{{"var", "-13x"}};
        const json element = R"( "${var}" )"_json;
        parseInteger(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
    }

    // Test where fails: Variable specified: Not an integer
    try
    {
        std::map<std::string, std::string> variables{{"var", "foo"}};
        const json element = R"( "${var}" )"_json;
        parseInteger(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an integer");
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

    // Test where works: Variable specified: Empty string
    {
        std::map<std::string, std::string> variables{{"var", ""}};
        const json element = R"( "${var}" )"_json;
        std::string value = parseString(element, true, variables);
        EXPECT_EQ(value, "");
    }

    // Test where works: Variable specified: Non-empty string
    {
        std::map<std::string, std::string> variables{{"var", "vio_regulator"}};
        const json element = R"( "${var}" )"_json;
        std::string value = parseString(element, false, variables);
        EXPECT_EQ(value, "vio_regulator");
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

    // Test where fails: Variable specified: Empty string
    try
    {
        std::map<std::string, std::string> variables{{"var", ""}};
        const json element = R"( "${var}" )"_json;
        parseString(element, false, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element contains an empty string");
    }

    // Test where fails: Variable specified: Variable not defined
    try
    {
        std::map<std::string, std::string> variables{{"var1", "foo"}};
        const json element = R"( "${var2}" )"_json;
        parseString(element, false, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Undefined variable: var2");
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

    // Test where works: Variable specified
    {
        std::map<std::string, std::string> variables{{"var", "19"}};
        const json element = R"( "${var}" )"_json;
        uint8_t value = parseUint8(element, variables);
        EXPECT_EQ(value, 19);
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

    // Test where fails: Variable specified: Value > UINT8_MAX
    try
    {
        std::map<std::string, std::string> variables{{"var", "256"}};
        const json element = R"( "${var}" )"_json;
        parseUint8(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not an 8-bit unsigned integer");
    }
}

TEST(JSONParserUtilsTests, ParseUint16)
{
    // Test where works: 0
    {
        const json element = R"( 0 )"_json;
        uint16_t value = parseUint16(element);
        EXPECT_EQ(value, 0);
    }

    // Test where works: UINT16_MAX
    {
        const json element = R"( 65535 )"_json;
        uint16_t value = parseUint16(element);
        EXPECT_EQ(value, 65535);
    }

    // Test where works: Variable specified
    {
        std::map<std::string, std::string> variables{{"var", "24699"}};
        const json element = R"( "${var}" )"_json;
        uint16_t value = parseUint16(element, variables);
        EXPECT_EQ(value, 24699);
    }

    // Test where fails: Element is not an integer
    try
    {
        const json element = R"( 1.03 )"_json;
        parseUint16(element);
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
        parseUint16(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a 16-bit unsigned integer");
    }

    // Test where fails: Value > UINT16_MAX
    try
    {
        const json element = R"( 65536 )"_json;
        parseUint16(element);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a 16-bit unsigned integer");
    }

    // Test where fails: Variable specified: Value > UINT16_MAX
    try
    {
        std::map<std::string, std::string> variables{{"var", "65536"}};
        const json element = R"( "${var}" )"_json;
        parseUint16(element, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Element is not a 16-bit unsigned integer");
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

    // Test where works: Variable specified
    {
        std::map<std::string, std::string> variables{{"var", "25678"}};
        const json element = R"( "${var}" )"_json;
        unsigned int value = parseUnsignedInteger(element, variables);
        EXPECT_EQ(value, 25678);
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
        EXPECT_STREQ(e.what(), "Element is not an integer");
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

    // Test where fails: Variable specified: Value < 0
    try
    {
        std::map<std::string, std::string> variables{{"var", "-23"}};
        const json element = R"( "${var}" )"_json;
        parseUnsignedInteger(element, variables);
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

TEST(JSONParserUtilsTests, ExpandVariables)
{
    // Test where works: Single variable: Variable is entire value: Lower case
    // in variable name
    {
        std::map<std::string, std::string> variables{{"var", "vio_regulator"}};
        std::string value{"${var}"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "vio_regulator");
    }

    // Test where works: Multiple variables: Variables are part of value: Upper
    // case and underscore in variable name
    {
        std::map<std::string, std::string> variables{
            {"CHASSIS_NUMBER", "1"}, {"REGULATOR", "vcs_vio"}, {"RAIL", "vio"}};
        std::string value{
            "chassis${CHASSIS_NUMBER}_${REGULATOR}_regulator_${RAIL}_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "chassis1_vcs_vio_regulator_vio_rail");
    }

    // Test where works: Variable at start of value: Number in variable name
    {
        std::map<std::string, std::string> variables{{"var1", "vio_regulator"}};
        std::string value{"${var1}_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "vio_regulator_rail");
    }

    // Test where works: Variable at end of value
    {
        std::map<std::string, std::string> variables{{"chassis_number", "3"}};
        std::string value{
            "/xyz/openbmc_project/inventory/system/chassis${chassis_number}"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "/xyz/openbmc_project/inventory/system/chassis3");
    }

    // Test where works: Variable has empty value: Start of value
    {
        std::map<std::string, std::string> variables{{"chassis_prefix", ""}};
        std::string value{"${chassis_prefix}vio_regulator"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "vio_regulator");
    }

    // Test where works: Variable has empty value: Middle of value
    {
        std::map<std::string, std::string> variables{{"chassis_number", ""}};
        std::string value{"c${chassis_number}_vio_regulator"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "c_vio_regulator");
    }

    // Test where works: Variable has empty value: End of value
    {
        std::map<std::string, std::string> variables{{"chassis_number", ""}};
        std::string value{
            "/xyz/openbmc_project/inventory/system/chassis${chassis_number}"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "/xyz/openbmc_project/inventory/system/chassis");
    }

    // Test where works: No variables specified
    {
        std::map<std::string, std::string> variables{{"var", "vio_regulator"}};
        std::string value{"vcs_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "vcs_rail");
    }

    // Test where works: Nested variable expansion
    {
        std::map<std::string, std::string> variables{{"var1", "${var2}"},
                                                     {"var2", "vio_reg"}};
        std::string value{"${var1}_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "vio_reg_rail");
    }

    // Test where fails: Variables map is empty
    {
        std::map<std::string, std::string> variables{};
        std::string value{"${var}_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "${var}_rail");
    }

    // Test where fails: Variable missing $
    {
        std::map<std::string, std::string> variables{{"var", "vio_reg"}};
        std::string value{"{var}_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "{var}_rail");
    }

    // Test where fails: Variable missing {
    {
        std::map<std::string, std::string> variables{{"var", "vio_reg"}};
        std::string value{"$var}_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "$var}_rail");
    }

    // Test where fails: Variable missing }
    {
        std::map<std::string, std::string> variables{{"var", "vio_reg"}};
        std::string value{"${var_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "${var_rail");
    }

    // Test where fails: Variable missing name
    {
        std::map<std::string, std::string> variables{{"var", "vio_reg"}};
        std::string value{"${}_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "${}_rail");
    }

    // Test where fails: Variable name has invalid characters
    {
        std::map<std::string, std::string> variables{{"var-2", "vio_reg"}};
        std::string value{"${var-2}_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "${var-2}_rail");
    }

    // Test where fails: Variable has unexpected whitespace
    {
        std::map<std::string, std::string> variables{{"var", "vio_reg"}};
        std::string value{"${ var }_rail"};
        expandVariables(value, variables);
        EXPECT_EQ(value, "${ var }_rail");
    }

    // Test where fails: Undefined variable
    try
    {
        std::map<std::string, std::string> variables{{"var", "vio_reg"}};
        std::string value{"${foo}_rail"};
        expandVariables(value, variables);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Undefined variable: foo");
    }
}
