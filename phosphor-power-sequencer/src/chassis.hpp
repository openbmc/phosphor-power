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

#include "config.h"

#include "chassis_status_monitor.hpp"
#include "power_interface.hpp"
#include "power_sequencer_device.hpp"
#include "services.hpp"

#include <stddef.h> // for size_t

#include <chrono>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

using ChassisStatusMonitorOptions =
    phosphor::power::util::ChassisStatusMonitorOptions;
using PowerState = PowerInterface::PowerState;
using PowerGood = PowerInterface::PowerGood;

/**
 * @struct PowerGoodFault
 *
 * Power good fault that was detected in the chassis for the current power on
 * attempt.
 */
struct PowerGoodFault
{
    /**
     * Specifies whether the fault was due to a timeout during power on attempt.
     */
    bool wasTimeout{false};

    /**
     * Specifies whether an error has been logged for the fault.
     *
     * For some faults, an error is not logged until a delay time has elapsed.
     *
     * The chassis should not be powered off until an error has been logged.
     */
    bool wasLogged{false};

    /**
     * Specifies the time when an error should be logged.
     *
     * Only used when an error is not logged until a delay time has elapsed.
     */
    std::chrono::time_point<std::chrono::steady_clock> logTime;
};

/**
 * @class Chassis
 *
 * A chassis within the system.
 *
 * Chassis are typically a physical enclosure that contains system components
 * such as CPUs, fans, power supplies, and PCIe cards. A chassis can be
 * stand-alone, such as a tower or desktop. A chassis can also be designed to be
 * mounted in an equipment rack.
 */
class Chassis
{
  public:
    Chassis() = delete;
    Chassis(const Chassis&) = delete;
    Chassis(Chassis&&) = delete;
    Chassis& operator=(const Chassis&) = delete;
    Chassis& operator=(Chassis&&) = delete;
    ~Chassis() = default;

    /**
     * Constructor.
     *
     * @param number Chassis number within the system. Must be >= 1.
     * @param inventoryPath D-Bus inventory path of the chassis
     * @param powerSequencers Power sequencer devices within the chassis
     * @param monitorOptions Types of chassis status monitoring to perform.
     */
    explicit Chassis(
        size_t number, const std::string& inventoryPath,
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers,
        const ChassisStatusMonitorOptions& monitorOptions) :
        number{number}, inventoryPath{inventoryPath},
        powerSequencers{std::move(powerSequencers)},
        monitorOptions{monitorOptions}
    {
        // Disable monitoring for D-Bus properties owned by this application
        this->monitorOptions.isPowerStateMonitored = false;
        this->monitorOptions.isPowerGoodMonitored = false;
    }

    /**
     * Returns the chassis number within the system.
     *
     * @return chassis number
     */
    size_t getNumber() const
    {
        return number;
    }

    /**
     * Returns the D-Bus inventory path of the chassis.
     *
     * @return inventory path
     */
    const std::string& getInventoryPath() const
    {
        return inventoryPath;
    }

    /**
     * Returns the power sequencer devices within the chassis.
     *
     * @return power sequencer devices
     */
    const std::vector<std::unique_ptr<PowerSequencerDevice>>&
        getPowerSequencers() const
    {
        return powerSequencers;
    }

    /**
     * Returns the types of chassis status monitoring to perform.
     *
     * @return chassis status monitoring options
     */
    const ChassisStatusMonitorOptions& getMonitorOptions() const
    {
        return monitorOptions;
    }

    /**
     * Initializes chassis monitoring.
     *
     * Creates a ChassisStatusMonitor object based on the monitoring options
     * specified in the constructor.
     *
     * This method must be called before any methods that return or check the
     * chassis status.
     *
     * Normally this method is only called once. However, it can be called
     * multiple times if required, such as for automated testing.
     *
     * @param services System services like hardware presence and the journal
     */
    void initializeMonitoring(Services& services)
    {
        // Note: replaces/deletes any previous monitor object
        statusMonitor = services.createChassisStatusMonitor(
            number, inventoryPath, monitorOptions);
    }

    /**
     * Returns the ChassisStatusMonitor object that is monitoring D-Bus
     * properties for the chassis.
     *
     * Throws an exception if chassis monitoring has not been initialized.
     *
     * @return reference to ChassisStatusMonitor object
     */
    ChassisStatusMonitor& getStatusMonitor()
    {
        verifyMonitoringInitialized();
        return *statusMonitor;
    }

    /**
     * Returns whether the chassis is present.
     *
     * Throws an exception if:
     * - Chassis monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true is chassis is present, false otherwise
     */
    bool isPresent()
    {
        verifyMonitoringInitialized();
        return statusMonitor->isPresent();
    }

