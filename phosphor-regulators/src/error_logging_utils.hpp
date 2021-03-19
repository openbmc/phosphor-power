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
#pragma once

#include "error_history.hpp"
#include "error_logging.hpp"
#include "services.hpp"

#include <exception>

/**
 * @namespace error_logging_utils
 *
 * Contains utility functions for logging errors.
 */
namespace phosphor::power::regulators::error_logging_utils
{

/**
 * Logs an error based on the specified exception and any nested inner
 * exceptions.
 *
 * @param eptr exception pointer
 * @param severity severity level
 * @param services system services like error logging and the journal
 */
void logError(std::exception_ptr eptr, Entry::Level severity,
              Services& services);

/**
 * Logs an error, if necessary, based on the specified exception and any nested
 * inner exceptions.
 *
 * Finds the error type would be logged based on the specified exception and any
 * nested inner exceptions.
 *
 * Checks to see if this error type has already been logged according to the
 * specified ErrorHistory object.
 *
 * If the error type has not been logged, an error log entry is created, and the
 * ErrorHistory is updated.
 *
 * If the error type has been logged, no further action is taken.
 *
 * @param eptr exception pointer
 * @param severity severity level
 * @param services system services like error logging and the journal
 * @param history error logging history
 */
void logError(std::exception_ptr eptr, Entry::Level severity,
              Services& services, ErrorHistory& history);

/*
 * Internal implementation details
 */
namespace internal
{

/**
 * Returns the exception to use when logging an error.
 *
 * Inspects the specified exception and any nested inner exceptions.  Returns
 * the highest priority exception from an error logging perspective.
 *
 * @param eptr exception pointer
 * @return exception to log
 */
std::exception_ptr getExceptionToLog(std::exception_ptr eptr);

} // namespace internal

} // namespace phosphor::power::regulators::error_logging_utils
