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

#include "error_logging.hpp"

#include "exception_utils.hpp"

#include <errno.h>     // for errno
#include <string.h>    // for strerror()
#include <sys/types.h> // for getpid(), lseek(), ssize_t
#include <unistd.h>    // for getpid(), lseek(), write()

#include <sdbusplus/message.hpp>

#include <exception>
#include <ios>
#include <sstream>
#include <stdexcept>

namespace phosphor::power::regulators
{

void DBusErrorLogging::logConfigFileError(Entry::Level severity,
                                          Journal& journal)
{
    std::string message{
        "xyz.openbmc_project.Power.Regulators.Error.ConfigFile"};
    if (severity == Entry::Level::Critical)
    {
        // Specify a different message property for critical config file errors.
        // These are logged when a critical operation cannot be performed due to
        // the lack of a valid config file.  These errors may require special
        // handling, like stopping a power on attempt.
        message =
            "xyz.openbmc_project.Power.Regulators.Error.ConfigFile.Critical";
    }

    std::map<std::string, std::string> additionalData{};
    logError(message, severity, additionalData, journal);
}

void DBusErrorLogging::logDBusError(Entry::Level severity, Journal& journal)
{
    std::map<std::string, std::string> additionalData{};
    logError("xyz.openbmc_project.Power.Error.DBus", severity, additionalData,
             journal);
}

void DBusErrorLogging::logI2CError(Entry::Level severity, Journal& journal,
                                   const std::string& bus, uint8_t addr,
                                   int errorNumber)
{
    // Convert I2C address to a hex string
    std::ostringstream ss;
    ss << "0x" << std::hex << std::uppercase << static_cast<uint16_t>(addr);
    std::string addrStr = ss.str();

    // Convert errno value to an integer string
    std::string errorNumberStr = std::to_string(errorNumber);

    std::map<std::string, std::string> additionalData{};
    additionalData.emplace("CALLOUT_IIC_BUS", bus);
    additionalData.emplace("CALLOUT_IIC_ADDR", addrStr);
    additionalData.emplace("CALLOUT_ERRNO", errorNumberStr);
    logError("xyz.openbmc_project.Power.Error.I2C", severity, additionalData,
             journal);
}

void DBusErrorLogging::logInternalError(Entry::Level severity, Journal& journal)
{
    std::map<std::string, std::string> additionalData{};
    logError("xyz.openbmc_project.Power.Error.Internal", severity,
             additionalData, journal);
}

void DBusErrorLogging::logPhaseFault(
    Entry::Level severity, Journal& journal, PhaseFaultType type,
    const std::string& inventoryPath,
    std::map<std::string, std::string> additionalData)
{
    std::string message =
        (type == PhaseFaultType::n)
            ? "xyz.openbmc_project.Power.Regulators.Error.PhaseFault.N"
            : "xyz.openbmc_project.Power.Regulators.Error.PhaseFault.NPlus1";
    additionalData.emplace("CALLOUT_INVENTORY_PATH", inventoryPath);
    logError(message, severity, additionalData, journal);
}

void DBusErrorLogging::logPMBusError(Entry::Level severity, Journal& journal,
                                     const std::string& inventoryPath)
{
    std::map<std::string, std::string> additionalData{};
    additionalData.emplace("CALLOUT_INVENTORY_PATH", inventoryPath);
    logError("xyz.openbmc_project.Power.Error.PMBus", severity, additionalData,
             journal);
}

void DBusErrorLogging::logWriteVerificationError(
    Entry::Level severity, Journal& journal, const std::string& inventoryPath)
{
    std::map<std::string, std::string> additionalData{};
    additionalData.emplace("CALLOUT_INVENTORY_PATH", inventoryPath);
    logError("xyz.openbmc_project.Power.Regulators.Error.WriteVerification",
             severity, additionalData, journal);
}

FFDCFile DBusErrorLogging::createFFDCFile(const std::vector<std::string>& lines)
{
    // Create FFDC file of type Text
    FFDCFile file{FFDCFormat::Text};
    int fd = file.getFileDescriptor();

    // Write lines to file
    std::string buffer;
    for (const std::string& line : lines)
    {
        // Copy line to buffer.  Add newline if necessary.
        buffer = line;
        if (line.empty() || (line.back() != '\n'))
        {
            buffer += '\n';
        }

        // Write buffer to file
        const char* bufPtr = buffer.c_str();
        unsigned int count = buffer.size();
        while (count > 0)
        {
            // Try to write remaining bytes; it might not write all of them
            ssize_t bytesWritten = write(fd, bufPtr, count);
            if (bytesWritten == -1)
            {
                throw std::runtime_error{
                    std::string{"Unable to write to FFDC file: "} +
                    strerror(errno)};
            }
            bufPtr += bytesWritten;
            count -= bytesWritten;
        }
    }

    // Seek to beginning of file so error logging system can read data
    if (lseek(fd, 0, SEEK_SET) != 0)
    {
        throw std::runtime_error{
            std::string{"Unable to seek within FFDC file: "} + strerror(errno)};
    }

    return file;
}

std::vector<FFDCFile> DBusErrorLogging::createFFDCFiles(Journal& journal)
{
    std::vector<FFDCFile> files{};

    // Create FFDC files containing journal messages from relevant executables.
    // Executables in priority order in case error log cannot hold all the FFDC.
    std::vector<std::string> executables{"phosphor-regulators", "systemd"};
    for (const std::string& executable : executables)
    {
        try
        {
            // Get recent journal messages from the executable
            std::vector<std::string> messages =
                journal.getMessages("SYSLOG_IDENTIFIER", executable, 30);

            // Create FFDC file containing the journal messages
            if (!messages.empty())
            {
                files.emplace_back(createFFDCFile(messages));
            }
        }
        catch (const std::exception& e)
        {
            journal.logError(exception_utils::getMessages(e));
        }
    }

    return files;
}

std::vector<FFDCTuple> DBusErrorLogging::createFFDCTuples(
    std::vector<FFDCFile>& files)
{
    std::vector<FFDCTuple> ffdcTuples{};
    for (FFDCFile& file : files)
    {
        ffdcTuples.emplace_back(
            file.getFormat(), file.getSubType(), file.getVersion(),
            sdbusplus::message::unix_fd(file.getFileDescriptor()));
    }
    return ffdcTuples;
}

void DBusErrorLogging::logError(
    const std::string& message, Entry::Level severity,
    std::map<std::string, std::string>& additionalData, Journal& journal)
{
    try
    {
        // Add PID to AdditionalData
        additionalData.emplace("_PID", std::to_string(getpid()));

        // Create FFDC files containing debug data to store in error log
        std::vector<FFDCFile> files{createFFDCFiles(journal)};

        // Create FFDC tuples used to pass FFDC files to D-Bus method
        std::vector<FFDCTuple> ffdcTuples{createFFDCTuples(files)};

        // Call D-Bus method to create an error log with FFDC files
        const char* service = "xyz.openbmc_project.Logging";
        const char* objPath = "/xyz/openbmc_project/logging";
        const char* interface = "xyz.openbmc_project.Logging.Create";
        const char* method = "CreateWithFFDCFiles";
        auto reqMsg = bus.new_method_call(service, objPath, interface, method);
        reqMsg.append(message, severity, additionalData, ffdcTuples);
        auto respMsg = bus.call(reqMsg);

        // Remove FFDC files.  If an exception occurs before this, the files
        // will be deleted by FFDCFile desctructor but errors will be ignored.
        removeFFDCFiles(files, journal);
    }
    catch (const std::exception& e)
    {
        journal.logError(exception_utils::getMessages(e));
        journal.logError("Unable to log error " + message);
    }
}

void DBusErrorLogging::removeFFDCFiles(std::vector<FFDCFile>& files,
                                       Journal& journal)
{
    // Explicitly remove FFDC files rather than relying on FFDCFile destructor.
    // This allows any resulting errors to be written to the journal.
    for (FFDCFile& file : files)
    {
        try
        {
            file.remove();
        }
        catch (const std::exception& e)
        {
            journal.logError(exception_utils::getMessages(e));
        }
    }

    // Clear vector since the FFDCFile objects can no longer be used
    files.clear();
}

} // namespace phosphor::power::regulators