    /**
     * Returns whether the chassis is available.
     *
     * If the D-Bus Available property is false, it means that communication to
     * the chassis is not possible. For example, the chassis does not have any
     * input power or communication cables to the BMC are disconnected.
     *
     * Throws an exception if:
     * - Chassis monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true is chassis is available, false otherwise
     */
    bool isAvailable()
    {
        verifyMonitoringInitialized();
        return statusMonitor->isAvailable();
    }

    /**
     * Returns whether the chassis is enabled.
     *
     * If the D-Bus Enabled property is false, it means that the chassis has
     * been put in hardware isolation (guarded).
     *
     * Throws an exception if:
     * - Chassis monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true is chassis is enabled, false otherwise
     */
    bool isEnabled()
    {
        verifyMonitoringInitialized();
        return statusMonitor->isEnabled();
    }

    /**
     * Returns whether the chassis input power status is good.
     *
     * Throws an exception if:
     * - Chassis monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true if chassis input power is good, false otherwise
     */
    bool isInputPowerGood()
    {
        verifyMonitoringInitialized();
        return statusMonitor->isInputPowerGood();
    }

    /**
     * Returns whether the power supplies power status is good.
     *
     * Throws an exception if:
     * - Chassis monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true if power supplies power is good, false otherwise
     */
    bool isPowerSuppliesPowerGood()
    {
        verifyMonitoringInitialized();
        return statusMonitor->isPowerSuppliesPowerGood();
    }

    /**
     * Returns the last requested chassis power state.
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
            throw std::runtime_error{std::format(
                "Power state could not be obtained for chassis {}", number)};
        }
        return *powerState;
    }

    /**
     * Returns whether the chassis can be set to the specified power state.
     *
     * Determined based on the current chassis status. For example, the chassis
     * cannot be powered on if it is not present.
     *
     * Throws an exception if chassis monitoring has not been initialized.
     *
     * @param newPowerState New chassis power state
     * @return If the state can be set, returns true and an empty string. If the
     *         state cannot be set, returns false and a string containing the
     *         reason.
     */
    std::tuple<bool, std::string> canSetPowerState(PowerState newPowerState);

    /**
     * Sets the requested chassis power state.
     *
     * Powers the chassis on or off based on the specified state.
     *
     * Throws an exception if one of the following occurs:
     * - Chassis monitoring has not been initialized.
     * - Chassis D-Bus status cannot be obtained.
     * - State change is not possible based on the chassis status.
     * - Error occurs powering on the power sequencer devices.
     *
     * @param newPowerState New chassis power state
     * @param services System services like hardware presence and the journal
     */
    void setPowerState(PowerState newPowerState, Services& services);

    /**
     * Returns the chassis power good value.
     *
     * The power good value is read by the monitor() method. That method must be
     * called before calling getPowerGood().
     *
     * Throws an exception if the power good value could not be obtained.
     *
     * @return chassis power good
     */
    PowerGood getPowerGood()
    {
        if (!powerGood)
        {
            throw std::runtime_error{std::format(
                "Power good could not be obtained for chassis {}", number)};
        }
        return *powerGood;
    }

    /**
     * Returns whether the chassis is in transition to a new requested power
     * state.
     *
     * A new power state has been requested using setPowerState(), but the power
     * good value does not yet match that state. For example, the power state
     * has been set to on, but the power good value is not yet on.
     *
     * @return true if chassis is in a power state transition, false otherwise
     */
    bool isInPowerStateTransition()
    {
        return isInStateTransition;
    }

    /**
     * Monitors the status of the chassis.
     *
     * Sets the chassis power good value by reading the power good value from
     * each power sequencer device.
     *
     * Reacts to any changes to chassis D-Bus properties.
     *
     * This method must be called once per second to update the power good value
     * and to detect power errors.
     *
     * Throws an exception if one of the following occurs:
     * - Chassis monitoring has not been initialized.
     * - Chassis D-Bus status cannot be obtained.
     *
     * @param services System services like hardware presence and the journal
     */
    void monitor(Services& services);

    /**
     * Returns whether a power good fault has been detected.
     *
     * A power good fault occurs in the following situations:
     * - A power on attempt times out and is unsuccessful.
     * - The chassis is successfully powered on, but later the power good value
     *   changes to off unexpectedly.
     *
     * Power good fault history is cleared when a new power on attempt occurs.
     *
     * @return true if a power good fault was found, false otherwise
     */
    bool hasPowerGoodFault()
    {
        return powerGoodFault.has_value();
    }

    /**
     * Returns the power good fault that was detected.
     *
     * See hasPowerGoodFault() for more information on power good faults.
     *
     * Throws an exception if no power good fault was detected.
     *
     * @return power good fault
     */
    const PowerGoodFault& getPowerGoodFault()
    {
        if (!powerGoodFault)
        {
            throw std::runtime_error{std::format(
                "No power good fault detected in chassis {}", number)};
        }
        return *powerGoodFault;
    }

