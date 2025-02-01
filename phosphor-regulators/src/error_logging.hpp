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

#include "ffdc_file.hpp"
#include "journal.hpp"
#include "phase_fault.hpp"
#include "xyz/openbmc_project/Logging/Create/server.hpp"
#include "xyz/openbmc_project/Logging/Entry/server.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::power::regulators
{

using namespace sdbusplus::xyz::openbmc_project::Logging::server;
using FFDCTuple =
    std::tuple<FFDCFormat, uint8_t, uint8_t, sdbusplus::message::unix_fd>;

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

    /**
     * Log a regulators configuration file error.
     *
     * This error is logged when the regulators configuration file could not be
     * found, could not be read, or had invalid contents.
     *
     * @param severity severity level
     * @param journal system journal
     */
    virtual void logConfigFileError(Entry::Level severity,
                                    Journal& journal) = 0;

    /**
     * Log a D-Bus error.
     *
     * This error is logged when D-Bus communication fails.
     *
     * @param severity severity level
     * @param journal system journal
     */
    virtual void logDBusError(Entry::Level severity, Journal& journal) = 0;

    /**
     * Log an I2C communication error.
     *
     * @param severity severity level
     * @param journal system journal
     * @param bus I2C bus in the form "/dev/i2c-X", where X is the 0-based bus
     *            number
     * @param addr 7 bit I2C address
     * @param errorNumber errno value from the failed I2C operation
     */
    virtual void logI2CError(Entry::Level severity, Journal& journal,
                             const std::string& bus, uint8_t addr,
                             int errorNumber) = 0;

    /**
     * Log an internal firmware error.
     *
     * @param severity severity level
     * @param journal system journal
     */
    virtual void logInternalError(Entry::Level severity, Journal& journal) = 0;

    /**
     * Log a phase fault error.
     *
     * This error is logged when a regulator has lost a redundant phase.
     *
     * @param severity severity level
     * @param journal system journal
     * @param type phase fault type
     * @param inventoryPath D-Bus inventory path of the device where the error
     *                      occurred
     * @param additionalData additional error data (if any)
     */
    virtual void logPhaseFault(
        Entry::Level severity, Journal& journal, PhaseFaultType type,
        const std::string& inventoryPath,
        std::map<std::string, std::string> additionalData) = 0;

    /**
     * Log a PMBus error.
     *
     * This error is logged when the I2C communication was successful, but the
     * PMBus value read is invalid or unsupported.
     *
     * @param severity severity level
     * @param journal system journal
     * @param inventoryPath D-Bus inventory path of the device where the error
     *                      occurred
     */
    virtual void logPMBusError(Entry::Level severity, Journal& journal,
                               const std::string& inventoryPath) = 0;

    /**
     * Log a write verification error.
     *
     * This error is logged when a device register is written, read back, and
     * the two values do not match.  This is also called a read-back error.
     *
     * @param severity severity level
     * @param journal system journal
     * @param inventoryPath D-Bus inventory path of the device where the error
     *                      occurred
     */
    virtual void logWriteVerificationError(
        Entry::Level severity, Journal& journal,
        const std::string& inventoryPath) = 0;
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
    explicit DBusErrorLogging(sdbusplus::bus_t& bus) : bus{bus} {}

    /** @copydoc ErrorLogging::logConfigFileError() */
    virtual void logConfigFileError(Entry::Level severity,
                                    Journal& journal) override;

    /** @copydoc ErrorLogging::logDBusError() */
    virtual void logDBusError(Entry::Level severity, Journal& journal) override;

    /** @copydoc ErrorLogging::logI2CError() */
    virtual void logI2CError(Entry::Level severity, Journal& journal,
                             const std::string& bus, uint8_t addr,
                             int errorNumber) override;

    /** @copydoc ErrorLogging::logInternalError() */
    virtual void logInternalError(Entry::Level severity,
                                  Journal& journal) override;

    /** @copydoc ErrorLogging::logPhaseFault() */
    virtual void logPhaseFault(
        Entry::Level severity, Journal& journal, PhaseFaultType type,
        const std::string& inventoryPath,
        std::map<std::string, std::string> additionalData) override;

    /** @copydoc ErrorLogging::logPMBusError() */
    virtual void logPMBusError(Entry::Level severity, Journal& journal,
                               const std::string& inventoryPath) override;

    /** @copydoc ErrorLogging::logWriteVerificationError() */
    virtual void logWriteVerificationError(
        Entry::Level severity, Journal& journal,
        const std::string& inventoryPath) override;

  private:
    /**
     * Create an FFDCFile object containing the specified lines of text data.
     *
     * Throws an exception if an error occurs.
     *
     * @param lines lines of text data to write to file
     * @return FFDCFile object
     */
    FFDCFile createFFDCFile(const std::vector<std::string>& lines);

    /**
     * Create FFDCFile objects containing debug data to store in the error log.
     *
     * If an error occurs, the error is written to the journal but an exception
     * is not thrown.
     *
     * @param journal system journal
     * @return vector of FFDCFile objects
     */
    std::vector<FFDCFile> createFFDCFiles(Journal& journal);

    /**
     * Create FFDCTuple objects corresponding to the specified FFDC files.
     *
     * The D-Bus method to create an error log requires a vector of tuples to
     * pass in the FFDC file information.
     *
     * @param files FFDC files
     * @return vector of FFDCTuple objects
     */
    std::vector<FFDCTuple> createFFDCTuples(std::vector<FFDCFile>& files);

    /**
     * Logs an error using the D-Bus CreateWithFFDCFiles method.
     *
     * If logging fails, a message is written to the journal but an exception is
     * not thrown.
     *
     * @param message Message property of the error log entry
     * @param severity Severity property of the error log entry
     * @param additionalData AdditionalData property of the error log entry
     * @param journal system journal
     */
    void logError(const std::string& message, Entry::Level severity,
                  std::map<std::string, std::string>& additionalData,
                  Journal& journal);

    /**
     * Removes the specified FFDC files from the file system.
     *
     * Also clears the specified vector, removing the FFDCFile objects.
     *
     * If an error occurs, the error is written to the journal but an exception
     * is not thrown.
     *
     * @param files FFDC files to remove
     * @param journal system journal
     */
    void removeFFDCFiles(std::vector<FFDCFile>& files, Journal& journal);

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t& bus;
};

} // namespace phosphor::power::regulators
