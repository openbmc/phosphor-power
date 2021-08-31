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

#include <fmt/format.h>

#include <gpiod.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <exception>

using namespace phosphor::logging;

namespace phosphor::power::sequencer
{

const std::string powerConrolLineName = "power-chassis-control";
const std::string pgoodLineName = "power-chassis-good";

PowerControl::PowerControl(sdbusplus::bus::bus& bus,
                           const sdeventplus::Event& event) :
    PowerObject{bus, POWER_OBJ_PATH, true},
    bus{bus}, timer{event, std::bind(&PowerControl::pollPgood, this),
                    pollInterval}
{
    // Obtain dbus service name
    bus.request_name(POWER_IFACE);
    setUpGpio();
}

int PowerControl::getPgood() const
{
    return pgood;
}

int PowerControl::getPgoodTimeout() const
{
    return timeout.count();
}

int PowerControl::getState() const
{
    return state;
}

void PowerControl::pollPgood()
{
    if (inStateTransition)
    {
        const std::chrono::time_point<std::chrono::steady_clock> now =
            std::chrono::steady_clock::now();
        if (now > pgoodTimeoutTime)
        {
            log<level::ERR>("ERROR PowerControl: Pgood poll timeout");
            inStateTransition = false;
            return;
        }
    }

    int pgoodState = pgoodLine.get_value();
    if (pgoodState != pgood)
    {
        pgood = pgoodState;
        if (pgoodState == 0)
        {
            emitPowerLostSignal();
        }
        else
        {
            emitPowerGoodSignal();
        }
        emitPropertyChangedSignal("pgood");
    }
    if (pgoodState == state)
    {
        inStateTransition = false;
    }
}

void PowerControl::setPgoodTimeout(int t)
{
    if (timeout.count() != t)
    {
        timeout = std::chrono::seconds(t);
        emitPropertyChangedSignal("pgood_timeout");
    }
}

void PowerControl::setState(int s)
{
    if (state == s)
    {
        log<level::INFO>(
            fmt::format("Power already at requested state: {}", state).c_str());
        return;
    }

    log<level::INFO>(fmt::format("setState: {}", s).c_str());
    powerContolLine.request(
        {"phosphor-power-control", gpiod::line_request::DIRECTION_OUTPUT, 0});
    powerContolLine.set_value(s);
    powerContolLine.release();

    pgoodTimeoutTime = std::chrono::steady_clock::now() + timeout;
    inStateTransition = true;
    state = s;
    emitPropertyChangedSignal("state");
}

void PowerControl::setUpGpio()
{
    pgoodLine = gpiod::find_line(pgoodLineName);
    if (!pgoodLine)
    {
        log<level::ERR>(
            fmt::format("Error finding line: {}", pgoodLineName).c_str());
    }
    powerContolLine = gpiod::find_line(powerConrolLineName);
    if (!powerContolLine)
    {
        log<level::ERR>(
            fmt::format("Error finding line: {}", powerConrolLineName).c_str());
    }

    pgoodLine.request(
        {"phosphor-power-control", gpiod::line_request::DIRECTION_INPUT, 0});
    int pgoodState = pgoodLine.get_value();
    pgood = pgoodState;
    state = pgoodState;
    log<level::INFO>(fmt::format("Pgood state: {}", pgoodState).c_str());
}

} // namespace phosphor::power::sequencer
