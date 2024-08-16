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
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class I2CCompareBytesAction
 *
 * Compares device register bytes to a list of expected values.  Communicates
 * with the device directly using the I2C interface.
 *
 * Implements the i2c_compare_bytes action in the JSON config file.
 */
class I2CCompareBytesAction : public I2CAction
{
  public:
    // Specify which compiler-generated methods we want
    I2CCompareBytesAction() = delete;
    I2CCompareBytesAction(const I2CCompareBytesAction&) = delete;
    I2CCompareBytesAction(I2CCompareBytesAction&&) = delete;
    I2CCompareBytesAction& operator=(const I2CCompareBytesAction&) = delete;
    I2CCompareBytesAction& operator=(I2CCompareBytesAction&&) = delete;
    virtual ~I2CCompareBytesAction() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param reg Device register address.  Note: named 'reg' because 'register'
     *            is a reserved keyword.
     * @param values One or more expected byte values.  The bytes must be
     *               specified in the same order as they will be received from
     *               the device (e.g. in little-endian order).
     */
    explicit I2CCompareBytesAction(uint8_t reg,
                                   const std::vector<uint8_t>& values) :
        I2CCompareBytesAction(reg, values,
                              std::vector<uint8_t>(values.size(), 0xFF))
    {}

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param reg Device register address.  Note: named 'reg' because 'register'
     *            is a reserved keyword.
     * @param values One or more expected byte values.  The bytes must be
     *               specified in the same order as they will be received from
     *               the device (e.g. in little-endian order).
     * @param masks One or more bit masks.  The number of bit masks must match
     *              the number of expected byte values.  Each mask specifies
     *              which bits should be compared within the corresponding byte
     *              value.  Only the bits with a value of 1 in the mask will be
     *              compared.
     */
    explicit I2CCompareBytesAction(uint8_t reg,
                                   const std::vector<uint8_t>& values,
                                   const std::vector<uint8_t>& masks) :
        reg{reg}, values{values}, masks{masks}
    {
        // Values vector must not be empty
        if (values.size() < 1)
        {
            throw std::invalid_argument{"Values vector is empty"};
        }

        // Masks vector must have same size as values vector
        if (masks.size() != values.size())
        {
            throw std::invalid_argument{"Masks vector has invalid size"};
        }
    }

    /**
     * Executes this action.
     *
     * Compares device register bytes to a list of expected values using the
     * I2C interface.
     *
     * All of the bytes will be read in a single I2C operation.
     *
     * The device register, byte values, and bit masks (if any) were specified
     * in the constructor.
     *
     * The device is obtained from the specified action environment.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return true if the register bytes contained the expected values,
     *         otherwise returns false.
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
     * Returns the expected byte values.
     *
     * @return expected values
     */
    const std::vector<uint8_t>& getValues() const
    {
        return values;
    }

    /**
     * Returns the bit masks.
     *
     * Each mask specifies which bits should be compared within the
     * corresponding byte value.  Only the bits with a value of 1 in the mask
     * will be compared.
     *
     * @return bit masks
     */
    const std::vector<uint8_t>& getMasks() const
    {
        return masks;
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
     * Expected byte values.
     */
    const std::vector<uint8_t> values{};

    /**
     * Bit masks.  Each mask specifies which bits should be compared within the
     * corresponding byte value.  Only the bits with a value of 1 in the mask
     * will be compared.
     */
    const std::vector<uint8_t> masks{};
};

} // namespace phosphor::power::regulators
