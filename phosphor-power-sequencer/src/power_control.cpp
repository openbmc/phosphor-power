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
    bus(bus),
    eventLoop{event}
{

    timer = std::make_unique<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
        event, std::bind(&PowerControl::pollPgood, this),
        std::chrono::milliseconds(pollInterval));
}

void PowerControl::pollPgood()
{
}

} // namespace phosphor::power::sequencer

int main()
{
    using namespace phosphor::power::sequencer;

    try
    {
        log<phosphor::logging::level::INFO>(
            "Start phosphor power control service...");

        auto bus = sdbusplus::bus::new_default();
        auto event = sdeventplus::Event::get_default();
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

        PowerControl control = PowerControl(bus, event);
        return event.loop();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(e.what());
        return -2;
    }
    catch (...)
    {
        log<level::ERR>("Caught unexpected exception type");
        return -3;
    }
}
