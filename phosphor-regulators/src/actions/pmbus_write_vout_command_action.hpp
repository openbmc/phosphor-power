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
#include "i2c_interface.hpp"
#include "pmbus_utils.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class PMBusWriteVoutCommandAction
 *
 * Writes the value of VOUT_COMMAND to set the output voltage of a PMBus
 * regulator rail.  Communicates with the device directly using the I2C
 * interface.
 *
 * Implements the pmbus_write_vout_command action in the JSON config file.
 *
 * The volts value to write can be specified in the constructor.  Otherwise, the
 * volts value will be obtained from the ActionEnvironment.
 *
 * The PMBus specification defines four data formats for the value of
 * VOUT_COMMAND:
 * - Linear
 * - VID
 * - Direct
 * - IEEE Half-Precision Floating Point
 * Currently only the linear data format is supported.  The volts value is
 * converted into linear format before being written.
 *
 * The linear data format requires an exponent value.  The exponent value can be
 * specified in the constructor.  Otherwise the exponent value will be obtained
 * from the PMBus VOUT_MODE command.  Note that some PMBus devices do not
 * support the VOUT_MODE command.  The exponent value for a device is often
 * found in the device documentation (data sheet).
 *
 * If desired, write verification can be performed.  The value of VOUT_COMMAND
 * will be read from the device after it is written to ensure that it contains
 * the expected value.  If VOUT_COMMAND contains an unexpected value, a
 * WriteVerificationError is thrown.  To perform verification, the device must
 * return all 16 bits of voltage data that were written to VOUT_COMMAND.
 */
class PMBusWriteVoutCommandAction : public I2CAction
{
  public:
    // Specify which compiler-generated methods we want
    PMBusWriteVoutCommandAction() = delete;
    PMBusWriteVoutCommandAction(const PMBusWriteVoutCommandAction&) = delete;
    PMBusWriteVoutCommandAction(PMBusWriteVoutCommandAction&&) = delete;
    PMBusWriteVoutCommandAction& operator=(const PMBusWriteVoutCommandAction&) =
        delete;
    PMBusWriteVoutCommandAction& operator=(PMBusWriteVoutCommandAction&&) =
        delete;
    virtual ~PMBusWriteVoutCommandAction() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param volts Optional volts value to write to VOUT_COMMAND.
     * @param format Data format of the volts value written to VOUT_COMMAND.
     *               Currently the only supported format is linear.
     * @param exponent Optional exponent to use to convert the volts value to
     *                 linear data format.
     * @param isVerified Specifies whether the updated value of VOUT_COMMAND is
     *                   verified by reading it from the device.
     */
    explicit PMBusWriteVoutCommandAction(
        std::optional<double> volts, pmbus_utils::VoutDataFormat format,
        std::optional<int8_t> exponent, bool isVerified) :
        volts{volts}, format{format}, exponent{exponent},
        isWriteVerified{isVerified}
    {
        // Currently only linear format is supported
        if (format != pmbus_utils::VoutDataFormat::linear)
        {
            throw std::invalid_argument{"Unsupported data format specified"};
        }
    }

    /**
     * Executes this action.
     *
     * Writes a volts value to VOUT_COMMAND using the I2C interface.
     *
     * If a volts value was specified in the constructor, that value will be
     * used.  Otherwise the volts value will be obtained from the
     * ActionEnvironment.
     *
     * The data format is specified in the constructor.  Currently only the
     * linear format is supported.
     *
     * An exponent value is required to convert the volts value to linear
     * format.  If an exponent value was specified in the constructor, that
     * value will be used.  Otherwise the exponent value will be obtained from
     * the VOUT_MODE command.
     *
     * Write verification will be performed if specified in the constructor.
     *
     * The device is obtained from the ActionEnvironment.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return true
     */
    virtual bool execute(ActionEnvironment& environment) override;

    /**
     * Returns the optional exponent value used to convert the volts value to
     * linear data format.
     *
     * @return optional exponent value
     */
    std::optional<int8_t> getExponent() const
    {
        return exponent;
    }

    /**
     * Returns the data format of the value written to VOUT_COMMAND.
     *
     * @return data format
     */
    pmbus_utils::VoutDataFormat getFormat() const
    {
        return format;
    }

    /**
     * Returns the optional volts value to write to VOUT_COMMAND.
     *
     * @return optional volts value
     */
    std::optional<double> getVolts() const
    {
        return volts;
    }

    /**
     * Returns whether write verification will be performed when writing to
     * VOUT_COMMAND.
     *
     * @return true if write verification will be performed, false otherwise
     */
    bool isVerified() const
    {
        return isWriteVerified;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override;

  private:
    /**
     * Gets the exponent value to use to convert the volts value to linear data
     * format.
     *
     * If an exponent value is defined for this action, that value is returned.
     * Otherwise VOUT_MODE is read from the current device to obtain the
     * exponent value.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @param interface I2C interface to the current device
     * @return exponent value
     */
    int8_t getExponentValue(ActionEnvironment& environment,
                            i2c::I2CInterface& interface);

    /**
     * Gets the volts value to write to VOUT_COMMAND.
     *
     * If a volts value is defined for this action, that value is returned.
     * Otherwise the volts value is obtained from the specified
     * ActionEnvironment.
     *
     * Throws an exception if no volts value is defined.
     *
     * @param environment action execution environment
     * @return volts value
     */
    double getVoltsValue(ActionEnvironment& environment);

    /**
     * Verifies the value written to VOUT_COMMAND.  Reads the current value of
     * VOUT_COMMAND and ensures that it matches the value written.
     *
     * Throws an exception if the values do not match or a communication error
     * occurs.
     *
     * @param environment action execution environment
     * @param interface I2C interface to the current device
     * @param valueWritten linear format volts value written to VOUT_COMMAND
     */
    void verifyWrite(ActionEnvironment& environment,
                     i2c::I2CInterface& interface, uint16_t valueWritten);

    /**
     * Optional volts value to write.
     */
    const std::optional<double> volts{};

    /**
     * Data format of the volts value written to VOUT_COMMAND.
     */
    const pmbus_utils::VoutDataFormat format{};

    /**
     * Optional exponent value to use to convert the volts value to linear data
     * format.
     */
    const std::optional<int8_t> exponent{};

    /**
     * Indicates whether write verification will be performed when writing to
     * VOUT_COMMAND.
     */
    const bool isWriteVerified{false};
};

} // namespace phosphor::power::regulators
