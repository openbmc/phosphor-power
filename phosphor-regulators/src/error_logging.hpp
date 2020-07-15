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

#include <sdbusplus/bus.hpp>

namespace phosphor::power::regulators
{

/**
 * @class ErrorLogging
 *
 * Abstract base class that provides an error logging interface.
 *
 * The interface is used to create error logs.
 */
class ErrorLogging
{
  public:
    // Specify which compiler-generated methods we want
    ErrorLogging() = default;
    ErrorLogging(const ErrorLogging&) = delete;
    ErrorLogging(ErrorLogging&&) = delete;
    ErrorLogging& operator=(const ErrorLogging&) = delete;
    ErrorLogging& operator=(ErrorLogging&&) = delete;
    virtual ~ErrorLogging() = default;

    // TODO: The following are stubs.  Add parameters and doxygen later.
    virtual void logConfigFileError() = 0;
    virtual void logDBusError() = 0;
    virtual void logI2CError() = 0;
    virtual void logInternalError() = 0;
    virtual void logPMBusError() = 0;
    virtual void logWriteVerificationError() = 0;
};

/**
 * @class DBusErrorLogging
 *
 * Implementation of the ErrorLogging interface using D-Bus method calls.
 */
class DBusErrorLogging : public ErrorLogging
{
  public:
    // Specify which compiler-generated methods we want
    DBusErrorLogging() = delete;
    DBusErrorLogging(const DBusErrorLogging&) = delete;
    DBusErrorLogging(DBusErrorLogging&&) = delete;
    DBusErrorLogging& operator=(const DBusErrorLogging&) = delete;
    DBusErrorLogging& operator=(DBusErrorLogging&&) = delete;
    virtual ~DBusErrorLogging() = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit DBusErrorLogging(sdbusplus::bus::bus& bus) : bus{bus}
    {
    }

    // TODO: The following are stubs.  Add parameters, implementation, and
    // doxygen later.
    virtual void logConfigFileError(){};
    virtual void logDBusError(){};
    virtual void logI2CError(){};
    virtual void logInternalError(){};
    virtual void logPMBusError(){};
    virtual void logWriteVerificationError(){};

  private:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus::bus& bus;
};

} // namespace phosphor::power::regulators
