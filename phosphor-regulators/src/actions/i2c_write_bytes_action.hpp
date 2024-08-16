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
 * @class I2CWriteBytesAction
 *
 * Writes bytes to a device register.  Communicates with the device directly
 * using the I2C interface.
 *
 * Implements the i2c_write_bytes action in the JSON config file.
 */
class I2CWriteBytesAction : public I2CAction
{
  public:
    // Specify which compiler-generated methods we want
    I2CWriteBytesAction() = delete;
    I2CWriteBytesAction(const I2CWriteBytesAction&) = delete;
    I2CWriteBytesAction(I2CWriteBytesAction&&) = delete;
    I2CWriteBytesAction& operator=(const I2CWriteBytesAction&) = delete;
    I2CWriteBytesAction& operator=(I2CWriteBytesAction&&) = delete;
    virtual ~I2CWriteBytesAction() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param reg Device register address.  Note: named 'reg' because 'register'
     *            is a reserved keyword.
     * @param values One or more byte values to write.  The bytes must be
     *               specified in the order required by the device (e.g. in
     *               little-endian order).
     */
    explicit I2CWriteBytesAction(uint8_t reg,
                                 const std::vector<uint8_t>& values) :
        reg{reg}, values{values}, masks{}
    {
        // Values vector must not be empty
        if (values.size() < 1)
        {
            throw std::invalid_argument{"Values vector is empty"};
        }
    }

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param reg Device register address.  Note: named 'reg' because 'register'
     *            is a reserved keyword.
     * @param values One or more byte values to write.  The bytes must be
     *               specified in the order required by the device (e.g. in
     *               little-endian order).
     * @param masks One or more bit masks.  The number of bit masks must match
     *              the number of byte values to write.  Each mask specifies
     *              which bits to write within the corresponding byte value.
     *              Only the bits with a value of 1 in the mask will be written.
     */
    explicit I2CWriteBytesAction(uint8_t reg,
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
     * Writes bytes to a device register using the I2C interface.
     *
     * All of the bytes will be written in a single I2C operation.
     *
     * The device register, byte values, and bit masks (if any) were specified
     * in the constructor.
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
     * Returns the byte values to write.
     *
     * @return values to write
     */
    const std::vector<uint8_t>& getValues() const
    {
        return values;
    }

    /**
     * Returns the bit masks.
     *
     * Each mask specifies which bits to write within the corresponding byte
     * value.  Only the bits with a value of 1 in the mask will be written.
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
     * Byte values to write.
     */
    const std::vector<uint8_t> values{};

    /**
     * Bit masks.  Each mask specifies which bits to write within the
     * corresponding byte value.  Only the bits with a value of 1 in the mask
     * will be written.
     */
    const std::vector<uint8_t> masks{};
};

} // namespace phosphor::power::regulators
