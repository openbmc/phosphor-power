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

#include "rail.hpp"
#include "services.hpp"

#include <cstdint>
#include <map>
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
     * Returns the voltage rails that are enabled and monitored by this device.
     *
     * @return voltage rails
     */
    virtual const std::vector<std::unique_ptr<Rail>>& getRails() const = 0;

    /**
     * Returns the GPIO values that can be read from the device.
     *
     * The vector indices correspond to the libgpiod line offsets.  For example,
     * the element at vector index 0 is the GPIO value at libgpiod line offset
     * 0.  These offsets may correspond to logical pin IDs, but they are usually
     * different from the physical pin numbers on the device.  Consult the
     * device documentation for more information.
     *
     * Throws an exception if the values could not be read or the device does
     * not support GPIO values.
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
     * Throws an exception if the value could not be obtained or the device does
     * not support the STATUS_WORD command.
     *
     * @param page PMBus page
     * @return STATUS_WORD value
     */
    virtual uint16_t getStatusWord(uint8_t page) = 0;

    /**
     * Returns the value of the PMBus STATUS_VOUT command for the specified
     * PMBus page.
     *
     * Throws an exception if the value could not be obtained or the device does
     * not support the STATUS_VOUT command.
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
     * Throws an exception if the value could not be obtained or the device does
     * not support the READ_VOUT command.
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
     * Throws an exception if the value could not be obtained or the device does
     * not support the VOUT_UV_FAULT_LIMIT command.
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
     * Throws an exception if an error occurs while trying to obtain the status
     * of the rails.
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
