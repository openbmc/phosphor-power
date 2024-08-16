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
 * @class I2CCompareByteAction
 *
 * Compares a device register to a byte value.  Communicates with the device
 * directly using the I2C interface.
 *
 * Implements the i2c_compare_byte action in the JSON config file.
 */
class I2CCompareByteAction : public I2CAction
{
  public:
    // Specify which compiler-generated methods we want
    I2CCompareByteAction() = delete;
    I2CCompareByteAction(const I2CCompareByteAction&) = delete;
    I2CCompareByteAction(I2CCompareByteAction&&) = delete;
    I2CCompareByteAction& operator=(const I2CCompareByteAction&) = delete;
    I2CCompareByteAction& operator=(I2CCompareByteAction&&) = delete;
    virtual ~I2CCompareByteAction() = default;

    /**
     * Constructor.
     *
     * @param reg Device register address.  Note: named 'reg' because 'register'
     *            is a reserved keyword.
     * @param value Expected byte value.
     * @param mask Bit mask.  Specifies which bits should be compared within the
     *             byte value.  Only the bits with a value of 1 in the mask will
     *             be compared.  If not specified, defaults to 0xFF which means
     *             that all bits will be compared.
     */
    explicit I2CCompareByteAction(uint8_t reg, uint8_t value,
                                  uint8_t mask = 0xFF) :
        reg{reg}, value{value}, mask{mask}
    {}

    /**
     * Executes this action.
     *
     * Compares a device register to a byte value using the I2C interface.
     *
     * The device register, byte value, and mask (if any) were specified in the
     * constructor.
     *
     * The device is obtained from the specified action environment.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return true if the register contained the expected value, otherwise
     *         returns false.
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
     * Returns the expected byte value.
     *
     * @return expected value
     */
    uint8_t getValue() const
    {
        return value;
    }

    /**
     * Returns the bit mask.
     *
     * Specifies which bits should be compared within the byte value.  Only the
     * bits with a value of 1 in the mask will be compared.
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
     * Expected byte value.
     */
    const uint8_t value{0x00};

    /**
     * Bit mask.  Specifies which bits should be compared within the byte value.
     * Only the bits with a value of 1 in the mask will be compared.
     */
    const uint8_t mask{0xFF};
};

} // namespace phosphor::power::regulators
