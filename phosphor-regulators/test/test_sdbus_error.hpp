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

#include <sdbusplus/exception.hpp>

#include <string>

namespace phosphor::power::regulators
{

/**
 * Concrete subclass of sdbusplus::exception_t abstract base class.
 */
class TestSDBusError : public sdbusplus::exception_t
{
  public:
    TestSDBusError(const std::string& error) : error{error} {}

    const char* what() const noexcept override
    {
        return error.c_str();
    }

    const char* name() const noexcept override
    {
        return "";
    }

    const char* description() const noexcept override
    {
        return "";
    }

    int get_errno() const noexcept override
    {
        return EIO;
    }

  private:
    const std::string error{};
};

} // namespace phosphor::power::regulators
