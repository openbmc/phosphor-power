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
#pragma once

#include "chassis_status_monitor.hpp"
#include "device.hpp"
#include "id_map.hpp"
#include "services.hpp"

#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

using ChassisStatusMonitorOptions =
    phosphor::power::util::ChassisStatusMonitorOptions;
using ChassisStatusMonitor = phosphor::power::util::ChassisStatusMonitor;

// Forward declarations to avoid circular dependencies
class System;

/**
 * @class Chassis
 *
 * A chassis within the system.
 *
 * Chassis are typically physical enclosures that contain system components
 * such as CPUs, fans, power supplies, and PCIe cards. A chassis can be
 * stand-alone, such as a tower or desktop. A chassis can also be designed to be
 * mounted in an equipment rack.
 *
 * A C++ Chassis object only needs to be created if the physical chassis
 * contains regulators that need to be configured or monitored.
 */
class Chassis
{
  public:
    // Specify which compiler-generated methods we want
    Chassis() = delete;
    Chassis(const Chassis&) = delete;
    Chassis(Chassis&&) = delete;
    Chassis& operator=(const Chassis&) = delete;
    Chassis& operator=(Chassis&&) = delete;
    ~Chassis() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param number Chassis number within the system.  Chassis numbers start at
     *               1 because chassis 0 represents the entire system.
     * @param inventoryPath D-Bus inventory path for this chassis
     * @param monitorOptions Options that specify what types of chassis status
     *                       monitoring are enabled.
     * @param devices Devices within this chassis, if any.  The vector should
     *                contain regulator devices and any related devices required
     *                to perform regulator operations.
     */
    explicit Chassis(unsigned int number, const std::string& inventoryPath,
                     const ChassisStatusMonitorOptions& monitorOptions,
                     std::vector<std::unique_ptr<Device>> devices =
                         std::vector<std::unique_ptr<Device>>{}) :
        number{number}, inventoryPath{inventoryPath},
        monitorOptions{monitorOptions}, devices{std::move(devices)}
    {
        if (number < 1)
        {
            throw std::invalid_argument{
                "Invalid chassis number: " + std::to_string(number)};
        }

        // Monitor chassis power state/power good. Needed for isPoweredOn().
        this->monitorOptions.isPowerStateMonitored = true;
        this->monitorOptions.isPowerGoodMonitored = true;
    }

    /**
     * Adds the Device and Rail objects in this chassis to the specified IDMap.
     *
     * @param idMap mapping from IDs to the associated Device/Rail/Rule objects
     */
    void addToIDMap(IDMap& idMap);

    /**
     * Returns whether the devices in this chassis can be configured.
     *
     * If the chassis status is not valid, such as not present, then the devices
     * cannot be configured.
     *
     * @param services system services like error logging and the journal
     * @return true if devices can be configured, false otherwise
     */
    bool canConfigure(Services& services);

    /**
     * Returns whether the devices in this chassis can be monitored.
     *
     * If the chassis status is not valid, such as not present, then the devices
     * cannot be monitored.
     *
     * @param services system services like error logging and the journal
     * @return true if devices can be monitored, false otherwise
     */
    bool canMonitor(Services& services);

    /**
     * Clear any cached data about hardware devices.
     */
    void clearCache();

    /**
     * Clears all error history.
     *
     * All data on previously logged errors will be deleted.  If errors occur
     * again in the future they will be logged again.
     *
     * This method is normally called when the system is being powered on.
     */
    void clearErrorHistory();

    /**
     * Close the devices within this chassis, if any.
     *
     * @param services system services like error logging and the journal
     */
    void closeDevices(Services& services);

    /**
     * Configure the devices within this chassis, if any.
     *
     * This method should be called during the boot before regulators are
     * enabled.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains this chassis
     */
    void configure(Services& services, System& system);

    /**
     * Detect redundant phase faults in regulator devices in this chassis.
     *
     * This method should be called repeatedly based on a timer.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains this chassis
     */
    void detectPhaseFaults(Services& services, System& system);

    /**
     * Returns the devices within this chassis, if any.
     *
     * The vector contains regulator devices and any related devices
     * required to perform regulator operations.
     *
     * @return devices in chassis
     */
    const std::vector<std::unique_ptr<Device>>& getDevices() const
    {
        return devices;
    }

    /**
     * Returns the D-Bus inventory path for this chassis.
     *
     * @return inventory path
     */
    const std::string& getInventoryPath() const
    {
        return inventoryPath;
    }

    /**
     * Returns the options that specify what types of chassis status monitoring
     * are enabled.
     *
     * @return chassis status monitoring options
     */
    const ChassisStatusMonitorOptions& getMonitorOptions() const
    {
        return monitorOptions;
    }

    /**
     * Returns the chassis number within the system.
     *
     * @return chassis number
     */
    unsigned int getNumber() const
    {
        return number;
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
     * @param services system services like error logging and the journal
     */
    void initializeMonitoring(Services& services)
    {
        // Note: replaces/deletes any previous monitor object
        statusMonitor = services.createChassisStatusMonitor(
            number, inventoryPath, monitorOptions);
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
     * Returns whether the chassis is powered on.
     *
     * Throws an exception if:
     * - Chassis monitoring has not been initialized
     * - D-Bus property value could not be obtained
     *
     * @return true if chassis is powered on, false otherwise
     */
    bool isPoweredOn()
    {
        verifyMonitoringInitialized();
        return statusMonitor->isPoweredOn();
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
     * Monitors the sensors for the voltage rails produced by this chassis, if
     * any.
     *
     * This method should be called repeatedly based on a timer.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains the chassis
     */
    void monitorSensors(Services& services, System& system);

  private:
    /**
     * Close the devices within this chassis, if any.
     *
     * @param services system services like error logging and the journal
     * @param ignoreErrors if true, do not write errors to the journal or create
     *                     an error log
     */
    void closeDevices(Services& services, bool ignoreErrors);

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
     * Chassis number within the system.
     *
     * Chassis numbers start at 1 because chassis 0 represents the entire
     * system.
     */
    const unsigned int number{};

    /**
     * D-Bus inventory path for this chassis.
     */
    const std::string inventoryPath{};

    /**
     * Options that specify what types of chassis status monitoring are enabled.
     */
    ChassisStatusMonitorOptions monitorOptions{};

    /**
     * Devices within this chassis, if any.
     *
     * The vector contains regulator devices and any related devices
     * required to perform regulator operations.
     */
    std::vector<std::unique_ptr<Device>> devices{};

    /**
     * Monitors the chassis status using D-Bus properties.
     */
    std::unique_ptr<ChassisStatusMonitor> statusMonitor{};

    /**
     * Maximum number of consecutive errors to count when obtaining chassis
     * status.
     */
    static constexpr uint8_t maxStatusErrorCount{3};

    /**
     * Number of consecutive errors that have occurred when obtaining chassis
     * status.
     */
    uint8_t statusErrorCount{0};
};

} // namespace phosphor::power::regulators
