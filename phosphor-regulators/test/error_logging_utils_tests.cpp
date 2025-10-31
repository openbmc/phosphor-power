/**
 * Copyright © 2021 IBM Corporation
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
#include "error_logging.hpp"
#include "error_logging_utils.hpp"
#include "i2c_interface.hpp"
#include "journal.hpp"
#include "mock_error_logging.hpp"
#include "mock_journal.hpp"
#include "mock_services.hpp"
#include "pmbus_error.hpp"
#include "test_sdbus_error.hpp"
#include "write_verification_error.hpp"

#include <errno.h>

#include <sdbusplus/exception.hpp>

#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using ConfigFileParserError = phosphor::power::util::ConfigFileParserError;

namespace fs = std::filesystem;

using ::testing::Ref;

TEST(ErrorLoggingUtilsTests, LogError_3Parameters)
{
    // Create exception with two nesting levels; top priority is inner
    // PMBusError
    std::exception_ptr eptr;
    try
    {
        try
        {
            throw PMBusError{"VOUT_MODE contains unsupported data format",
                             "reg1",
                             "/xyz/openbmc_project/inventory/system/chassis/"
                             "motherboard/reg1"};
        }
        catch (...)
        {
            std::throw_with_nested(
                std::runtime_error{"Unable to set output voltage"});
        }
    }
    catch (...)
    {
        eptr = std::current_exception();
    }

    // Create MockServices.  Expect logPMBusError() to be called.
    MockServices services{};
    MockErrorLogging& errorLogging = services.getMockErrorLogging();
    MockJournal& journal = services.getMockJournal();
    EXPECT_CALL(
        errorLogging,
        logPMBusError(
            Entry::Level::Error, Ref(journal),
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1"))
        .Times(1);

    // Log error based on the nested exception
    error_logging_utils::logError(eptr, Entry::Level::Error, services);
}

TEST(ErrorLoggingUtilsTests, LogError_4Parameters)
{
    // Test where exception pointer is null
    {
        std::exception_ptr eptr;

        // Create MockServices.  Don't expect any log*() methods to be called.
        MockServices services{};

        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Error, services,
                                      history);
    }

    // Test where exception is not nested
    {
        std::exception_ptr eptr;
        try
        {
            throw i2c::I2CException{"Unable to open device reg1", "/dev/i2c-8",
                                    0x30, ENODEV};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logI2CError() to be called.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging,
                    logI2CError(Entry::Level::Critical, Ref(journal),
                                "/dev/i2c-8", 0x30, ENODEV))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Critical, services,
                                      history);
    }

    // Test where exception is nested
    {
        std::exception_ptr eptr;
        try
        {
            try
            {
                throw std::invalid_argument{"JSON element is not an array"};
            }
            catch (...)
            {
                std::throw_with_nested(ConfigFileParserError{
                    fs::path{"/etc/phosphor-regulators/config.json"},
                    "Unable to parse JSON configuration file"});
            }
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logConfigFileError() to be called.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging,
                    logConfigFileError(Entry::Level::Warning, Ref(journal)))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Warning, services,
                                      history);
    }

    // Test where exception is a ConfigFileParserError
    {
        std::exception_ptr eptr;
        try
        {
            throw ConfigFileParserError{
                fs::path{"/etc/phosphor-regulators/config.json"},
                "Unable to parse JSON configuration file"};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logConfigFileError() to be called once.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging,
                    logConfigFileError(Entry::Level::Error, Ref(journal)))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Error, services,
                                      history);

        // Try to log error again.  Should not happen due to ErrorHistory.
        error_logging_utils::logError(eptr, Entry::Level::Error, services,
                                      history);
    }

    // Test where exception is a PMBusError
    {
        std::exception_ptr eptr;
        try
        {
            throw PMBusError{"VOUT_MODE contains unsupported data format",
                             "reg1",
                             "/xyz/openbmc_project/inventory/system/chassis/"
                             "motherboard/reg1"};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logPMBusError() to be called once.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging,
                    logPMBusError(Entry::Level::Error, Ref(journal),
                                  "/xyz/openbmc_project/inventory/system/"
                                  "chassis/motherboard/reg1"))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Error, services,
                                      history);

        // Try to log error again.  Should not happen due to ErrorHistory.
        error_logging_utils::logError(eptr, Entry::Level::Error, services,
                                      history);
    }

    // Test where exception is a WriteVerificationError
    {
        std::exception_ptr eptr;
        try
        {
            throw WriteVerificationError{
                "value_written: 0xDEAD, value_read: 0xBEEF", "reg1",
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                "reg1"};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logWriteVerificationError() to be
        // called once.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging, logWriteVerificationError(
                                      Entry::Level::Warning, Ref(journal),
                                      "/xyz/openbmc_project/inventory/system/"
                                      "chassis/motherboard/reg1"))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Warning, services,
                                      history);

        // Try to log error again.  Should not happen due to ErrorHistory.
        error_logging_utils::logError(eptr, Entry::Level::Warning, services,
                                      history);
    }

    // Test where exception is a I2CException
    {
        std::exception_ptr eptr;
        try
        {
            throw i2c::I2CException{"Unable to open device reg1", "/dev/i2c-8",
                                    0x30, ENODEV};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logI2CError() to be called once.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging,
                    logI2CError(Entry::Level::Informational, Ref(journal),
                                "/dev/i2c-8", 0x30, ENODEV))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Informational,
                                      services, history);

        // Try to log error again.  Should not happen due to ErrorHistory.
        error_logging_utils::logError(eptr, Entry::Level::Informational,
                                      services, history);
    }

    // Test where exception is a sdbusplus::exception_t
    {
        std::exception_ptr eptr;
        try
        {
            // Throw TestSDBusError; exception_t is a pure virtual base class
            throw TestSDBusError{"DBusError: Invalid object path."};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logDBusError() to be called once.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging,
                    logDBusError(Entry::Level::Debug, Ref(journal)))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Debug, services,
                                      history);

        // Try to log error again.  Should not happen due to ErrorHistory.
        error_logging_utils::logError(eptr, Entry::Level::Debug, services,
                                      history);
    }

    // Test where exception is a std::exception
    {
        std::exception_ptr eptr;
        try
        {
            throw std::runtime_error{
                "Unable to read configuration file: No such file or directory"};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logInternalError() to be called once.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging,
                    logInternalError(Entry::Level::Error, Ref(journal)))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Error, services,
                                      history);

        // Try to log error again.  Should not happen due to ErrorHistory.
        error_logging_utils::logError(eptr, Entry::Level::Error, services,
                                      history);
    }

    // Test where exception is unknown type
    {
        std::exception_ptr eptr;
        try
        {
            throw 23;
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        // Create MockServices.  Expect logInternalError() to be called once.
        MockServices services{};
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(errorLogging,
                    logInternalError(Entry::Level::Warning, Ref(journal)))
            .Times(1);

        // Log error based on the nested exception
        ErrorHistory history{};
        error_logging_utils::logError(eptr, Entry::Level::Warning, services,
                                      history);

        // Try to log error again.  Should not happen due to ErrorHistory.
        error_logging_utils::logError(eptr, Entry::Level::Warning, services,
                                      history);
    }
}

TEST(ErrorLoggingUtilsTests, GetExceptionToLog)
{
    // Test where exception is not nested
    {
        std::exception_ptr eptr;
        try
        {
            throw i2c::I2CException{"Unable to open device reg1", "/dev/i2c-8",
                                    0x30, ENODEV};
        }
        catch (...)
        {
            eptr = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(eptr);
        EXPECT_EQ(eptr, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is innermost exception
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw PMBusError{
                    "VOUT_MODE contains unsupported data format", "reg1",
                    "/xyz/openbmc_project/inventory/system/chassis/"
                    "motherboard/reg1"};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(
                    std::runtime_error{"Unable to set output voltage"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(inner, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is middle exception
    {
        std::exception_ptr inner, middle, outer;
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
                    inner = std::current_exception();
                    std::throw_with_nested(ConfigFileParserError{
                        fs::path{"/etc/phosphor-regulators/config.json"},
                        "Unable to parse JSON configuration file"});
                }
            }
            catch (...)
            {
                middle = std::current_exception();
                std::throw_with_nested(
                    std::runtime_error{"Unable to load config file"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(middle, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is outermost exception
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw std::invalid_argument{"JSON element is not an array"};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(ConfigFileParserError{
                    fs::path{"/etc/phosphor-regulators/config.json"},
                    "Unable to parse JSON configuration file"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(outer, exceptionToLog);
    }

    // Test where exception is nested: Two exceptions have same priority.
    // Should return outermost exception with that priority.
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw std::invalid_argument{"JSON element is not an array"};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(
                    std::runtime_error{"Unable to load config file"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(outer, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is ConfigFileParserError
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw ConfigFileParserError{
                    fs::path{"/etc/phosphor-regulators/config.json"},
                    "Unable to parse JSON configuration file"};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(
                    std::runtime_error{"Unable to load config file"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(inner, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is PMBusError
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw std::invalid_argument{"Invalid VOUT_MODE value"};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(PMBusError{
                    "VOUT_MODE contains unsupported data format", "reg1",
                    "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                    "reg1"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(outer, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is
    // WriteVerificationError
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw WriteVerificationError{
                    "value_written: 0xDEAD, value_read: 0xBEEF", "reg1",
                    "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                    "reg1"};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(
                    std::runtime_error{"Unable set voltage"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(inner, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is I2CException
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw std::invalid_argument{"No such device"};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(i2c::I2CException{
                    "Unable to open device reg1", "/dev/i2c-8", 0x30, ENODEV});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(outer, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is
    // sdbusplus::exception_t
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                // Throw TestSDBusError; exception_t is pure virtual class
                throw TestSDBusError{"DBusError: Invalid object path."};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(
                    std::runtime_error{"Unable to call D-Bus method"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(inner, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is std::exception
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw std::invalid_argument{"No such file or directory"};
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(
                    std::runtime_error{"Unable load config file"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(outer, exceptionToLog);
    }

    // Test where exception is nested: Highest priority is unknown type
    {
        std::exception_ptr inner, outer;
        try
        {
            try
            {
                throw 23;
            }
            catch (...)
            {
                inner = std::current_exception();
                std::throw_with_nested(std::string{"Unable load config file"});
            }
        }
        catch (...)
        {
            outer = std::current_exception();
        }

        std::exception_ptr exceptionToLog =
            error_logging_utils::internal::getExceptionToLog(outer);
        EXPECT_EQ(outer, exceptionToLog);
    }
}
