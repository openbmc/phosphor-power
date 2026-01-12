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
     * Monitors the status of the system.
     *
     * Sets the system power good value by obtaining the power good value from
     * each chassis selected for the current power on/off attempt.
     *
     * This method must be called periodically (such as once per second) to
     * ensure the system power state and power good values are correct.
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
     * @param newTimeOut New timeout value
     */
    void setPowerGoodTimeOut(std::chrono::milliseconds newTimeOut)
    {
        for (auto& curChassis : chassis)
        {
            curChassis->setPowerGoodTimeOut(newTimeOut);
        }
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
     * Sets the system power good value based on the chassis power good values.
     */
    void setPowerGood();

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
     * Chassis numbers that were selected for the current power on/off attempt.
     */
    std::set<size_t> selectedChassis;
};

} // namespace phosphor::power::sequencer
