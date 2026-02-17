/**
 * Copyright © 2025 IBM Corporation
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

#include "chassis.hpp"
#include "power_interface.hpp"
#include "services.hpp"

#include <stddef.h> // for size_t

#include <chrono>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class System
 *
 * The computer system being controlled and monitored by the BMC.
 *
 * The system contains one or more chassis.
 */
class System
{
  public:
    System() = delete;
    System(const System&) = delete;
    System(System&&) = delete;
    System& operator=(const System&) = delete;
    System& operator=(System&&) = delete;
    ~System() = default;

    /**
     * Constructor.
     *
     * @param chassis Chassis in the system
     */
    explicit System(std::vector<std::unique_ptr<Chassis>> chassis) :
        chassis{std::move(chassis)}
    {}

    /**
     * Returns the chassis in the system.
     *
     * @return chassis
     */
    const std::vector<std::unique_ptr<Chassis>>& getChassis() const
    {
        return chassis;
    }

    /**
     * Initializes system monitoring.
     *
     * This method must be called before any methods that return or check the
     * system status.
     *
     * Normally this method is only called once. However, it can be called
     * multiple times if required, such as for automated testing.
     *
     * @param services System services like hardware presence and the journal
     */
    void initializeMonitoring(Services& services);

    /**
     * Returns the last requested system power state.
     *
     * The initial power state is obtained by the monitor() method. That method
     * must be called before calling getPowerState().
     *
     * Throws an exception if the power state could not be obtained.
     *
     * @return last requested power state
     */
    PowerState getPowerState()
    {
        if (!powerState)
        {
            throw std::runtime_error{
                "System power state could not be obtained"};
        }
        return *powerState;
    }

    /**
     * Sets the requested system power state.
     *
     * Powers the system on or off based on the specified state.
     *
     * Throws an exception if one of the following occurs:
     * - System monitoring has not been initialized
     * - System cannot be set to the specified power state
     * - No chassis can be set to the specified power state
     *
     * @param newPowerState New system power state
     * @param services System services like hardware presence and the journal
     */
    void setPowerState(PowerState newPowerState, Services& services);

    /**
     * Returns the chassis numbers selected for the current power on/off
     * attempt.
     *
     * @return chassis numbers in current power on/off attempt
     */
    const std::set<size_t>& getSelectedChassis()
    {
        return selectedChassis;
    }

    /**
     * Returns the system power good value.
     *
     * The power good value is set by the monitor() method. That method must be
     * called before calling getPowerGood().
     *
     * Throws an exception if the power good value could not be obtained.
     *
     * @return system power good
     */
    PowerGood getPowerGood()
    {
        if (!powerGood)
        {
            throw std::runtime_error{"System power good could not be obtained"};
        }
        return *powerGood;
    }

    /**
     * Returns whether the system is in transition to a new requested power
     * state.
     *
     * A new power state has been requested using setPowerState(), but the power
     * good value does not yet match that state. For example, the power state
     * has been set to on, but the power good value is not yet on.
     *
     * @return true if system is in a power state transition, false otherwise
     */
    bool isInPowerStateTransition()
    {
        return isInStateTransition;
    }

    /**
     * Monitors the status of the system.
     *
     * Sets the system power good value by obtaining the power good value from
     * each chassis selected for the current power on/off attempt.
     *
     * This method must be called once per second to update the power good value
     * and to detect power errors.
     *
     * Throws an exception if system monitoring has not been initialized.
     *
     * @param services System services like hardware presence and the journal
     */
    void monitor(Services& services);

    /**
     * Sets the power good timeout for all chassis.
     *
     * This timeout indicates a power state change has taken too much time and
     * has failed.
     *
     * If a power state change is already occurring, the new value will not be
     * used until the next power state change.
     *
     * @param newTimeout New timeout value
     */
    void setPowerGoodTimeout(std::chrono::milliseconds newTimeout)
    {
        for (auto& curChassis : chassis)
        {
            curChassis->setPowerGoodTimeout(newTimeout);
        }
    }

    /**
     * Sets the power supply error occurring in all chassis, if any.
     *
     * @param error Power supply error or empty string if no error occurring
     */
    void setPowerSupplyError(const std::string& error)
    {
        for (auto& curChassis : chassis)
        {
            curChassis->setPowerSupplyError(error);
        }
    }

    /**
     * Clears the error history for the system.
     */
    void clearErrorHistory()
    {
        hasRequestedDump = false;
        hasRequestedPowerOff = false;
    }

  private:
    /**
     * Verifies that system monitoring has been initialized.
     *
     * Throws an exception if monitoring has not been initialized.
     */
    void verifyMonitoringInitialized()
    {
        if (!isMonitoringInitialized)
        {
            throw std::runtime_error{
                "System monitoring has not been initialized"};
        }
    }

    /**
     * Verifies that the system can be set to the specified power state.
     *
     * Throws an exception if the new power state is not allowed.
     *
     * @param newPowerState New system power state
     */
    void verifyCanSetPowerState(PowerState newPowerState)
    {
        if (powerState && (powerState == newPowerState))
        {
            throw std::runtime_error{std::format(
                "Unable to set system to state {}: Already at requested state",
                PowerInterface::toString(newPowerState))};
        }
    }

    /**
     * Returns the set of chassis numbers that can be set to the specified new
     * power state.
     *
     * Throws an exception if system monitoring has not been initialized.
     *
     * @param newPowerState New system power state
     * @param services System services like hardware presence and the journal
     * @return set of chassis numbers that can be set to new power state
     */
    std::set<size_t> getChassisForNewPowerState(PowerState newPowerState,
                                                Services& services);

