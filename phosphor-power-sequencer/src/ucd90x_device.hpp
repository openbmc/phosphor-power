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

#include "pmbus_driver_device.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class UCD90xDevice
 *
 * PMBusDriverDevice sub-class for the UCD90X family of power sequencer devices.
 *
 * These devices share a common device driver.
 */
class UCD90xDevice : public PMBusDriverDevice
{
  public:
    // Specify which compiler-generated methods we want
    UCD90xDevice() = delete;
    UCD90xDevice(const UCD90xDevice&) = delete;
    UCD90xDevice(UCD90xDevice&&) = delete;
    UCD90xDevice& operator=(const UCD90xDevice&) = delete;
    UCD90xDevice& operator=(UCD90xDevice&&) = delete;
    virtual ~UCD90xDevice() = default;

    /**
     * Constructor.
     *
     * @param name Device name
     * @param bus I2C bus for the device
     * @param address I2C address for the device
     * @param powerControlGPIOName Name of the GPIO that turns this device on
     *                             and off
     * @param powerGoodGPIOName Name of the GPIO that reads the power good
     *                          signal from this device
     * @param rails Voltage rails that are enabled and monitored by this device
     * @param services System services like hardware presence and the journal
     */
    explicit UCD90xDevice(
        const std::string& name, uint8_t bus, uint16_t address,
        const std::string& powerControlGPIOName,
        const std::string& powerGoodGPIOName,
        std::vector<std::unique_ptr<Rail>> rails, Services& services) :
        PMBusDriverDevice(name, bus, address, powerControlGPIOName,
                          powerGoodGPIOName, std::move(rails), services,
                          driverName)
    {}

    /**
     * Returns the value of the PMBus MFR_STATUS command.
     *
     * This is a manufacturer-specific command that replaces the standard
     * STATUS_MFR_SPECIFIC command on UCD90x devices.
     *
     * The returned value is in host-endian order.
     *
     * Note that the UCD90x documentation states that this is a paged command.
     * This means that the PMBus PAGE should be set, and some of the bits in the
     * command value are page-specific.  However, the current device driver only
     * provides a single file in sysfs, and the driver always sets the PAGE to
     * 0.  Thus, the bits that are page-specific in the returned value are
     * always for PAGE 0.
     *
     * Throws an exception if the value could not be obtained.
     *
     * @return MFR_STATUS value
     */
    virtual uint64_t getMfrStatus();

    constexpr static std::string driverName{"ucd9000"};

  protected:
    /** @copydoc PMBusDriverDevice::storePgoodFaultDebugData() */
    virtual void storePgoodFaultDebugData(
        Services& services, const std::vector<int>& gpioValues,
        std::map<std::string, std::string>& additionalData) override;
};

} // namespace phosphor::power::sequencer
