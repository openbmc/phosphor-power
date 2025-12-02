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

#include "gpio.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class PowerSequencerDevice
 *
 * Abstract base class for a hardware device that performs the following tasks:
 * - Enables (turns on) the voltage rails in the proper sequence
 * - Checks the pgood (power good) status of each voltage rail
 */
class PowerSequencerDevice
{
  public:
    // Specify which compiler-generated methods we want
    PowerSequencerDevice() = default;
    PowerSequencerDevice(const PowerSequencerDevice&) = delete;
    PowerSequencerDevice(PowerSequencerDevice&&) = delete;
    PowerSequencerDevice& operator=(const PowerSequencerDevice&) = delete;
    PowerSequencerDevice& operator=(PowerSequencerDevice&&) = delete;
    virtual ~PowerSequencerDevice() = default;

    /**
     * Returns the device name.
     *
     * @return device name
     */
    virtual const std::string& getName() const = 0;

    /**
     * Returns the I2C bus for the device.
     *
     * @return I2C bus
     */
    virtual uint8_t getBus() const = 0;

    /**
     * Returns the I2C address for the device.
     *
     * @return I2C address
     */
    virtual uint16_t getAddress() const = 0;

    /**
     * Returns the name of the GPIO that turns this device on and off.
     *
     * @return GPIO name
     */
    virtual const std::string& getPowerControlGPIOName() const = 0;

    /**
     * Returns the name of the GPIO that reads the power good signal from this
     * device.
     *
     * @return GPIO name
     */
    virtual const std::string& getPowerGoodGPIOName() const = 0;

    /**
     * Returns the voltage rails that are enabled and monitored by this device.
     *
     * @return voltage rails
     */
    virtual const std::vector<std::unique_ptr<Rail>>& getRails() const = 0;

    /**
     * Open this device.
     *
     * This method must be called before any methods that access the device or
     * the named GPIOs.
     *
     * Does nothing if the device is already open.
     *
     * Throws an exception if an error occurs.
     *
     * @param services System services like hardware presence and the journal
     */
    virtual void open(Services& services) = 0;

    /**
     * Returns whether this device is open.
     *
     * @return true if device is open, false otherwise
     */
    virtual bool isOpen() const = 0;

    /**
     * Close this device.
     *
     * Does nothing if the device is not open.
     *
     * Throws an exception if an error occurs.
     */
    virtual void close() = 0;

    /**
     * Close this device, ignoring any errors that occur.
     *
     * Does nothing if the device is not open.
     */
    virtual void closeWithoutException() noexcept = 0;

    /**
     * Returns the GPIO that turns this device on and off.
     *
     * Throws an exception if the device is not open.
     *
     * @return GPIO object
     */
    virtual GPIO& getPowerControlGPIO() = 0;

    /**
     * Returns the GPIO that reads the power good signal from this device.
     *
     * Throws an exception if the device is not open.
     *
     * @return GPIO object
     */
    virtual GPIO& getPowerGoodGPIO() = 0;

    /**
     * Power on this device.
     *
     * Normally this means the device will power on the individual voltage rails
     * in the correct order, verifying each rail asserts power good before
     * moving to the next.
     *
     * When the power on is complete, the device will normally set the
     * device-level power good signal to true.
     *
     * Throws an exception if the device is not open or an error occurs powering
     * on the device.
     */
    virtual void powerOn() = 0;

    /**
     * Power off this device.
     *
     * Normally this means the device will power off the individual voltage
     * rails in the correct order.
     *
     * When the power off is complete, the device will normally set the
     * device-level power good signal to false.
     *
     * Throws an exception if the device is not open or an error occurs powering
     * off the device.
     */
    virtual void powerOff() = 0;

    /**
     * Returns the power good signal for this device.
     *
     * A return value of true normally indicates that all voltage rails being
     * monitored by this device are asserting power good.
     *
     * A return value of false normally indicates that at least one voltage rail
     * being monitored by this device is not asserting power good.
     *
     * Throws an exception if the device is not open or an error occurs
     * obtaining the power good signal from the device.
     *
     * @return power good signal for this device
     */
    virtual bool getPowerGood() = 0;

    /**
     * Returns the GPIO values that can be read from the device.
     *
     * The vector indices correspond to the libgpiod line offsets.  For example,
     * the element at vector index 0 is the GPIO value at libgpiod line offset
     * 0.  These offsets may correspond to logical pin IDs, but they are usually
     * different from the physical pin numbers on the device.  Consult the
     * device documentation for more information.
     *
     * Throws an exception if the device is not open, the values could not be
     * read, or the device does not support GPIO values.
     *
     * @param services System services like hardware presence and the journal
     * @return GPIO values
     */
    virtual std::vector<int> getGPIOValues(Services& services) = 0;

    /**
     * Returns the value of the PMBus STATUS_WORD command for the specified
     * PMBus page.
     *
     * The returned value is in host-endian order.
     *
     * Throws an exception if the device is not open, the value could not be
     * obtained, or the device does not support the STATUS_WORD command.
     *
     * @param page PMBus page
     * @return STATUS_WORD value
     */
    virtual uint16_t getStatusWord(uint8_t page) = 0;

    /**
     * Returns the value of the PMBus STATUS_VOUT command for the specified
     * PMBus page.
     *
     * Throws an exception if the device is not open, the value could not be
     * obtained, or the device does not support the STATUS_VOUT command.
     *
     * @param page PMBus page
     * @return STATUS_VOUT value
     */
    virtual uint8_t getStatusVout(uint8_t page) = 0;

    /**
     * Returns the value of the PMBus READ_VOUT command for the specified
     * PMBus page.
     *
     * The returned value is in Volts.
     *
     * Throws an exception if the device is not open, the value could not be
     * obtained, or the device does not support the READ_VOUT command.
     *
     * @param page PMBus page
     * @return READ_VOUT value in volts
     */
    virtual double getReadVout(uint8_t page) = 0;

    /**
     * Returns the value of the PMBus VOUT_UV_FAULT_LIMIT command for the
     * specified PMBus page.
     *
     * The returned value is in Volts.
     *
     * Throws an exception if the device is not open, the value could not be
     * obtained, or the device does not support the VOUT_UV_FAULT_LIMIT command.
     *
     * @param page PMBus page
     * @return VOUT_UV_FAULT_LIMIT value in volts
     */
    virtual double getVoutUVFaultLimit(uint8_t page) = 0;

    /**
     * Checks whether a pgood fault has occurred on one of the rails being
     * monitored by this device.
     *
     * If a pgood fault was found, this method returns a string containing the
     * error that should be logged.  If no fault was found, an empty string is
     * returned.
     *
     * Throws an exception if the device is not open or an error occurs while
     * trying to obtain the status of the rails.
     *
     * @param services System services like hardware presence and the journal
     * @param powerSupplyError Power supply error that occurred before the pgood
     *                         fault.  Set to the empty string if no power
     *                         supply error occurred.  This error may be the
     *                         root cause if a pgood fault occurred on a power
     *                         supply rail monitored by this device.
     * @param additionalData Additional data to include in the error log if
     *                       a pgood fault was found
     * @return error that should be logged if a pgood fault was found, or an
     *         empty string if no pgood fault was found
     */
    virtual std::string findPgoodFault(
        Services& services, const std::string& powerSupplyError,
        std::map<std::string, std::string>& additionalData) = 0;
};

} // namespace phosphor::power::sequencer
