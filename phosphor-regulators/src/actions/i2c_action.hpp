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

#include "action.hpp"
#include "action_environment.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"

namespace phosphor::power::regulators
{

/**
 * @class I2CAction
 *
 * Abstract base class for actions that communicate with a device using an I2C
 * interface.
 */
class I2CAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    I2CAction() = default;
    I2CAction(const I2CAction&) = delete;
    I2CAction(I2CAction&&) = delete;
    I2CAction& operator=(const I2CAction&) = delete;
    I2CAction& operator=(I2CAction&&) = delete;
    virtual ~I2CAction() = default;

  protected:
    /**
     * Returns the I2C interface to the current device within the specified
     * action environment.
     *
     * Opens the interface if it was not already open.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return I2C interface to current device
     */
    i2c::I2CInterface& getI2CInterface(ActionEnvironment& environment)
    {
        // Get current device from action environment
        Device& device = environment.getDevice();

        // Get I2C interface from device
        i2c::I2CInterface& interface = device.getI2CInterface();

        // Open interface if necessary
        if (!interface.isOpen())
        {
            interface.open();
        }

        return interface;
    }
};

} // namespace phosphor::power::regulators
