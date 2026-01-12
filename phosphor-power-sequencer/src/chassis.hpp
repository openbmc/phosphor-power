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
     * Monitors the status of the chassis.
     *
     * Sets the chassis power good value by reading the power good value from
     * each power sequencer device.
     *
     * Reacts to any changes to chassis D-Bus properties.
     *
     * This method must be called periodically (such as once per second) to
     * ensure the chassis power state and power good values are correct.
     *
     * Throws an exception if one of the following occurs:
     * - Chassis monitoring has not been initialized.
     * - Chassis D-Bus status cannot be obtained.
     *
     * @param services System services like hardware presence and the journal
     */
    void monitor(Services& services);

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
     * Returns the power good timeout.
     *
     * This timeout indicates a power state change has taken too much time and
     * has failed.
     *
     * @return timeout in milliseconds
     */
    std::chrono::milliseconds getPowerGoodTimeOut() const
    {
        return powerGoodTimeOut;
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
     * @param newTimeOut New timeout value
     */
    void setPowerGoodTimeOut(std::chrono::milliseconds newTimeOut)
    {
        powerGoodTimeOut = newTimeOut;
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
    std::chrono::milliseconds powerGoodTimeOut =
        std::chrono::seconds{PGOOD_TIMEOUT};
};

} // namespace phosphor::power::sequencer
