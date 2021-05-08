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

#include "sensor_monitoring.hpp"

#include "action_environment.hpp"
#include "action_utils.hpp"
#include "chassis.hpp"
#include "device.hpp"
#include "error_logging_utils.hpp"
#include "exception_utils.hpp"
#include "rail.hpp"
#include "sensors.hpp"
#include "system.hpp"

#include <exception>

namespace phosphor::power::regulators
{

void SensorMonitoring::execute(Services& services, System& system,
                               Chassis& chassis, Device& device, Rail& rail)
{
    // Notify sensors service that monitoring is starting for this rail
    Sensors& sensors = services.getSensors();
    sensors.startRail(rail.getID(), device.getFRU(),
                      chassis.getInventoryPath());

    // Read all sensors defined for this rail
    bool errorOccurred{false};
    try
    {
        // Create ActionEnvironment
        ActionEnvironment environment{system.getIDMap(), device.getID(),
                                      services};

        // Execute the actions
        action_utils::execute(actions, environment);
    }
    catch (const std::exception& e)
    {
        // Set flag to notify sensors service that an error occurred
        errorOccurred = true;

        // Log error messages in journal for the first 3 errors
        if (++errorCount <= 3)
        {
            services.getJournal().logError(exception_utils::getMessages(e));
            services.getJournal().logError(
                "Unable to monitor sensors for rail " + rail.getID());
        }

        // Create error log entry if this type hasn't already been logged
        error_logging_utils::logError(std::current_exception(),
                                      Entry::Level::Warning, services,
                                      errorHistory);
    }

    // Notify sensors service that monitoring has ended for this rail
    sensors.endRail(errorOccurred);
}

} // namespace phosphor::power::regulators
