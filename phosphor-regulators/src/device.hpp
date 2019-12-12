/**
 * Copyright © 2019 IBM Corporation
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

#include "i2c_interface.hpp"

#include <memory>
#include <string>
#include <utility>

namespace phosphor::power::regulators
{

/**
 * @class Device
 *
 * A hardware device, such as a voltage regulator or I/O expander.
 */
class Device
{
  public:
    // Specify which compiler-generated methods we want
    Device() = delete;
    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;
    ~Device() = default;

    /**
     * Constructor.
     *
     * @param id unique device ID
     * @param isRegulator indicates whether this device is a voltage regulator
     * @param fru Field-Replaceable Unit (FRU) for this device
     * @param i2cInterface I2C interface to this device
     */
    explicit Device(const std::string& id, bool isRegulator,
                    const std::string& fru,
                    std::unique_ptr<i2c::I2CInterface> i2cInterface) :
        id{id},
        isRegulatorDevice{isRegulator}, fru{fru}, i2cInterface{
                                                      std::move(i2cInterface)}
    {
    }

    /**
     * Returns the unique ID of this device.
     *
     * @return device ID
     */
    const std::string& getID() const
    {
        return id;
    }

    /**
     * Returns whether this device is a voltage regulator.
     *
     * @return true if device is a voltage regulator, false otherwise
     */
    bool isRegulator() const
    {
        return isRegulatorDevice;
    }

    /**
     * Returns the Field-Replaceable Unit (FRU) for this device.
     *
     * Returns the D-Bus inventory path of the FRU.  If the device itself is not
     * a FRU, returns the FRU that contains the device.
     *
     * @return FRU for this device
     */
    const std::string& getFRU() const
    {
        return fru;
    }

    /**
     * Gets the I2C interface to this device.
     *
     * @return I2C interface to device
     */
    i2c::I2CInterface& getI2CInterface()
    {
        return *i2cInterface;
    }

  private:
    /**
     * Unique ID of this device.
     */
    const std::string id{};

    /**
     * Indicates whether this device is a voltage regulator.
     */
    const bool isRegulatorDevice{false};

    /**
     * Field-Replaceable Unit (FRU) for this device.
     *
     * Set to the D-Bus inventory path of the FRU.  If the device itself is not
     * a FRU, set to the FRU that contains the device.
     */
    const std::string fru{};

    /**
     * I2C interface to this device.
     */
    std::unique_ptr<i2c::I2CInterface> i2cInterface{};
};

} // namespace phosphor::power::regulators
