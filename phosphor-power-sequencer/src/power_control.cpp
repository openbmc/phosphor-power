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
#include <sys/types.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <exception>
#include <string>

using namespace phosphor::logging;

namespace phosphor::power::sequencer
{

const std::string powerControlLineName = "power-chassis-control";
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
        const auto now = std::chrono::steady_clock::now();
        if (now > pgoodTimeoutTime)
        {
            log<level::ERR>("ERROR PowerControl: Pgood poll timeout");
            inStateTransition = false;

            try
            {
                auto method = bus.new_method_call(
                    "xyz.openbmc_project.Logging",
                    "/xyz/openbmc_project/logging",
                    "xyz.openbmc_project.Logging.Create", "Create");

                std::map<std::string, std::string> additionalData;
                // Add PID to AdditionalData
                additionalData.emplace("_PID", std::to_string(getpid()));

                method.append(
                    state ? "xyz.openbmc_project.Power.Error.PowerOnTimeout"
                          : "xyz.openbmc_project.Power.Error.PowerOffTimeout",
                    sdbusplus::xyz::openbmc_project::Logging::server::Entry::
                        Level::Critical,
                    additionalData);
                bus.call_noreply(method);
            }
            catch (const std::exception& e)
            {
                log<level::ERR>(
                    fmt::format("Error logging timeout, state: {}, error {}",
                                state, e.what())
                        .c_str());
            }

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
    if (s == 0)
    {
        // Wait for two seconds when powering down. This is to allow host and
        // other BMC applications time to complete power off processing
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    log<level::INFO>(fmt::format("setState: {}", s).c_str());
    powerControlLine.request(
        {"phosphor-power-control", gpiod::line_request::DIRECTION_OUTPUT, 0});
    powerControlLine.set_value(s);
    powerControlLine.release();

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
        std::string errorString{"GPIO line name not found: " + pgoodLineName};
        log<level::ERR>(errorString.c_str());
        report<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
        throw std::runtime_error(errorString);
    }
    powerControlLine = gpiod::find_line(powerControlLineName);
    if (!powerControlLine)
    {
        std::string errorString{"GPIO line name not found: " +
                                powerControlLineName};
        log<level::ERR>(errorString.c_str());
        report<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
        throw std::runtime_error(errorString);
    }

    pgoodLine.request(
        {"phosphor-power-control", gpiod::line_request::DIRECTION_INPUT, 0});
    int pgoodState = pgoodLine.get_value();
    pgood = pgoodState;
    state = pgoodState;
    log<level::INFO>(fmt::format("Pgood state: {}", pgoodState).c_str());
}

} // namespace phosphor::power::sequencer
