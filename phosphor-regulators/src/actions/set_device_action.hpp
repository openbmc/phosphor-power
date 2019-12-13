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

#include "action.hpp"
#include "action_environment.hpp"

#include <string>

namespace phosphor::power::regulators
{

/**
 * @class SetDeviceAction
 *
 * Sets the device that will be used by subsequent actions.
 *
 * Implements the set_device action in the JSON config file.
 */
class SetDeviceAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    SetDeviceAction() = delete;
    SetDeviceAction(const SetDeviceAction&) = delete;
    SetDeviceAction(SetDeviceAction&&) = delete;
    SetDeviceAction& operator=(const SetDeviceAction&) = delete;
    SetDeviceAction& operator=(SetDeviceAction&&) = delete;
    virtual ~SetDeviceAction() = default;

    /**
     * Constructor.
     *
     * @param deviceID device ID
     */
    explicit SetDeviceAction(const std::string& deviceID) : deviceID{deviceID}
    {}

    /**
     * Executes this action.
     *
     * Sets the current device ID in the ActionEnvironment.  This causes
     * subsequent actions to use that device.
     *
     * @param environment Action execution environment.
     * @return true
     */
    virtual bool execute(ActionEnvironment& environment) override
    {
        environment.setDeviceID(deviceID);
        return true;
    }

    /**
     * Returns the device ID.
     *
     * @return device ID
     */
    const std::string& getDeviceID() const
    {
        return deviceID;
    }

  private:
    /**
     * Device ID.
     */
    const std::string deviceID{};
};

} // namespace phosphor::power::regulators
