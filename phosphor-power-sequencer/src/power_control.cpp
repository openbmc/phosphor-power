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
#include "ucd90320_monitor.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <exception>

using namespace phosphor::logging;

namespace phosphor::power::sequencer
{

const std::string interfaceName = "xyz.openbmc_project.Configuration.UCD90320";
const std::string addressPropertyName = "Address";
const std::string busPropertyName = "Bus";
const std::string namePropertyName = "Name";

PowerControl::PowerControl(sdbusplus::bus_t& bus,
                           const sdeventplus::Event& event) :
    PowerObject{bus, POWER_OBJ_PATH, PowerObject::action::defer_emit},
    bus{bus}, device{std::make_unique<PowerSequencerMonitor>(bus)},
    match{bus,
          sdbusplus::bus::match::rules::interfacesAdded() +
              sdbusplus::bus::match::rules::sender(
                  "xyz.openbmc_project.EntityManager"),
          std::bind(&PowerControl::interfacesAddedHandler, this,
                    std::placeholders::_1)},
    pgoodWaitTimer{event, std::bind(&PowerControl::onFailureCallback, this)},
    powerOnAllowedTime{std::chrono::steady_clock::now() + minimumColdStartTime},
    timer{event, std::bind(&PowerControl::pollPgood, this), pollInterval}
{
    // Obtain dbus service name
    bus.request_name(POWER_IFACE);

    setUpDevice();
    setUpGpio();
}

void PowerControl::getDeviceProperties(util::DbusPropertyMap& properties)
{
    uint64_t* i2cBus = nullptr;
    uint64_t* i2cAddress = nullptr;
    std::string* name = nullptr;

    for (const auto& property : properties)
    {
        try
        {
            if (property.first == busPropertyName)
            {
                i2cBus = std::get_if<uint64_t>(&properties[busPropertyName]);
            }
            else if (property.first == addressPropertyName)
            {
                i2cAddress =
                    std::get_if<uint64_t>(&properties[addressPropertyName]);
            }
            else if (property.first == namePropertyName)
            {
                name = std::get_if<std::string>(&properties[namePropertyName]);
            }
        }
        catch (const std::exception&)
        {}
    }

    if (i2cBus && i2cAddress && name && !name->empty())
    {
        log<level::DEBUG>(
            fmt::format(
                "Found power sequencer device properties, name: {}, bus: {} addr: {:#02x} ",
                *name, *i2cBus, *i2cAddress)
                .c_str());
        // Create device object
        device = std::make_unique<UCD90320Monitor>(bus, *i2cBus, *i2cAddress);
        deviceFound = true;
    }
}

int PowerControl::getPgood()
{
    pollPgood();
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

void PowerControl::interfacesAddedHandler(sdbusplus::message_t& msg)
{
    // Only continue if message is valid and device has not already been found
    if (!msg || deviceFound)
    {
        return;
    }

    try
    {
        // Read the dbus message
        sdbusplus::message::object_path objPath;
        std::map<std::string, std::map<std::string, util::DbusVariant>>
            interfaces;
        msg.read(objPath, interfaces);

        // Find the device interface, if present
        auto itIntf = interfaces.find(interfaceName);
        if (itIntf != interfaces.cend())
        {
            log<level::INFO>(
                fmt::format("InterfacesAdded for: {}", interfaceName).c_str());
            getDeviceProperties(itIntf->second);
        }
    }
    catch (const std::exception&)
    {
        // Error trying to read interfacesAdded message.
    }
}

void PowerControl::onFailureCallback()
{
    log<level::INFO>("After onFailure wait");

    onFailure(false);

    // Power good has failed, call for chassis hard power off
    auto method = bus.new_method_call(util::SYSTEMD_SERVICE, util::SYSTEMD_ROOT,
                                      util::SYSTEMD_INTERFACE, "StartUnit");
    method.append(util::POWEROFF_TARGET);
    method.append("replace");
    bus.call_noreply(method);
}

void PowerControl::onFailure(bool timeout)
{
    // Call device on failure
    device->onFailure(timeout, powerSupplyError);
}

void PowerControl::pollPgood()
{
    if (inStateTransition)
    {
        // In transition between power on and off, check for timeout
        const auto now = std::chrono::steady_clock::now();
        if (now > pgoodTimeoutTime)
        {
            log<level::ERR>(
                fmt::format("Power state transition timeout, state: {}", state)
                    .c_str());
            inStateTransition = false;

            if (state)
            {
                // Time out powering on
                onFailure(true);
            }
            else
            {
                // Time out powering off
                std::map<std::string, std::string> additionalData{};
                device->logError(
                    "xyz.openbmc_project.Power.Error.PowerOffTimeout",
                    additionalData);
            }

            failureFound = true;
            return;
        }
    }

    int pgoodState = pgoodLine.get_value();
    if (pgoodState != pgood)
    {
        // Power good has changed since last read
        pgood = pgoodState;
        if (pgoodState == 0)
        {
            emitPowerLostSignal();
        }
        else
        {
            emitPowerGoodSignal();
            // Clear any errors on the transition to power on
            powerSupplyError.clear();
            failureFound = false;
        }
        emitPropertyChangedSignal("pgood");
    }
    if (pgoodState == state)
    {
        // Power good matches requested state
        inStateTransition = false;
    }
    else if (!inStateTransition && (pgoodState == 0) && !failureFound)
    {
        // Not in power off state, not changing state, and power good is off
        log<level::ERR>("Chassis pgood failure");
        pgoodWaitTimer.restartOnce(std::chrono::seconds(7));
        failureFound = true;
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

void PowerControl::setPowerSupplyError(const std::string& error)
{
    powerSupplyError = error;
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
    else
    {
        // If minimum power off time has not passed, wait
        if (powerOnAllowedTime > std::chrono::steady_clock::now())
        {
            log<level::INFO>(
                fmt::format(
                    "Waiting {} seconds until power on allowed",
                    std::chrono::duration_cast<std::chrono::seconds>(
                        powerOnAllowedTime - std::chrono::steady_clock::now())
                        .count())
                    .c_str());
        }
        std::this_thread::sleep_until(powerOnAllowedTime);
    }

    log<level::INFO>(fmt::format("setState: {}", s).c_str());
    powerControlLine.request(
        {"phosphor-power-control", gpiod::line_request::DIRECTION_OUTPUT, 0});
    powerControlLine.set_value(s);
    powerControlLine.release();

    if (s == 0)
    {
        // Set a minimum amount of time to wait before next power on
        powerOnAllowedTime =
            std::chrono::steady_clock::now() + minimumPowerOffTime;
    }

    pgoodTimeoutTime = std::chrono::steady_clock::now() + timeout;
    inStateTransition = true;
    state = s;
    emitPropertyChangedSignal("state");
}

void PowerControl::setUpDevice()
{
    try
    {
        auto objects = util::getSubTree(bus, "/", interfaceName, 0);

        // Search for matching interface in returned objects
        for (const auto& [path, services] : objects)
        {
            auto service = services.begin()->first;

            if (path.empty() || service.empty())
            {
                continue;
            }

            // Get the properties for the device interface
            auto properties =
                util::getAllProperties(bus, path, interfaceName, service);

            getDeviceProperties(properties);
        }
    }
    catch (const std::exception&)
    {
        // Interface or property not found. Let the Interfaces Added callback
        // process the information once the interfaces are added to D-Bus.
    }
}

void PowerControl::setUpGpio()
{
    const std::string powerControlLineName = "power-chassis-control";
    const std::string pgoodLineName = "power-chassis-good";

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
