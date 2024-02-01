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

#include "services.hpp"

#include "types.hpp"
#include "utility.hpp"

#include <sys/types.h> // for getpid()
#include <unistd.h>    // for getpid()

#include <gpiod.hpp>

#include <exception>

namespace phosphor::power::sequencer
{

void BMCServices::logError(const std::string& message, Entry::Level severity,
                           std::map<std::string, std::string>& additionalData)
{
    try
    {
        // Add PID to AdditionalData
        additionalData.emplace("_PID", std::to_string(getpid()));

        // If severity is critical, set error as system terminating
        if (severity == Entry::Level::Critical)
        {
            additionalData.emplace("SEVERITY_DETAIL", "SYSTEM_TERM");
        }

        auto method = bus.new_method_call(
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Logging.Create", "Create");
        method.append(message, severity, additionalData);
        bus.call_noreply(method);
    }
    catch (const std::exception& e)
    {
        lg2::error("Unable to log error {ERROR}: {EXCEPTION}", "ERROR", message,
                   "EXCEPTION", e);
    }
}

bool BMCServices::isPresent(const std::string& inventoryPath)
{
    // Initially assume hardware is not present
    bool present{false};

    // Get presence from D-Bus interface/property
    try
    {
        util::getProperty(INVENTORY_IFACE, PRESENT_PROP, inventoryPath,
                          INVENTORY_MGR_IFACE, bus, present);
    }
    catch (const sdbusplus::exception_t& e)
    {
        // If exception type is expected and indicates hardware not present
        if (isExpectedException(e))
        {
            present = false;
        }
        else
        {
            // Re-throw unexpected exception
            throw;
        }
    }

    return present;
}

std::vector<int> BMCServices::getGPIOValues(const std::string& chipLabel)
{
    // Set up the chip object
    gpiod::chip chip{chipLabel, gpiod::chip::OPEN_BY_LABEL};
    unsigned int numLines = chip.num_lines();
    lg2::info(
        "Reading GPIO values from chip {NAME} with label {LABEL} and {NUM_LINES} lines",
        "NAME", chip.name(), "LABEL", chipLabel, "NUM_LINES", numLines);

    // Read GPIO values.  Work around libgpiod bulk line maximum by getting
    // values from individual lines.
    std::vector<int> values;
    for (unsigned int offset = 0; offset < numLines; ++offset)
    {
        gpiod::line line = chip.get_line(offset);
        line.request({"phosphor-power-control",
                      gpiod::line_request::DIRECTION_INPUT, 0});
        values.push_back(line.get_value());
        line.release();
    }
    return values;
}

bool BMCServices::isExpectedException(const sdbusplus::exception_t& e)
{
    // Initially assume exception is not one of the expected types
    bool isExpected{false};

    // If the D-Bus error name is set within the exception
    if (e.name() != nullptr)
    {
        // Check if the error name is one of the expected values when hardware
        // is not present.
        //
        // Sometimes the object path does not exist.  Sometimes the object path
        // exists, but it does not implement the D-Bus interface that contains
        // the present property.  Both of these cases result in exceptions.
        //
        // In the case where the interface is not implemented, the systemd
        // documentation seems to indicate that the error name should be
        // SD_BUS_ERROR_UNKNOWN_INTERFACE.  However, in OpenBMC the
        // SD_BUS_ERROR_UNKNOWN_PROPERTY error name can occur.
        std::string name = e.name();
        if ((name == SD_BUS_ERROR_UNKNOWN_OBJECT) ||
            (name == SD_BUS_ERROR_UNKNOWN_INTERFACE) ||
            (name == SD_BUS_ERROR_UNKNOWN_PROPERTY))
        {
            isExpected = true;
        }
    }

    return isExpected;
}

} // namespace phosphor::power::sequencer
