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

#include "device.hpp"
#include "id_map.hpp"
#include "services.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

// Forward declarations to avoid circular dependencies
class System;

/**
 * @class Chassis
 *
 * A chassis within the system.
 *
 * Chassis are large enclosures that can be independently powered off and on by
 * the BMC.  Small and mid-sized systems may contain a single chassis.  In a
 * large rack-mounted system, each drawer may correspond to a chassis.
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
     * @param devices Devices within this chassis, if any.  The vector should
     *                contain regulator devices and any related devices required
     *                to perform regulator operations.
     */
    explicit Chassis(unsigned int number, const std::string& inventoryPath,
                     std::vector<std::unique_ptr<Device>> devices =
                         std::vector<std::unique_ptr<Device>>{}) :
        number{number}, inventoryPath{inventoryPath},
        devices{std::move(devices)}
    {
        if (number < 1)
        {
            throw std::invalid_argument{
                "Invalid chassis number: " + std::to_string(number)};
        }
    }

    /**
     * Adds the Device and Rail objects in this chassis to the specified IDMap.
     *
     * @param idMap mapping from IDs to the associated Device/Rail/Rule objects
     */
    void addToIDMap(IDMap& idMap);

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
     * Returns the chassis number within the system.
     *
     * @return chassis number
     */
    unsigned int getNumber() const
    {
        return number;
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
     * Devices within this chassis, if any.
     *
     * The vector contains regulator devices and any related devices
     * required to perform regulator operations.
     */
    std::vector<std::unique_ptr<Device>> devices{};
};

} // namespace phosphor::power::regulators
