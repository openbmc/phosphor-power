/**
 * Copyright © 2021 IBM Corporation
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

#include "action_environment.hpp"
#include "i2c_action.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class I2CCaptureBytesAction
 *
 * Captures device register bytes to be stored in an error log.  Communicates
 * with the device directly using the I2C interface.
 *
 * Implements the i2c_capture_bytes action in the JSON config file.
 */
class I2CCaptureBytesAction : public I2CAction
{
  public:
    // Specify which compiler-generated methods we want
    I2CCaptureBytesAction() = delete;
    I2CCaptureBytesAction(const I2CCaptureBytesAction&) = delete;
    I2CCaptureBytesAction(I2CCaptureBytesAction&&) = delete;
    I2CCaptureBytesAction& operator=(const I2CCaptureBytesAction&) = delete;
    I2CCaptureBytesAction& operator=(I2CCaptureBytesAction&&) = delete;
    virtual ~I2CCaptureBytesAction() = default;

    /**
     * Constructor.
     *
     * Throws an exception if the count parameter is invalid.
     *
     * @param reg Device register address.  Note: named 'reg' because 'register'
     *            is a reserved keyword.
     * @param count Number of bytes to read from the device register.
     */
    explicit I2CCaptureBytesAction(uint8_t reg, uint8_t count) :
        reg{reg}, count{count}
    {
        if (count < 1)
        {
            throw std::invalid_argument{"Invalid byte count: Less than 1"};
        }
    }

    /**
     * Executes this action.
     *
     * Reads one or more bytes from a device register using the I2C interface.
     * The resulting values are stored as additional error data in the specified
     * action environment.
     *
     * All of the bytes will be read in a single I2C operation.
     *
     * The device register was specified in the constructor.
     *
     * The device is obtained from the action environment.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return true
     */
    virtual bool execute(ActionEnvironment& environment) override;

    /**
     * Returns the number of bytes to read from the device register.
     *
     * @return byte count
     */
    uint8_t getCount() const
    {
        return count;
    }

    /**
     * Returns the device register address.
     *
     * @return register address
     */
    uint8_t getRegister() const
    {
        return reg;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override;

  private:
    /**
     * Returns the key for storing additional error data as a key/value pair in
     * the action environment.
     *
     * @param environment action execution environment
     * @return error data key
     */
    std::string getErrorDataKey(ActionEnvironment& environment) const;

    /**
     * Returns the value for storing additional error data as a key/value pair
     * in the action environment.
     *
     * @param values Array of byte values read from the device.  The count data
     *               member specifies the number of bytes that were read.
     * @return error data value
     */
    std::string getErrorDataValue(const uint8_t* values) const;

    /**
     * Device register address.  Note: named 'reg' because 'register' is a
     * reserved keyword.
     */
    const uint8_t reg;

    /**
     * Number of bytes to read from the device register.
     */
    const uint8_t count;
};

} // namespace phosphor::power::regulators
