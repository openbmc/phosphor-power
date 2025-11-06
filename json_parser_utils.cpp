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

#include <charconv>
#include <regex>

namespace phosphor::power::json_parser_utils
{

const std::map<std::string, std::string> NO_VARIABLES{};

static std::regex VARIABLE_REGEX{R"(\$\{([A-Za-z0-9_]+)\})"};

uint8_t parseBitPosition(const nlohmann::json& element,
                         const std::map<std::string, std::string>& variables)
{
    int value = parseInteger(element, variables);
    if ((value < 0) || (value > 7))
    {
        throw std::invalid_argument{"Element is not a bit position"};
    }
    return static_cast<uint8_t>(value);
}

uint8_t parseBitValue(const nlohmann::json& element,
                      const std::map<std::string, std::string>& variables)
{
    int value = parseInteger(element, variables);
    if ((value < 0) || (value > 1))
    {
        throw std::invalid_argument{"Element is not a bit value"};
    }
    return static_cast<uint8_t>(value);
}

bool parseBoolean(const nlohmann::json& element,
                  const std::map<std::string, std::string>& variables)
{
    if (element.is_boolean())
    {
        return element.get<bool>();
    }

    if (element.is_string() && !variables.empty())
    {
        std::string value = parseString(element, true, variables);
        if (value == "true")
        {
            return true;
        }
        else if (value == "false")
        {
            return false;
        }
    }

    throw std::invalid_argument{"Element is not a boolean"};
}

double parseDouble(const nlohmann::json& element,
                   const std::map<std::string, std::string>& variables)
{
    if (element.is_number())
    {
        return element.get<double>();
    }

    if (element.is_string() && !variables.empty())
    {
        std::string strValue = parseString(element, true, variables);
        const char* first = strValue.data();
        const char* last = strValue.data() + strValue.size();
        double value;
        auto [ptr, ec] = std::from_chars(first, last, value);
        if ((ptr == last) && (ec == std::errc()))
        {
            return value;
        }
    }

    throw std::invalid_argument{"Element is not a double"};
}

uint8_t parseHexByte(const nlohmann::json& element,
                     const std::map<std::string, std::string>& variables)
{
    std::string value = parseString(element, true, variables);
    bool isHex = (value.compare(0, 2, "0x") == 0) && (value.size() > 2) &&
                 (value.size() < 5) &&
                 (value.find_first_not_of("0123456789abcdefABCDEF", 2) ==
                  std::string::npos);
    if (!isHex)
    {
        throw std::invalid_argument{"Element is not hexadecimal string"};
    }
    return static_cast<uint8_t>(std::stoul(value, nullptr, 0));
}

std::vector<uint8_t> parseHexByteArray(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables)
{
    verifyIsArray(element);
    std::vector<uint8_t> values;
    for (auto& valueElement : element)
    {
        values.emplace_back(parseHexByte(valueElement, variables));
    }
    return values;
}

int8_t parseInt8(const nlohmann::json& element,
                 const std::map<std::string, std::string>& variables)
{
    int value = parseInteger(element, variables);
    if ((value < INT8_MIN) || (value > INT8_MAX))
    {
        throw std::invalid_argument{"Element is not an 8-bit signed integer"};
    }
    return static_cast<int8_t>(value);
}

int parseInteger(const nlohmann::json& element,
                 const std::map<std::string, std::string>& variables)
{
    if (element.is_number_integer())
    {
        return element.get<int>();
    }

    if (element.is_string() && !variables.empty())
    {
        std::string strValue = parseString(element, true, variables);
        const char* first = strValue.data();
        const char* last = strValue.data() + strValue.size();
        int value;
        auto [ptr, ec] = std::from_chars(first, last, value);
        if ((ptr == last) && (ec == std::errc()))
        {
            return value;
        }
    }

    throw std::invalid_argument{"Element is not an integer"};
}

std::string parseString(const nlohmann::json& element, bool isEmptyValid,
                        const std::map<std::string, std::string>& variables)
{
    if (!element.is_string())
    {
        throw std::invalid_argument{"Element is not a string"};
    }
    std::string value = element.get<std::string>();
    internal::expandVariables(value, variables);
    if (value.empty() && !isEmptyValid)
    {
        throw std::invalid_argument{"Element contains an empty string"};
    }
    return value;
}

uint8_t parseUint8(const nlohmann::json& element,
                   const std::map<std::string, std::string>& variables)
{
    int value = parseInteger(element, variables);
    if ((value < 0) || (value > UINT8_MAX))
    {
        throw std::invalid_argument{"Element is not an 8-bit unsigned integer"};
    }
    return static_cast<uint8_t>(value);
}

uint16_t parseUint16(const nlohmann::json& element,
                     const std::map<std::string, std::string>& variables)
{
    int value = parseInteger(element, variables);
    if ((value < 0) || (value > UINT16_MAX))
    {
        throw std::invalid_argument{"Element is not a 16-bit unsigned integer"};
    }
    return static_cast<uint16_t>(value);
}

unsigned int parseUnsignedInteger(
    const nlohmann::json& element,
    const std::map<std::string, std::string>& variables)
{
    int value = parseInteger(element, variables);
    if (value < 0)
    {
        throw std::invalid_argument{"Element is not an unsigned integer"};
    }
    return static_cast<unsigned int>(value);
}

namespace internal
{

void expandVariables(std::string& value,
                     const std::map<std::string, std::string>& variables)
{
    if (variables.empty())
    {
        return;
    }

    std::smatch results;
    while (std::regex_search(value, results, VARIABLE_REGEX))
    {
        if (results.size() != 2)
        {
            throw std::runtime_error{
                "Unexpected regular expression match result while parsing string"};
        }
        const std::string& variable = results[1];
        auto it = variables.find(variable);
        if (it == variables.end())
        {
            throw std::invalid_argument{"Undefined variable: " + variable};
        }
        value.replace(results.position(0), results.length(0), it->second);
    }
}

} // namespace internal

} // namespace phosphor::power::json_parser_utils
