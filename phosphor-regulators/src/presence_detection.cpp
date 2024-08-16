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

#include "presence_detection.hpp"

#include "action_environment.hpp"
#include "action_utils.hpp"
#include "chassis.hpp"
#include "device.hpp"
#include "error_logging_utils.hpp"
#include "exception_utils.hpp"
#include "system.hpp"

#include <exception>

namespace phosphor::power::regulators
{

bool PresenceDetection::execute(Services& services, System& system,
                                Chassis& /*chassis*/, Device& device)
{
    // If no presence value is cached
    if (!isPresent.has_value())
    {
        // Initially assume device is present
        isPresent = true;

        // Execute actions to find device presence
        try
        {
            // Create ActionEnvironment
            ActionEnvironment environment{system.getIDMap(), device.getID(),
                                          services};

            // Execute the actions and cache resulting value
            isPresent = action_utils::execute(actions, environment);
        }
        catch (const std::exception& e)
        {
            // Log error messages in journal
            services.getJournal().logError(exception_utils::getMessages(e));
            services.getJournal().logError(
                "Unable to determine presence of " + device.getID());

            // Create error log entry
            error_logging_utils::logError(std::current_exception(),
                                          Entry::Level::Warning, services);
        }
    }

    return isPresent.value();
}

} // namespace phosphor::power::regulators
