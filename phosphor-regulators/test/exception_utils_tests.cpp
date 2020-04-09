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
#include "exception_utils.hpp"
#include "journal.hpp"
#include "mock_journal.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(ExceptionUtilsTests, GetMessages)
{
    try
    {
        try
        {
            throw std::invalid_argument{"JSON element is not an array"};
        }
        catch (...)
        {
            std::throw_with_nested(
                std::logic_error{"Unable to parse config file"});
        }
    }
    catch (const std::exception& e)
    {
        std::vector<std::string> messages = exception_utils::getMessages(e);
        EXPECT_EQ(messages.size(), 2);
        EXPECT_EQ(messages[0], "JSON element is not an array");
        EXPECT_EQ(messages[1], "Unable to parse config file");
    }
}

TEST(ExceptionUtilsTests, Log)
{
    try
    {
        try
        {
            throw std::invalid_argument{"JSON element is not an array"};
        }
        catch (...)
        {
            std::throw_with_nested(
                std::logic_error{"Unable to parse config file"});
        }
    }
    catch (const std::exception& e)
    {
        journal::clear();
        exception_utils::log(e);
        const std::vector<std::string>& messages = journal::getErrMessages();
        EXPECT_EQ(messages.size(), 2);
        EXPECT_EQ(messages[0], "JSON element is not an array");
        EXPECT_EQ(messages[1], "Unable to parse config file");
    }
}

// Test for getMessages() function in the internal namespace
TEST(ExceptionUtilsTests, GetMessagesInternal)
{
    // Test where exception is not nested
    {
        std::invalid_argument e{"JSON element is not an array"};
        std::vector<std::string> messages{};
        exception_utils::internal::getMessages(e, messages);
        EXPECT_EQ(messages.size(), 1);
        EXPECT_EQ(messages[0], "JSON element is not an array");
    }

    // Test where exception is nested
    try
    {
        try
        {
            try
            {
                throw std::invalid_argument{"JSON element is not an array"};
            }
            catch (...)
            {
                std::throw_with_nested(
                    std::logic_error{"Unable to parse config file"});
            }
        }
        catch (...)
        {
            std::throw_with_nested(
                std::runtime_error{"Unable to configure regulators"});
        }
    }
    catch (const std::exception& e)
    {
        std::vector<std::string> messages{};
        exception_utils::internal::getMessages(e, messages);
        EXPECT_EQ(messages.size(), 3);
        EXPECT_EQ(messages[0], "JSON element is not an array");
        EXPECT_EQ(messages[1], "Unable to parse config file");
        EXPECT_EQ(messages[2], "Unable to configure regulators");
    }

    // Test where nested exception is not a child of std::exception
    try
    {
        try
        {
            try
            {
                throw "JSON element is not an array";
            }
            catch (...)
            {
                std::throw_with_nested(
                    std::logic_error{"Unable to parse config file"});
            }
        }
        catch (...)
        {
            std::throw_with_nested(
                std::runtime_error{"Unable to configure regulators"});
        }
    }
    catch (const std::exception& e)
    {
        std::vector<std::string> messages{};
        exception_utils::internal::getMessages(e, messages);
        EXPECT_EQ(messages.size(), 2);
        EXPECT_EQ(messages[0], "Unable to parse config file");
        EXPECT_EQ(messages[1], "Unable to configure regulators");
    }
}
