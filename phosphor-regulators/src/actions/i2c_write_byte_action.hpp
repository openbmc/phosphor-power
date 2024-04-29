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
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class I2CWriteByteAction
 *
 * Writes a byte to a device register.  Communicates with the device directly
 * using the I2C interface.
 *
 * Implements the i2c_write_byte action in the JSON config file.
 */
class I2CWriteByteAction : public I2CAction
{
  public:
    // Specify which compiler-generated methods we want
    I2CWriteByteAction() = delete;
    I2CWriteByteAction(const I2CWriteByteAction&) = delete;
    I2CWriteByteAction(I2CWriteByteAction&&) = delete;
    I2CWriteByteAction& operator=(const I2CWriteByteAction&) = delete;
    I2CWriteByteAction& operator=(I2CWriteByteAction&&) = delete;
    virtual ~I2CWriteByteAction() = default;

    /**
     * Constructor.
     *
     * @param reg Device register address.  Note: named 'reg' because 'register'
     *            is a reserved keyword.
     * @param value Byte value to write.
     * @param mask Bit mask.  Specifies which bits to write within the byte
     *             value.  Only the bits with a value of 1 in the mask will be
     *             written.
     */
    explicit I2CWriteByteAction(uint8_t reg, uint8_t value,
                                uint8_t mask = 0xFF) :
        reg{reg}, value{value}, mask{mask}
    {}

    /**
     * Executes this action.
     *
     * Writes a byte to a device register using the I2C interface.
     *
     * The device register, byte value, and mask (if any) were specified in the
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
     * Returns the byte value to write.
     *
     * @return value to write
     */
    uint8_t getValue() const
    {
        return value;
    }

    /**
     * Returns the bit mask.
     *
     * Specifies which bits to write within the byte value.  Only the bits with
     * a value of 1 in the mask will be written.
     *
     * @return bit mask
     */
    uint8_t getMask() const
    {
        return mask;
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
     * Byte value to write.
     */
    const uint8_t value{0x00};

    /**
     * Bit mask.  Specifies which bits to write within the byte value.  Only the
     * bits with a value of 1 in the mask will be written.
     */
    const uint8_t mask{0xFF};
};

} // namespace phosphor::power::regulators
