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

#include "chassis_status_monitor.hpp"
#include "power_sequencer_device.hpp"
#include "services.hpp"

#include <stddef.h> // for size_t

#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

using ChassisStatusMonitorOptions =
    phosphor::power::util::ChassisStatusMonitorOptions;

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
     * Initializes status monitoring for the chassis.
     *
     * Creates a ChassisStatusMonitor object based on the monitoring options
     * specified in the constructor.
     *
     * This method must be called before any methods that return chassis status
     * or return the ChassisStatusMonitor object.
     *
     * Normally this method is only called once. However, it can be called
     * multiple times if required, such as for automated testing.
     *
     * @param services System services like hardware presence and the journal
     */
    void initializeStatusMonitoring(Services& services)
    {
        // Note: replaces/deletes any previous monitor object
        statusMonitor = services.createChassisStatusMonitor(
            number, inventoryPath, monitorOptions);
    }

    /**
     * Returns the ChassisStatusMonitor object that is monitoring D-Bus
     * properties for the chassis.
     *
     * You must call initializeStatusMonitoring() before calling this method in
     * order to create the ChassisStatusMonitor object.
     *
     * Throws an exception if the ChassisStatusMonitor object does not exist.
     *
     * @return reference to ChassisStatusMonitor object
     */
    ChassisStatusMonitor& getStatusMonitor()
    {
        verifyStatusMonitoringInitialized();
        return *statusMonitor;
    }

    /**
     * Returns whether the chassis is present.
     *
     * Throws an exception if:
     * - Status monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true is chassis is present, false otherwise
     */
    bool isPresent()
    {
        verifyStatusMonitoringInitialized();
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
     * - Status monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true is chassis is available, false otherwise
     */
    bool isAvailable()
    {
        verifyStatusMonitoringInitialized();
        return statusMonitor->isAvailable();
    }

    /**
     * Returns whether the chassis is enabled.
     *
     * If the D-Bus Enabled property is false, it means that the chassis has
     * been put in hardware isolation (guarded).
     *
     * Throws an exception if:
     * - Status monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true is chassis is enabled, false otherwise
     */
    bool isEnabled()
    {
        verifyStatusMonitoringInitialized();
        return statusMonitor->isEnabled();
    }

    /**
     * Returns whether the chassis input power status is good.
     *
     * Throws an exception if:
     * - Status monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true if chassis input power is good, false otherwise
     */
    bool isInputPowerGood()
    {
        verifyStatusMonitoringInitialized();
        return statusMonitor->isInputPowerGood();
    }

    /**
     * Returns whether the power supplies power status is good.
     *
     * Throws an exception if:
     * - Status monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true if power supplies power is good, false otherwise
     */
    bool isPowerSuppliesPowerGood()
    {
        verifyStatusMonitoringInitialized();
        return statusMonitor->isPowerSuppliesPowerGood();
    }

  private:
    /**
     * Verifies that status monitoring has been initialized and a
     * ChassisStatusMonitor object has been created.
     *
     * Throws an exception if monitoring has not been initialized.
     */
    void verifyStatusMonitoringInitialized()
    {
        if (!statusMonitor)
        {
            throw std::runtime_error{std::format(
                "Status monitoring not initialized for chassis {}", number)};
        }
    }

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
};

} // namespace phosphor::power::sequencer
