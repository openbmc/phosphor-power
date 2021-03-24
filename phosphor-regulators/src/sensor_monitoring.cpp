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
#include "system.hpp"

#include <exception>

namespace phosphor::power::regulators
{

void SensorMonitoring::execute(Services& services, System& system,
                               Chassis& /*chassis*/, Device& device, Rail& rail)
{
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
        // Log error messages in journal
        services.getJournal().logError(exception_utils::getMessages(e));
        services.getJournal().logError("Unable to monitor sensors for rail " +
                                       rail.getID());

        // Create error log entry
        // TODO: Add ErrorHistory data member and specify as parameter below
        error_logging_utils::logError(std::current_exception(),
                                      Entry::Level::Warning, services);
    }
}

} // namespace phosphor::power::regulators