    /**
     * Closes all power sequencer devices that are open.
     *
     * Does not throw exceptions. This method may be called because a chassis is
     * no longer present or no longer has input power. In those scenarios
     * closing the device may fail. However, closing the devices is still
     * necessary in order to clean up resources like file handles.
     */
    void closeDevices();

    /**
     * Clears the error history for the chassis.
     */
    void clearErrorHistory()
    {
        powerSupplyError.clear();
        powerGoodFault.reset();
    }

    /**
     * Returns the power good timeout.
     *
     * This timeout indicates a power state change has taken too much time and
     * has failed.
     *
     * @return timeout in milliseconds
     */
    std::chrono::milliseconds getPowerGoodTimeout() const
    {
        return powerGoodTimeout;
    }

    /**
     * Sets the power good timeout.
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
        powerGoodTimeout = newTimeout;
    }

    /**
     * Returns the delay time between detecting a power good fault and logging
     * an error.
     *
     * Error logging is delayed to allow the power supplies and other hardware
     * time to complete failure processing.
     *
     * Error logging is not delayed if the power good fault was due to a
     * timeout.
     *
     * @return delay time in milliseconds
     */
    std::chrono::milliseconds getPowerGoodFaultLogDelay()
    {
        return powerGoodFaultLogDelay;
    }

    /**
     * Sets the delay time between detecting a power good fault and logging an
     * error.
     *
     * See getPowerGoodFaultLogDelay() for more information.
     *
     * @param delay Delay time in milliseconds
     */
    void setPowerGoodFaultLogDelay(std::chrono::milliseconds delay)
    {
        powerGoodFaultLogDelay = delay;
    }

    /**
     * Returns the power supply error occurring within this chassis, if any.
     *
     * @return power supply error or empty string if no error occurring
     */
    const std::string& getPowerSupplyError()
    {
        return powerSupplyError;
    }

    /**
     * Sets the power supply error occurring within this chassis, if any.
     *
     * @param error Power supply error or empty string if no error occurring
     */
    void setPowerSupplyError(const std::string& error)
    {
        powerSupplyError = error;
    }

  private:
    /**
     * Verifies that chassis monitoring has been initialized and a
     * ChassisStatusMonitor object has been created.
     *
     * Throws an exception if monitoring has not been initialized.
     */
    void verifyMonitoringInitialized()
    {
        if (!statusMonitor)
        {
            throw std::runtime_error{std::format(
                "Monitoring not initialized for chassis {}", number)};
        }
    }

    /**
     * Returns the current system time.
     *
     * @return current time
     */
    std::chrono::time_point<std::chrono::steady_clock> getCurrentTime()
    {
        return std::chrono::steady_clock::now();
    }

    /**
     * Opens the specified power sequencer device if it is not already open.
     *
     * Throws an exception if an error occurs.
     *
     * @param device power sequencer device
     * @param services System services like hardware presence and the journal
     */
    void openDeviceIfNeeded(PowerSequencerDevice& device, Services& services)
    {
        if (!device.isOpen())
        {
            device.open(services);
        }
    }

    /**
     * Updates the power good value.
     *
     * If the chassis status is valid, the power good value is read.
     *
     * If the chassis is not present or does not have input power, the power
     * state and power good are set to off and all power sequencer devices are
     * closed.
     *
     * @param services System services like hardware presence and the journal
     */
    void updatePowerGood(Services& services);

    /**
     * Reads the power good value from all power sequencer devices.
     *
     * Determines the combined power good value for the entire chassis.
     *
     * @param services System services like hardware presence and the journal
     */
    void readPowerGood(Services& services);

    /**
     * Sets the initial power state value if it currently has no value.
     *
     * This is necessary when the application first starts or when a previously
     * unavailable chassis becomes available.
     *
     * The initial power state value is based on the current power good value.
     * We assume that the last requested power state matches the power good
     * value. For example, if the chassis power good is on, then we assume the
     * last requested chassis power state was on.
     *
     * The power state value will be set explicitly next time the chassis is
     * powered on or off by setPowerState().
     */
    void setInitialPowerStateIfNeeded();

    /**
     * Powers on all the power sequencer devices in the chassis.
     *
     * Throws an exception if an error occurs.
     *
     * @param services System services like hardware presence and the journal
     */
    void powerOn(Services& services);

    /**
     * Powers off all the power sequencer devices in the chassis.
     *
     * Throws an exception if an error occurs.
     *
     * @param services System services like hardware presence and the journal
     */
    void powerOff(Services& services);

    /**
     * Updates isInStateTransition based on the current power state and power
     * good values.
     *
     * See isInPowerStateTransition() for more information.
     */
    void updateInPowerStateTransition();

