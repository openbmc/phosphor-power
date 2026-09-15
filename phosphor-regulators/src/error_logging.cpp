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

#include <sys/types.h> // for getpid()
#include <unistd.h>    // for getpid()

#include <exception>
#include <sstream>

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

void DBusErrorLogging::logError(
    const std::string& message, Entry::Level severity,
    std::map<std::string, std::string>& additionalData, Journal& journal)
{
    try
    {
        // Add PID to AdditionalData
        additionalData.emplace("_PID", std::to_string(getpid()));

        // Call D-Bus method to create an error log
        auto method = bus.new_method_call(
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Logging.Create", "Create");
        method.append(message, severity, additionalData);
        bus.call_noreply(method);
    }
    catch (const std::exception& e)
    {
        journal.logError(exception_utils::getMessages(e));
        journal.logError("Unable to log error " + message);
    }
}

} // namespace phosphor::power::regulators
