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

#include <exception>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class WriteVerificationError
 *
 * An error that occurred when performing a read after a write to ensure the
 * write was successful.
 *
 * This exception is thrown when the value read is different than the value
 * written.  This is also known as a readback error.
 */
class WriteVerificationError : public std::exception
{
  public:
    // Specify which compiler-generated methods we want
    WriteVerificationError() = delete;
    WriteVerificationError(const WriteVerificationError&) = default;
    WriteVerificationError(WriteVerificationError&&) = default;
    WriteVerificationError& operator=(const WriteVerificationError&) = default;
    WriteVerificationError& operator=(WriteVerificationError&&) = default;
    virtual ~WriteVerificationError() = default;

    /**
     * Constructor.
     *
     * @param error error message
     */
    explicit WriteVerificationError(const std::string& error) :
        error{"WriteVerificationError: " + error}
    {
    }

    /**
     * Returns the description of this error.
     *
     * @return error description
     */
    const char* what() const noexcept override
    {
        return error.c_str();
    }

  private:
    /**
     * Error message.
     */
    const std::string error{};
};

} // namespace phosphor::power::regulators