    /**
     * Checks whether a power good error has occurred.
     *
     * Checks for the following:
     * - Timeout has occurred during a power on attempt
     * - Timeout has occurred during a power off attempt
     * - Power on attempt worked, but power good suddenly changed to off
     *
     * @param services System services like hardware presence and the journal
     */
    void checkForPowerGoodError(Services& services);

    /**
     * Handles a timeout waiting for the power good value to change during a
     * power on or power off attempt.
     *
     * This occurs when it takes too long for a power on/off attempt to succeed.
     *
     * Logs an error and sets isInStateTransition to false.
     *
     * @param services System services like hardware presence and the journal
     */
    void handlePowerGoodTimeout(Services& services);

    /**
     * Handles a power good fault after the chassis had been powered on.
     *
     * Creates a PowerGoodFault object but does not log an error until a delay
     * time has elapsed.
     *
     * See hasPowerGoodFault() and getPowerGoodFaultLogDelay() for more
     * information.
     *
     * @param services System services like hardware presence and the journal
     */
    void handlePowerGoodFault(Services& services);

    /**
     * Logs an error due to a power off attempt hitting a timeout.
     *
     * See getPowerGoodTimeout() for more information.
     *
     * @param services System services like hardware presence and the journal
     */
    void logPowerOffTimeout(Services& services);

    /**
     * Logs an error due to a power good fault.
     *
     * Tries to find which voltage rail caused the power good fault. If no rail
     * is found, a more general error is logged.
     *
     * See hasPowerGoodFault() for more information.
     *
     * @param services System services like hardware presence and the journal
     */
    void logPowerGoodFault(Services& services);

    /**
     * Checks whether a power good fault has occurred on one of the voltage
     * rails within the chassis.
     *
     * If a power good fault was found, this method returns a string containing
     * the error that should be logged. If no fault was found, an empty string
     * is returned.
     *
     * @param services System services like hardware presence and the journal
     * @param additionalData Additional data to include in the error log if a
     *                       power good fault was found
     * @return error that should be logged if a power good fault was found, or
     *         an empty string if no power good fault was found
     */
    std::string findPowerGoodFaultInRail(
        std::map<std::string, std::string>& additionalData, Services& services);

    /**
     * Chassis number within the system.
     *
     * Chassis numbers start at 1 because chassis 0 represents the entire
     * system.
     */
    size_t number;

    /**
     * D-Bus inventory path of the chassis.
     */
    std::string inventoryPath{};

    /**
     * Power sequencer devices within the chassis.
     */
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers{};

    /**
     * Types of chassis status monitoring to perform.
     */
    ChassisStatusMonitorOptions monitorOptions{};

    /**
     * Monitors the chassis status using D-Bus properties.
     */
    std::unique_ptr<ChassisStatusMonitor> statusMonitor{};

    /**
     * Last requested chassis power state.
     */
    std::optional<PowerState> powerState{};

    /**
     * Chassis power good.
     */
    std::optional<PowerGood> powerGood{};

    /**
     * Indicates whether the chassis is in a power state transition.
     *
     * See isInPowerStateTransition() for more information.
     */
    bool isInStateTransition{false};

    /**
     * Power good fault that was detected during the current power on attempt,
     * if any.
     *
     * See hasPowerGoodFault() for more information.
     */
    std::optional<PowerGoodFault> powerGoodFault{};

    /**
     * Timeout that indicates a power state change has taken too much time and
     * has failed.
     *
     * The timeout is expressed in milliseconds. Normally the timeout will be
     * some number of seconds, but milliseconds are used to enable fast timeouts
     * during automated testing.
     *
     * The default value is defined by a build option that is expressed in
     * seconds.
     */
    std::chrono::milliseconds powerGoodTimeout =
        std::chrono::seconds{PGOOD_TIMEOUT};

    /**
     * System time when timeout will occur for the current power on/off attempt.
     *
     * See powerGoodTimeout for more information.
     */
    std::chrono::time_point<std::chrono::steady_clock> powerGoodTimeoutTime{};

    /**
     * Delay time between detecting a power good fault and logging an error.
     *
     * See getPowerGoodFaultLogDelay() for more information.
     */
    std::chrono::milliseconds powerGoodFaultLogDelay = std::chrono::seconds(7);

    /**
     * Power supply error occurring in this chassis, if any.
     *
     * If a power supply error is occurring, it could cause a power good fault.
     *
     * The power supply monitoring application will notify the power sequencer
     * application using a D-Bus interface. The error string will be stored in
     * this data member.
     *
     * If no power supply error is occurring, this data member is set to the
     * empty string.
     */
    std::string powerSupplyError{};
};

} // namespace phosphor::power::sequencer
