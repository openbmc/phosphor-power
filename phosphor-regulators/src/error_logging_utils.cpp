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

#include "error_logging_utils.hpp"

#include "config_file_parser_error.hpp"
#include "exception_utils.hpp"
#include "i2c_interface.hpp"
#include "journal.hpp"
#include "pmbus_error.hpp"
#include "write_verification_error.hpp"

#include <sdbusplus/exception.hpp>

#include <vector>

using ConfigFileParserError = phosphor::power::util::ConfigFileParserError;

namespace phosphor::power::regulators::error_logging_utils
{

void logError(std::exception_ptr eptr, Entry::Level severity,
              Services& services)
{
    // Specify empty error history so that all error types will be logged
    ErrorHistory history{};
    logError(eptr, severity, services, history);
}

void logError(std::exception_ptr eptr, Entry::Level severity,
              Services& services, ErrorHistory& history)
{
    // Check for null exception pointer
    if (!eptr)
    {
        return;
    }

    // Get exception to log from specified exception and any nested exceptions
    std::exception_ptr exceptionToLog = internal::getExceptionToLog(eptr);

    // Log an error based on the exception
    ErrorLogging& errorLogging = services.getErrorLogging();
    Journal& journal = services.getJournal();
    ErrorType errorType{};
    try
    {
        std::rethrow_exception(exceptionToLog);
    }
    catch (const ConfigFileParserError& e)
    {
        errorType = ErrorType::configFile;
        if (!history.wasLogged(errorType))
        {
            history.setWasLogged(errorType, true);
            errorLogging.logConfigFileError(severity, journal);
        }
    }
    catch (const PMBusError& e)
    {
        errorType = ErrorType::pmbus;
        if (!history.wasLogged(errorType))
        {
            history.setWasLogged(errorType, true);
            errorLogging.logPMBusError(severity, journal, e.getInventoryPath());
        }
    }
    catch (const WriteVerificationError& e)
    {
        errorType = ErrorType::writeVerification;
        if (!history.wasLogged(errorType))
        {
            history.setWasLogged(errorType, true);
            errorLogging.logWriteVerificationError(severity, journal,
                                                   e.getInventoryPath());
        }
    }
    catch (const i2c::I2CException& e)
    {
        errorType = ErrorType::i2c;
        if (!history.wasLogged(errorType))
        {
            history.setWasLogged(errorType, true);
            errorLogging.logI2CError(severity, journal, e.bus, e.addr,
                                     e.errorCode);
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        errorType = ErrorType::dbus;
        if (!history.wasLogged(errorType))
        {
            history.setWasLogged(errorType, true);
            errorLogging.logDBusError(severity, journal);
        }
    }
    catch (const std::exception& e)
    {
        errorType = ErrorType::internal;
        if (!history.wasLogged(errorType))
        {
            history.setWasLogged(errorType, true);
            errorLogging.logInternalError(severity, journal);
        }
    }
    catch (...)
    {
        errorType = ErrorType::internal;
        if (!history.wasLogged(errorType))
        {
            history.setWasLogged(errorType, true);
            errorLogging.logInternalError(severity, journal);
        }
    }
}

namespace internal
{

std::exception_ptr getExceptionToLog(std::exception_ptr eptr)
{
    // Default to selecting the outermost exception
    std::exception_ptr exceptionToLog{eptr};

    // Get vector containing this exception and any nested exceptions
    std::vector<std::exception_ptr> exceptions =
        exception_utils::getExceptions(eptr);

    // Define temporary constants for exception priorities
    const int lowPriority{0}, mediumPriority{1}, highPriority{2};

    // Loop through the exceptions from innermost to outermost.  Find the
    // exception with the highest priority.  If there is a tie, select the
    // outermost exception with that priority.
    int highestPriorityFound{-1};
    for (std::exception_ptr curptr : exceptions)
    {
        int priority{-1};
        try
        {
            std::rethrow_exception(curptr);
        }
        catch (const ConfigFileParserError& e)
        {
            priority = highPriority;
        }
        catch (const PMBusError& e)
        {
            priority = highPriority;
        }
        catch (const WriteVerificationError& e)
        {
            priority = highPriority;
        }
        catch (const i2c::I2CException& e)
        {
            priority = highPriority;
        }
        catch (const sdbusplus::exception_t& e)
        {
            priority = mediumPriority;
        }
        catch (const std::exception& e)
        {
            priority = lowPriority;
        }
        catch (...)
        {
            priority = lowPriority;
        }

        if (priority >= highestPriorityFound)
        {
            highestPriorityFound = priority;
            exceptionToLog = curptr;
        }
    }

    return exceptionToLog;
}

} // namespace internal

} // namespace phosphor::power::regulators::error_logging_utils
