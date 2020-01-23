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

#include "manager.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace power
{
namespace regulators
{

Manager::Manager(sdbusplus::bus::bus& bus) :
    ManagerObject(bus, objPath, true), bus(bus)
{
    // TODO get property (IM keyword)
    // call parse json function
    // TODO subscribe to interfacesadded (IM keyword)
    // callback would call parse json function

    // Obtain dbus service name
    bus.request_name(busName);
}

void Manager::configure()
{
    // TODO Configuration errors that should halt poweron,
    // throw InternalFailure exception (or similar) to
    // fail the call(busctl) to this method
}

void Manager::monitor(bool enable)
{
    // TODO Enable or disable timer event that will do the monitoring
}

} // namespace regulators
} // namespace power
} // namespace phosphor
