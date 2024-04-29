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
#pragma once

#include "action.hpp"
#include "action_environment.hpp"
#include "error_history.hpp"
#include "phase_fault.hpp"
#include "services.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

// Forward declarations to avoid circular dependencies
class Chassis;
class Device;
class System;

/**
 * @class PhaseFaultDetection
 *
 * Detects and logs redundant phase faults in a voltage regulator.
 *
 * A voltage regulator is sometimes called a "phase controller" because it
 * controls one or more phases that perform the actual voltage regulation.
 *
 * A regulator may have redundant phases.  If a redundant phase fails, the
 * regulator will continue to provide the desired output voltage.  However, a
 * phase fault error should be logged warning the user that the regulator has
 * lost redundancy.
 *
 * The technique used to detect a phase fault varies depending on the regulator
 * hardware.  Often a bit is checked in a status register.  The status register
 * could exist in the regulator or in a related I/O expander.
 *
 * Phase fault detection is executed repeatedly based on a timer.  A phase fault
 * must be detected two consecutive times before an error is logged.  This
 * provides "de-glitching" to ignore transient hardware problems.
 *
 * Phase faults are detected by executing actions.
 */
class PhaseFaultDetection
{
  public:
    // Specify which compiler-generated methods we want
    PhaseFaultDetection() = delete;
    PhaseFaultDetection(const PhaseFaultDetection&) = delete;
    PhaseFaultDetection(PhaseFaultDetection&&) = delete;
    PhaseFaultDetection& operator=(const PhaseFaultDetection&) = delete;
    PhaseFaultDetection& operator=(PhaseFaultDetection&&) = delete;
    ~PhaseFaultDetection() = default;

    /**
     * Constructor.
     *
     * @param actions Actions that detect phase faults in the regulator.
     * @param deviceID Unique ID of the device to use when detecting phase
     *                 faults.  If not specified, the regulator will be used.
     */
    explicit PhaseFaultDetection(std::vector<std::unique_ptr<Action>> actions,
                                 const std::string& deviceID = "") :
        actions{std::move(actions)}, deviceID{deviceID}
    {}

    /**
     * Clears all error history.
     *
     * All data on previously logged errors will be deleted.  If errors occur
     * again in the future they will be logged again.
     *
     * This method is normally called when the system is being powered on.
     */
    void clearErrorHistory()
    {
        errorHistory.clear();
        actionErrorCount = 0;
        nFaultCount = 0;
        nPlus1FaultCount = 0;
    }

    /**
     * Executes the actions that detect phase faults in the regulator.
     *
     * If the required number of consecutive phase faults are detected, an error
     * is logged.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains the chassis
     * @param chassis chassis that contains the regulator device
     * @param regulator voltage regulator device
     */
    void execute(Services& services, System& system, Chassis& chassis,
                 Device& regulator);

    /**
     * Returns the actions that detect phase faults in the regulator.
     *
     * @return actions
     */
    const std::vector<std::unique_ptr<Action>>& getActions() const
    {
        return actions;
    }

    /**
     * Returns the unique ID of the device to use when detecting phase
     * faults.
     *
     * If the value is "", the regulator will be used.
     *
     * @return device ID
     */
    const std::string& getDeviceID() const
    {
        return deviceID;
    }

  private:
    /**
     * Checks if the specified phase fault type was detected.
     *
     * If the fault type was detected, increments the counter tracking
     * consecutive faults.  If the required number of consecutive faults have
     * been detected, logs a phase fault error.
     *
     * The ActionEnvironment contains the set of phase fault types that were
     * detected (if any).
     *
     * @param faultType phase fault type to check
     * @param services system services like error logging and the journal
     * @param regulator voltage regulator device
     * @param environment action execution environment
     */
    void checkForPhaseFault(PhaseFaultType faultType, Services& services,
                            Device& regulator, ActionEnvironment& environment);

    /**
     * Logs an error for the specified phase fault type.
     *
     * @param faultType phase fault type that occurred
     * @param services system services like error logging and the journal
     * @param regulator voltage regulator device
     * @param environment action execution environment
     */
    void logPhaseFault(PhaseFaultType faultType, Services& services,
                       Device& regulator, ActionEnvironment& environment);

    /**
     * Actions that detect phase faults in the regulator.
     */
    std::vector<std::unique_ptr<Action>> actions{};

    /**
     * Unique ID of the device to use when detecting phase faults.
     *
     * Sometimes a separate device, such as an I/O expander, is accessed to
     * obtain the phase fault status for a regulator.
     *
     * If the value is "", the regulator will be used.
     */
    const std::string deviceID{};

    /**
     * History of which error types have been logged.
     *
     * Since phase fault detection runs repeatedly based on a timer, each error
     * type is only logged once.
     */
    ErrorHistory errorHistory{};

    /**
     * Number of errors that have occurred while executing actions, resulting in
     * an exception.
     */
    unsigned short actionErrorCount{0};

    /**
     * Number of consecutive N phase faults that have been detected.
     */
    unsigned short nFaultCount{0};

    /**
     * Number of consecutive N+1 phase faults that have been detected.
     */
    unsigned short nPlus1FaultCount{0};
};

} // namespace phosphor::power::regulators
