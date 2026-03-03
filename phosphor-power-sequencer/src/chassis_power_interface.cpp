/**
 * Copyright © 2026 IBM Corporation
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

#include "chassis_power_interface.hpp"

#include "chassis.hpp"

#include <chrono>

namespace phosphor::power::sequencer
{

ChassisPowerInterface::ChassisPowerInterface(
    sdbusplus::bus_t& bus, const char* path, int state, int pgood,
    int pgoodTimeout, Chassis& chassis) :
    PowerObject{bus, path, PowerObject::action::defer_emit}, chassis{chassis}
{
    setChassisNumber(chassis.getNumber());

    // Set D-Bus properties; don't send PropertiesChanged signals
    bool skipSignal{true};
    setStateProperty(state, skipSignal);
    setPgoodProperty(pgood, skipSignal);
    setPgoodTimeoutProperty(pgoodTimeout, skipSignal);

    // Emit D-Bus signal that object has been created
    emit_object_added();
}

void ChassisPowerInterface::setPowerState(int newState [[maybe_unused]])
{
    /* Setting the power state of an individual chassis using D-Bus is not
     * supported. The entire system needs to be powered on or off. However,
     * the BMC state management code will attempt to set the state of
     * each chassis as well as the system. To avoid causing an error in the
     * state management code, return "successfully" without doing anything.
     */
    return;
}

void ChassisPowerInterface::setPowerGoodTimeout(int newTimeout)
{
    // Note: Value from D-Bus is expressed in seconds
    chassis.setPowerGoodTimeout(std::chrono::seconds(newTimeout));
}

void ChassisPowerInterface::setPowerSupplyError(const std::string& error)
{
    chassis.setPowerSupplyError(error);
}

} // namespace phosphor::power::sequencer
