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

#include "system_power_interface.hpp"

#include "system.hpp"
#include "types.hpp"

#include <chrono>

namespace phosphor::power::sequencer
{

SystemPowerInterface::SystemPowerInterface(
    sdbusplus::bus_t& bus, const char* path, int state, int pgood,
    int pgoodTimeout, System& system, Services& services) :
    PowerObject{bus, path, PowerObject::action::emit_no_signals},
    system{system}, services{services}
{
    setChassisNumber(0);

    // Set D-Bus properties; don't send PropertiesChanged signals
    bool skipSignal{true};
    setStateProperty(state, skipSignal);
    setPgoodProperty(pgood, skipSignal);
    setPgoodTimeoutProperty(pgoodTimeout, skipSignal);

    // Obtain D-Bus service name. This application is not considered started
    // until it has published system state/pgood and claimed bus name.
    bus.request_name(POWER_IFACE);
}

void SystemPowerInterface::setPowerState(int newState)
{
    if (newState == static_cast<int>(PowerState::off))
    {
        system.setPowerState(PowerState::off, services);
    }
    else
    {
        system.setPowerState(PowerState::on, services);
    }
}

void SystemPowerInterface::setPowerGoodTimeout(int newTimeout)
{
    // Note: Value from D-Bus is expressed in seconds
    system.setPowerGoodTimeout(std::chrono::seconds(newTimeout));
}

void SystemPowerInterface::setPowerSupplyError(const std::string& error)
{
    system.setPowerSupplyError(error);
}

} // namespace phosphor::power::sequencer
