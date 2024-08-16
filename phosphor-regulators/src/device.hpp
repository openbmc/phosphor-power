/**
 * Copyright © 2019 IBM Corporation
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

#include "configuration.hpp"
#include "i2c_interface.hpp"
#include "id_map.hpp"
#include "phase_fault_detection.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

// Forward declarations to avoid circular dependencies
class Chassis;
class System;

/**
 * @class Device
 *
 * A hardware device, such as a voltage regulator or I/O expander.
 */
class Device
{
  public:
    // Specify which compiler-generated methods we want
    Device() = delete;
    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;
    ~Device() = default;

    /**
     * Constructor.
     *
     * @param id unique device ID
     * @param isRegulator indicates whether this device is a voltage regulator
     * @param fru Field-Replaceable Unit (FRU) for this device
     * @param i2cInterface I2C interface to this device
     * @param presenceDetection presence detection for this device, if any
     * @param configuration configuration changes to apply to this device, if
     *                      any
     * @param phaseFaultDetection phase fault detection for this device, if any
     * @param rails voltage rails produced by this device, if any
     */
    explicit Device(
        const std::string& id, bool isRegulator, const std::string& fru,
        std::unique_ptr<i2c::I2CInterface> i2cInterface,
        std::unique_ptr<PresenceDetection> presenceDetection = nullptr,
        std::unique_ptr<Configuration> configuration = nullptr,
        std::unique_ptr<PhaseFaultDetection> phaseFaultDetection = nullptr,
        std::vector<std::unique_ptr<Rail>> rails =
            std::vector<std::unique_ptr<Rail>>{}) :
        id{id}, isRegulatorDevice{isRegulator}, fru{fru},
        i2cInterface{std::move(i2cInterface)},
        presenceDetection{std::move(presenceDetection)},
        configuration{std::move(configuration)},
        phaseFaultDetection{std::move(phaseFaultDetection)},
        rails{std::move(rails)}
    {}

    /**
     * Adds this Device object to the specified IDMap.
     *
     * Also adds any Rail objects in this Device to the IDMap.
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
     * Closes this device.
     *
     * Closes any interfaces that are open to this device.  Releases any other
     * operating system resources associated with this device.
     *
     * @param services system services like error logging and the journal
     */
    void close(Services& services);

    /**
     * Configure this device.
     *
     * Applies the configuration changes that are defined for this device, if
     * any.
     *
     * Also configures the voltage rails produced by this device, if any.
     *
     * This method should be called during the boot before regulators are
     * enabled.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains the chassis
     * @param chassis chassis that contains this device
     */
    void configure(Services& services, System& system, Chassis& chassis);

    /**
     * Detect redundant phase faults in this device.
     *
     * Does nothing if phase fault detection is not defined for this device.
     *
     * This method should be called repeatedly based on a timer.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains the chassis
     * @param chassis chassis that contains the device
     */
    void detectPhaseFaults(Services& services, System& system,
                           Chassis& chassis);

    /**
     * Returns the configuration changes to apply to this device, if any.
     *
     * @return Pointer to Configuration object.  Will equal nullptr if no
     *         configuration changes are defined for this device.
     */
    const std::unique_ptr<Configuration>& getConfiguration() const
    {
        return configuration;
    }

    /**
     * Returns the Field-Replaceable Unit (FRU) for this device.
     *
     * Returns the D-Bus inventory path of the FRU.  If the device itself is not
     * a FRU, returns the FRU that contains the device.
     *
     * @return FRU for this device
     */
    const std::string& getFRU() const
    {
        return fru;
    }

    /**
     * Returns the I2C interface to this device.
     *
     * @return I2C interface to device
     */
    i2c::I2CInterface& getI2CInterface()
    {
        return *i2cInterface;
    }

    /**
     * Returns the unique ID of this device.
     *
     * @return device ID
     */
    const std::string& getID() const
    {
        return id;
    }

    /**
     * Returns the phase fault detection for this device, if any.
     *
     * @return Pointer to PhaseFaultDetection object.  Will equal nullptr if no
     *         phase fault detection is defined for this device.
     */
    const std::unique_ptr<PhaseFaultDetection>& getPhaseFaultDetection() const
    {
        return phaseFaultDetection;
    }

    /**
     * Returns the presence detection for this device, if any.
     *
     * @return Pointer to PresenceDetection object.  Will equal nullptr if no
     *         presence detection is defined for this device.
     */
    const std::unique_ptr<PresenceDetection>& getPresenceDetection() const
    {
        return presenceDetection;
    }

    /**
     * Returns the voltage rails produced by this device, if any.
     *
     * @return voltage rails
     */
    const std::vector<std::unique_ptr<Rail>>& getRails() const
    {
        return rails;
    }

    /**
     * Returns whether this device is present.
     *
     * @return true if device is present, false otherwise
     */
    bool isPresent(Services& services, System& system, Chassis& chassis)
    {
        if (presenceDetection)
        {
            // Execute presence detection to determine if device is present
            return presenceDetection->execute(services, system, chassis, *this);
        }
        else
        {
            // No presence detection defined; assume device is present
            return true;
        }
    }

    /**
     * Returns whether this device is a voltage regulator.
     *
     * @return true if device is a voltage regulator, false otherwise
     */
    bool isRegulator() const
    {
        return isRegulatorDevice;
    }

    /**
     * Monitors the sensors for the voltage rails produced by this device, if
     * any.
     *
     * This method should be called repeatedly based on a timer.
     *
     * @param services system services like error logging and the journal
     * @param system system that contains the chassis
     * @param chassis chassis that contains the device
     */
    void monitorSensors(Services& services, System& system, Chassis& chassis);

  private:
    /**
     * Unique ID of this device.
     */
    const std::string id{};

    /**
     * Indicates whether this device is a voltage regulator.
     */
    const bool isRegulatorDevice{false};

    /**
     * Field-Replaceable Unit (FRU) for this device.
     *
     * Set to the D-Bus inventory path of the FRU.  If the device itself is not
     * a FRU, set to the FRU that contains the device.
     */
    const std::string fru{};

    /**
     * I2C interface to this device.
     */
    std::unique_ptr<i2c::I2CInterface> i2cInterface{};

    /**
     * Presence detection for this device, if any.  Set to nullptr if no
     * presence detection is defined for this device.
     */
    std::unique_ptr<PresenceDetection> presenceDetection{};

    /**
     * Configuration changes to apply to this device, if any.  Set to nullptr if
     * no configuration changes are defined for this device.
     */
    std::unique_ptr<Configuration> configuration{};

    /**
     * Phase fault detection for this device, if any.  Set to nullptr if no
     * phase fault detection is defined for this device.
     */
    std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};

    /**
     * Voltage rails produced by this device, if any.  Vector is empty if no
     * voltage rails are defined for this device.
     */
    std::vector<std::unique_ptr<Rail>> rails{};
};

} // namespace phosphor::power::regulators
