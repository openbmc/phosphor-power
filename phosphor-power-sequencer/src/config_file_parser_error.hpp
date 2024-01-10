/**
 * Copyright © 2024 IBM Corporation
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
#include <filesystem>
#include <string>

namespace phosphor::power::sequencer
{

/**
 * @class ConfigFileParserError
 *
 * An error that occurred while parsing a JSON configuration file.
 */
class ConfigFileParserError : public std::exception
{
  public:
    // Specify which compiler-generated methods we want
    ConfigFileParserError() = delete;
    ConfigFileParserError(const ConfigFileParserError&) = default;
    ConfigFileParserError(ConfigFileParserError&&) = default;
    ConfigFileParserError& operator=(const ConfigFileParserError&) = default;
    ConfigFileParserError& operator=(ConfigFileParserError&&) = default;
    virtual ~ConfigFileParserError() = default;

    /**
     * Constructor.
     *
     * @param pathName Configuration file path name
     * @param error Error message
     */
    explicit ConfigFileParserError(const std::filesystem::path& pathName,
                                   const std::string& error) :
        pathName{pathName},
        error{"ConfigFileParserError: " + pathName.string() + ": " + error}
    {}

    /**
     * Returns the configuration file path name.
     *
     * @return path name
     */
    const std::filesystem::path& getPathName()
    {
        return pathName;
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
     * Configuration file path name.
     */
    const std::filesystem::path pathName;

    /**
     * Error message.
     */
    const std::string error{};
};

} // namespace phosphor::power::sequencer
