/**
 * Copyright © 2021 IBM Corporation
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

#include "phase_fault_detection.hpp"

#include "action_utils.hpp"
#include "chassis.hpp"
#include "device.hpp"
#include "error_logging.hpp"
#include "error_logging_utils.hpp"
#include "exception_utils.hpp"
#include "journal.hpp"
#include "system.hpp"

#include <exception>
#include <map>

namespace phosphor::power::regulators
{

/**
 * Maximum number of action errors to write to the journal.
 */
constexpr unsigned short maxActionErrorCount{3};

/**
 * Number of consecutive phase faults required to log an error.  This provides
 * "de-glitching" to ignore transient hardware problems.
 */
constexpr unsigned short requiredConsecutiveFaults{2};

void PhaseFaultDetection::execute(Services& services, System& system,
                                  Chassis& /*chassis*/, Device& regulator)
{
    try
    {
        // Find the device ID to use.  If the deviceID data member is empty, use
        // the ID of the specified regulator.
        const std::string& effectiveDeviceID =
            deviceID.empty() ? regulator.getID() : deviceID;

        // Create ActionEnvironment
        ActionEnvironment environment{system.getIDMap(), effectiveDeviceID,
                                      services};

        // Execute the actions to detect phase faults
        action_utils::execute(actions, environment);

        // Check for any N or N+1 phase faults that were detected
        checkForPhaseFault(PhaseFaultType::n, services, regulator, environment);
        checkForPhaseFault(PhaseFaultType::n_plus_1, services, regulator,
                           environment);
    }
    catch (const std::exception& e)
    {
        // Log error messages in journal if we haven't hit the max
        if (actionErrorCount < maxActionErrorCount)
        {
            ++actionErrorCount;
            services.getJournal().logError(exception_utils::getMessages(e));
            services.getJournal().logError(
                "Unable to detect phase faults in regulator " +
                regulator.getID());
        }

        // Create error log entry if this type hasn't already been logged
        error_logging_utils::logError(std::current_exception(),
                                      Entry::Level::Warning, services,
                                      errorHistory);
    }
}

void PhaseFaultDetection::checkForPhaseFault(
    PhaseFaultType faultType, Services& services, Device& regulator,
    ActionEnvironment& environment)
{
    // Find ErrorType that corresponds to PhaseFaultType; used by ErrorHistory
    ErrorType errorType = toErrorType(faultType);

    // If this error has not been logged yet
    if (!errorHistory.wasLogged(errorType))
    {
        // Create reference to consecutive fault count data member
        unsigned short& faultCount =
            (faultType == PhaseFaultType::n) ? nFaultCount : nPlus1FaultCount;

        // Check if the phase fault was detected
        if (environment.getPhaseFaults().count(faultType) == 0)
        {
            // Phase fault not detected; reset consecutive fault count
            faultCount = 0;
        }
        else
        {
            // Phase fault detected; increment consecutive fault count
            ++faultCount;

            // Log error message in journal
            services.getJournal().logError(
                toString(faultType) + " phase fault detected in regulator " +
                regulator.getID() + ": count=" + std::to_string(faultCount));

            // If the required number of consecutive faults have been detected
            if (faultCount >= requiredConsecutiveFaults)
            {
                // Log phase fault error and update ErrorHistory
                logPhaseFault(faultType, services, regulator, environment);
                errorHistory.setWasLogged(errorType, true);
            }
        }
    }
}

void PhaseFaultDetection::logPhaseFault(PhaseFaultType faultType,
                                        Services& services, Device& regulator,
                                        ActionEnvironment& environment)
{
    ErrorLogging& errorLogging = services.getErrorLogging();
    Entry::Level severity = (faultType == PhaseFaultType::n)
                                ? Entry::Level::Warning
                                : Entry::Level::Informational;
    Journal& journal = services.getJournal();
    const std::string& inventoryPath = regulator.getFRU();
    const std::map<std::string, std::string>& additionalData =
        environment.getAdditionalErrorData();
    errorLogging.logPhaseFault(severity, journal, faultType, inventoryPath,
                               additionalData);
}

} // namespace phosphor::power::regulators
