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

#include "action_environment.hpp"
#include "i2c_action.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class I2CWriteBitAction
 *
 * Writes a bit to a device register.  Communicates with the device directly
 * using the I2C interface.
 *
 * Implements the i2c_write_bit action in the JSON config file.
 */
class I2CWriteBitAction : public I2CAction
{
  public:
    // Specify which compiler-generated methods we want
    I2CWriteBitAction() = delete;
    I2CWriteBitAction(const I2CWriteBitAction&) = delete;
    I2CWriteBitAction(I2CWriteBitAction&&) = delete;
    I2CWriteBitAction& operator=(const I2CWriteBitAction&) = delete;
    I2CWriteBitAction& operator=(I2CWriteBitAction&&) = delete;
    virtual ~I2CWriteBitAction() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param reg Device register address.  Note: named 'reg' because 'register'
     *            is a reserved keyword.
     * @param position Bit position.  Must be in the range 0-7.  Bit 0 is the
     *                 least significant bit.
     * @param value Bit value to write.  Must be 0 or 1.
     */
    explicit I2CWriteBitAction(uint8_t reg, uint8_t position, uint8_t value) :
        reg{reg}, position{position}, value{value}
    {
        if (position > 7)
        {
            throw std::invalid_argument{
                "Invalid bit position: " +
                std::to_string(static_cast<unsigned>(position))};
        }

        if (value > 1)
        {
            throw std::invalid_argument{
                "Invalid bit value: " +
                std::to_string(static_cast<unsigned>(value))};
        }
    }

    /**
     * Executes this action.
     *
     * Writes a bit to a device register using the I2C interface.
     *
     * The device register, bit position, and bit value were specified in the
     * constructor.
     *
     * The device is obtained from the specified action environment.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return true
     */
    virtual bool execute(ActionEnvironment& environment) override;

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
     * Returns the bit position.
     *
     * Value is in the range 0-7.  Bit 0 is the least significant bit.
     *
     * @return bit position
     */
    uint8_t getPosition() const
    {
        return position;
    }

    /**
     * Returns the bit value to write.
     *
     * Value is 0 or 1.
     *
     * @return bit value
     */
    uint8_t getValue() const
    {
        return value;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override;

  private:
    /**
     * Device register address.  Note: named 'reg' because 'register' is a
     * reserved keyword.
     */
    const uint8_t reg{0x00};

    /**
     * Bit position.  Must be in the range 0-7.  Bit 0 is the least significant
     * bit.
     */
    const uint8_t position{0};

    /**
     * Bit value to write.  Must be 0 or 1.
     */
    const uint8_t value{0};
};

} // namespace phosphor::power::regulators
