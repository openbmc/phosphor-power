/**
 * Copyright © 2021 IBM Corporation
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

#include "power_control.hpp"

#include "types.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <exception>

using namespace phosphor::logging;

namespace phosphor::power::sequencer
{

PowerControl::PowerControl(sdbusplus::bus::bus& bus,
                           const sdeventplus::Event& event) :
    bus{bus},
    eventLoop{event}, timer{event, std::bind(&PowerControl::pollPgood, this),
                            std::chrono::milliseconds(pollInterval)}
{
    // Obtain dbus service name
    bus.request_name(POWER_IFACE);
}

void PowerControl::pollPgood()
{
}

} // namespace phosphor::power::sequencer
