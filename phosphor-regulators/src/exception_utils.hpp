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
#pragma once

#include "journal.hpp"

#include <exception>
#include <string>
#include <vector>

/**
 * @namespace exception_utils
 *
 * Contains utility functions for handling exceptions.
 */
namespace phosphor::power::regulators::exception_utils
{

/**
 * Returns a vector containing the specified exception and any nested inner
 * exceptions.
 *
 * If the exception contains nested inner exceptions, the returned vector will
 * be ordered from innermost exception to outermost exception.
 *
 * This function makes it easier to handle nested exceptions.  You can iterate
 * over them in a simple loop instead of writing a recursive function.
 *
 * @param eptr exception pointer
 * @return vector of exceptions, from innermost to outermost
 */
std::vector<std::exception_ptr> getExceptions(std::exception_ptr eptr);

/**
 * Gets the error messages from the specified exception and any nested inner
 * exceptions.
 *
 * If the exception contains nested inner exceptions, the messages in the
 * returned vector will be ordered from innermost exception to outermost
 * exception.
 *
 * @param e exception
 * @return error messages from exceptions
 */
std::vector<std::string> getMessages(const std::exception& e);

/*
 * Internal implementation details
 */
namespace internal
{

/**
 * Builds a vector containing the specified exception and any nested inner
 * exceptions.
 *
 * Stores the exceptions in the specified vector, from innermost exception to
 * outermost exception.
 *
 * @param eptr exception pointer
 * @param exceptions vector where exceptions will be stored
 */
void getExceptions(std::exception_ptr eptr,
                   std::vector<std::exception_ptr>& exceptions);

/**
 * Gets the error messages from the specified exception and any nested inner
 * exceptions.
 *
 * Stores the error messages in the specified vector, from innermost exception
 * to outermost exception.
 *
 * @param e exception
 * @param messages vector where error messages will be stored
 */
void getMessages(const std::exception& e, std::vector<std::string>& messages);

} // namespace internal

} // namespace phosphor::power::regulators::exception_utils
