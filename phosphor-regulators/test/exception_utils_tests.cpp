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

TEST(ExceptionUtilsTests, GetExceptions)
{
    // Test where exception pointer is null
    {
        std::exception_ptr eptr;
        std::vector<std::exception_ptr> exceptions =
            exception_utils::getExceptions(eptr);
        EXPECT_EQ(exceptions.size(), 0);
    }

    // Test where exception pointer is not null
    {
        // Create nested exception with two nesting levels
        std::exception_ptr eptr;
        try
        {
            try
            {
                throw std::logic_error{"JSON element is not an array"};
            }
            catch (...)
            {
                std::throw_with_nested(
                    std::runtime_error{"Unable to parse config file"});
            }
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Get vector containing exceptions
        std::vector<std::exception_ptr> exceptions =
            exception_utils::getExceptions(eptr);
        EXPECT_EQ(exceptions.size(), 2);

        // Verify first exception in vector, which is the innermost exception
        try
        {
            std::rethrow_exception(exceptions[0]);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::logic_error& e)
        {
            EXPECT_STREQ(e.what(), "JSON element is not an array");
        }
        catch (...)
        {
            ADD_FAILURE() << "Unexpected exception type";
        }

        // Verify second exception in vector, which is the outermost exception
        try
        {
            std::rethrow_exception(exceptions[1]);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(), "Unable to parse config file");
        }
        catch (...)
        {
            ADD_FAILURE() << "Unexpected exception type";
        }
    }
}

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

TEST(ExceptionUtilsTests, InternalGetExceptions)
{
    // Test where exception pointer is null
    {
        std::exception_ptr eptr;
        std::vector<std::exception_ptr> exceptions;
        exception_utils::internal::getExceptions(eptr, exceptions);
        EXPECT_EQ(exceptions.size(), 0);
    }

    // Test where exception is not nested
    {
        // Create exception
        std::exception_ptr eptr;
        try
        {
            throw std::logic_error{"JSON element is not an array"};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Build vector of exceptions
        std::vector<std::exception_ptr> exceptions;
        exception_utils::internal::getExceptions(eptr, exceptions);
        EXPECT_EQ(exceptions.size(), 1);

        // Verify exception in vector
        try
        {
            std::rethrow_exception(exceptions[0]);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::logic_error& e)
        {
            EXPECT_STREQ(e.what(), "JSON element is not an array");
        }
        catch (...)
        {
            ADD_FAILURE() << "Unexpected exception type";
        }
    }

    // Test where exception is nested
    {
        // Throw exception with three levels of nesting
        std::exception_ptr eptr;
        try
        {
            try
            {
                try
                {
                    throw std::string{"Invalid JSON element"};
                }
                catch (...)
                {
                    std::throw_with_nested(
                        std::logic_error{"JSON element is not an array"});
                }
            }
            catch (...)
            {
                std::throw_with_nested(
                    std::runtime_error{"Unable to parse config file"});
            }
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Build vector of exceptions
        std::vector<std::exception_ptr> exceptions;
        exception_utils::internal::getExceptions(eptr, exceptions);
        EXPECT_EQ(exceptions.size(), 3);

        // Verify first exception in vector, which is the innermost exception
        try
        {
            std::rethrow_exception(exceptions[0]);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::string& s)
        {
            EXPECT_EQ(s, "Invalid JSON element");
        }
        catch (...)
        {
            ADD_FAILURE() << "Unexpected exception type";
        }

        // Verify second exception in vector
        try
        {
            std::rethrow_exception(exceptions[1]);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::logic_error& e)
        {
            EXPECT_STREQ(e.what(), "JSON element is not an array");
        }
        catch (...)
        {
            ADD_FAILURE() << "Unexpected exception type";
        }

        // Verify third exception in vector, which is the outermost exception
        try
        {
            std::rethrow_exception(exceptions[2]);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::runtime_error& e)
        {
            EXPECT_STREQ(e.what(), "Unable to parse config file");
        }
        catch (...)
        {
            ADD_FAILURE() << "Unexpected exception type";
        }
    }
}

TEST(ExceptionUtilsTests, InternalGetMessages)
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