    /**
     * Sets the power state of all selected chassis.
     *
     * @param services System services like hardware presence and the journal
     */
    void setChassisPowerState(Services& services);

    /**
     * Monitors the status of all chassis.
     *
     * Does not restrict monitoring to only chassis selected for power on/off.
     * All chassis need to have their monitor method called in order to react to
     * D-Bus status changes.
     *
     * @param services System services like hardware presence and the journal
     */
    void monitorChassisStatus(Services& services);

    /**
     * Defines the initial set of chassis selected for power on/off if needed.
     *
     * This is necessary when the application first starts.
     *
     * We do not know which chassis were selected for power on/off prior to the
     * application starting.
     *
     * Define the selected chassis set based on which chassis are currently
     * powered on and off.
     *
     * The selected chassis will be set explicitly next time the system is
     * powered on or off by setPowerState().
     */
    void setInitialSelectedChassisIfNeeded();

    /**
     * Sets the system power good value based on the selected chassis power good
     * values.
     */
    void setPowerGood();

    /**
     * Returns whether the power good value from the specified chassis should be
     * used in setting the system power good.
     *
     * @param aChassis The chassis to inspect
     */
    bool shouldUseChassisPowerGood(Chassis& aChassis);

    /**
     * Sets the initial power state value if it currently has no value.
     *
     * This is necessary when the application first starts.
     *
     * The initial power state value is based on the current power good value.
     * We assume that the last requested power state matches the power good
     * value. For example, if the system power good is on, then we assume the
     * last requested system power state was on.
     *
     * The power state value will be set explicitly next time the system is
     * powered on or off by setPowerState().
     */
    void setInitialPowerStateIfNeeded();

    /**
     * Updates isInStateTransition based on the current power state and power
     * good values.
     *
     * See isInPowerStateTransition() for more information.
     */
    void updateInPowerStateTransition();

    /**
     * Checks whether power good faults have been detected in any of the
     * selected chassis.
     *
     * There are two types of power good faults:
     * - Timeout power good faults: Occur during power on when a timeout expires
     *   before the chassis power good changes to true. These are handled by the
     *   state manager application, which does a power off/cycle and requests a
     *   BMC dump.
     * - Non-timeout power good faults: Occur after a chassis had been
     *   successfully powered on. The power good unexpectedly changes from true
     *   to false. These are handled by this application, which does a power
     *   off/cycle and requests a BMC dump.
     *
     * The difference in which application handles the power off/cycle and dump
     * request is due to complex service file dependencies during the power on
     * attempt.
     *
     * A non-timeout power good fault can occur while the overall system power
     * on is still occurring (in transition). One chassis might power on
     * successfully and then almost immediately lose power good while other
     * chassis have not yet powered on.
     *
     * A delay is implemented between when a non-timeout power good fault is
     * detected and when an error is logged. The delay allows the power supplies
     * and other hardware time to complete failure processing. As a result, the
     * power off/cycle and dump request is not performed until after the error
     * has been logged.
     *
     * @param services System services like hardware presence and the journal
     */
    void checkForPowerGoodFaults(Services& services);

    /**
     * Checks if any of the selected chassis have an invalid status.
     *
     * When a power on starts, only chassis with a valid status are selected.
     * However, during or after the power on the status of a selected chassis
     * could change.
     *
     * Only checks the chassis status if the system is powering on and still in
     * transition.
     *
     * Does not check the chassis status if the system has successfully powered
     * on (not in transition). We don't want to power off a running system based
     * on chassis status. The status could be incorrect (such as due to a bad
     * GPIO), or the bad status could be transitory.
     *
     * @param services System services like hardware presence and the journal
     */
    void checkForInvalidChassisStatus(Services& services);

    /**
     * Creates a BMC dump.
     *
     * Does nothing if a dump has already been requested.
     *
     * @param services System services like hardware presence and the journal
     */
    void createBMCDump(Services& services);

    /**
     * Performs a hard power off of the system using systemd.
     *
     * The system is powered off without notifying the host and giving it time
     * to shut itself down.
     *
     * If this is a multiple chassis system, after the power off is complete the
     * system is powered back on again. The power cycle systemd target is used
     * rather than the power off target.
     *
     * Does nothing if a hard power off has already been requested.
     *
     * @param services System services like hardware presence and the journal
     */
    void hardPowerOff(Services& services);

    /**
     * Chassis in the system.
     */
    std::vector<std::unique_ptr<Chassis>> chassis{};

    /**
     * Indicates whether system monitoring has been initialized.
     */
    bool isMonitoringInitialized{false};

    /**
     * Last requested system power state.
     */
    std::optional<PowerState> powerState{};

    /**
     * System power good.
     */
    std::optional<PowerGood> powerGood{};

    /**
     * Indicates whether the system is in a power state transition.
     *
     * See isInPowerStateTransition() for more information.
     */
    bool isInStateTransition{false};

    /**
     * Chassis numbers that were selected for the current power on/off attempt.
     */
    std::set<size_t> selectedChassis;

    /**
     * Indicates whether a BMC dump has been requested.
     */
    bool hasRequestedDump{false};

    /**
     * Indicates whether a hard power off has been requested using systemd.
     */
    bool hasRequestedPowerOff{false};
};

} // namespace phosphor::power::sequencer
