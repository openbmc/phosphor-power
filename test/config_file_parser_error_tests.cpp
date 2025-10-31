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
#include "config_file_parser_error.hpp"

#include <filesystem>

#include <gtest/gtest.h>

using ConfigFileParserError = phosphor::power::util::ConfigFileParserError;

TEST(ConfigFileParserErrorTests, Constructor)
{
    std::filesystem::path pathName{"/etc/phosphor-regulators/foo.json"};
    ConfigFileParserError error{pathName, "Required property missing"};
    EXPECT_EQ(error.getPathName(), pathName);
    EXPECT_STREQ(error.what(),
                 "ConfigFileParserError: /etc/phosphor-regulators/foo.json: "
                 "Required property missing");
}

TEST(ConfigFileParserErrorTests, GetPathName)
{
    std::filesystem::path pathName{"/usr/share/phosphor-regulators/foo.json"};
    ConfigFileParserError error{pathName, "Required property missing"};
    EXPECT_EQ(error.getPathName(), pathName);
}

TEST(ConfigFileParserErrorTests, What)
{
    std::filesystem::path pathName{"/etc/phosphor-regulators/foo.json"};
    ConfigFileParserError error{pathName, "Required property missing"};
    EXPECT_STREQ(error.what(),
                 "ConfigFileParserError: /etc/phosphor-regulators/foo.json: "
                 "Required property missing");
}
