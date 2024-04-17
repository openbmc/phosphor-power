/**
 * Copyright © 2024 IBM Corporation
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

#include "power_sequencer_device.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class StandardDevice
 *
 * PowerSequencerDevice sub-class that implements the standard pgood fault
 * detection algorithm.
 *
 * When adding support for a new power sequencer device type, create a sub-class
 * of StandardDevice if possible.  This will ensure that pgood fault detection
 * works consistently across device types.
 */
class StandardDevice : public PowerSequencerDevice
{
  public:
    // Specify which compiler-generated methods we want
    StandardDevice() = delete;
    StandardDevice(const StandardDevice&) = delete;
    StandardDevice(StandardDevice&&) = delete;
    StandardDevice& operator=(const StandardDevice&) = delete;
    StandardDevice& operator=(StandardDevice&&) = delete;
    virtual ~StandardDevice() = default;

    /**
     * Constructor.
     *
     * @param name device name
     * @param rails voltage rails that are enabled and monitored by this device
     */
    explicit StandardDevice(const std::string& name,
                            std::vector<std::unique_ptr<Rail>> rails) :
        name{name},
        rails{std::move(rails)}
    {}

    /** @copydoc PowerSequencerDevice::getName() */
    virtual const std::string& getName() const override
    {
        return name;
    }

    /** @copydoc PowerSequencerDevice::getRails() */
    virtual const std::vector<std::unique_ptr<Rail>>& getRails() const override
    {
        return rails;
    }

    /**
     * Returns whether a pgood fault has occurred on one of the rails being
     * monitored by this device.
     *
     * Calls prepareForPgoodFaultDetection() before starting detection.  If a
     * pgood fault is detected, calls storePgoodFaultDebugData().
     *
     * Throws an exception if an error occurs while trying to obtain the status
     * of the rails.
     *
     * @param services System services like hardware presence and the journal
     * @param powerSupplyError Power supply error that occurred before the pgood
     *                         fault.  Set to the empty string if no power
     *                         supply error occurred.  This error may be the
     *                         root cause if a pgood fault occurred on a power
     *                         supply rail monitored by this device.
     * @param error Error that should be logged if this method returns true.
     * @param additionalData Additional data to include in the error log if
     *                       this method returns true.
     * @return true if a pgood fault was found on a rail monitored by this
     *         device, false otherwise
     */
    virtual bool hasPgoodFault(
        Services& services, const std::string& powerSupplyError,
        std::string& error,
        std::map<std::string, std::string>& additionalData) override;

  protected:
    /**
     * Prepare for pgood fault detection.
     *
     * Perform any actions that are necessary to prepare for fault detection.
     * For example, cache information that is slow to obtain and is used
     * multiple times during detection.
     *
     * Default implementation does nothing.  Override in sub-classes if needed.
     *
     * @param services System services like hardware presence and the journal
     */
    virtual void
        prepareForPgoodFaultDetection([[maybe_unused]] Services& services)
    {}

    /**
     * Returns the GPIO values that can be read from the device, if possible.
     *
     * If the device does not support reading GPIO values or an error occurs, an
     * empty vector is returned.
     *
     * @return GPIO values, or empty vector if values could not be read
     */
    virtual std::vector<int> getGPIOValuesIfPossible();

    /**
     * Store pgood fault debug data in the specified additional data map.
     *
     * The default implementation stores the device name and all the GPIO
     * values.  The GPIO values are stored as a simple list of integers.
     *
     * Sub-classes should override if needed to store device-specific data
     * and/or a formatted representation of the GPIO values.
     *
     * This method should NOT throw exceptions.  If debug data cannot be
     * obtained, the error should be caught and ignored so that pgood error
     * handling can continue.
     *
     * @param services System services like hardware presence and the journal
     * @param gpioValues GPIO values obtained from the device (if any)
     * @param additionalData Additional data to include in an error log
     */
    virtual void storePgoodFaultDebugData(
        Services& services, const std::vector<int>& gpioValues,
        std::map<std::string, std::string>& additionalData);

    /**
     * Device name.
     */
    std::string name{};

    /**
     * Voltage rails that are enabled and monitored by this device.
     */
    std::vector<std::unique_ptr<Rail>> rails{};
};

} // namespace phosphor::power::sequencer
