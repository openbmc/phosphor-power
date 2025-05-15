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

#include "power_interface.hpp"

#include "types.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>

#include <string>
#include <tuple>

namespace phosphor::power::sequencer
{

PowerInterface::PowerInterface(sdbusplus::bus_t& bus, const char* path) :
    serverInterface(bus, path, POWER_IFACE, vtable, this)
{}

int PowerInterface::callbackGetPgood(
    sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
    const char* /*property*/, sd_bus_message* msg, void* context,
    sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            auto pwrObj = static_cast<PowerInterface*>(context);
            int pgood = pwrObj->getPgood();
            lg2::debug("callbackGetPgood: {PGOOD}", "PGOOD", pgood);

            sdbusplus::message_t(msg).append(pgood);
        }
        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
    }
    else
    {
        // The message or context were null
        lg2::error("Unable to service get pgood property callback");
        return -1;
    }

    return 1;
}

int PowerInterface::callbackGetPgoodTimeout(
    sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
    const char* /*property*/, sd_bus_message* msg, void* context,
    sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            auto pwrObj = static_cast<PowerInterface*>(context);
            int timeout = pwrObj->getPgoodTimeout();
            lg2::debug("callbackGetPgoodTimeout: {TIMEOUT}", "TIMEOUT",
                       timeout);

            sdbusplus::message_t(msg).append(timeout);
        }
        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
    }
    else
    {
        // The message or context were null
        lg2::error("Unable to service get pgood timeout property callback");
        return -1;
    }

    return 1;
}

int PowerInterface::callbackGetPowerState(sd_bus_message* msg, void* context,
                                          sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            auto pwrObj = static_cast<PowerInterface*>(context);
            // Return the current power state of the GPIO, rather than the last
            // requested power state change
            int pgood = pwrObj->getPgood();
            lg2::debug("callbackGetPowerState: {PGOOD}", "PGOOD", pgood);

            auto reply = sdbusplus::message_t(msg).new_method_return();
            reply.append(pgood);
            reply.method_return();
        }
        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
    }
    else
    {
        // The message or context were null
        lg2::error("Unable to service getPowerState method callback");
        return -1;
    }

    return 1;
}

int PowerInterface::callbackSetPgoodTimeout(
    sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
    const char* /*property*/, sd_bus_message* msg, void* context,
    sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            auto m = sdbusplus::message_t(msg);

            int timeout{};
            m.read(timeout);
            lg2::info("callbackSetPgoodTimeout: {TIMEOUT}", "TIMEOUT", timeout);

            auto pwrObj = static_cast<PowerInterface*>(context);
            pwrObj->setPgoodTimeout(timeout);
        }
        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
    }
    else
    {
        // The message or context were null
        lg2::error("Unable to service set pgood timeout property callback");
        return -1;
    }

    return 1;
}

int PowerInterface::callbackGetState(
    sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
    const char* /*property*/, sd_bus_message* msg, void* context,
    sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            auto pwrObj = static_cast<PowerInterface*>(context);
            int state = pwrObj->getState();
            lg2::debug("callbackGetState: {STATE}", "STATE", state);

            sdbusplus::message_t(msg).append(state);
        }
        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
    }
    else
    {
        // The message or context were null
        lg2::error("Unable to service get state property callback");
        return -1;
    }

    return 1;
}

int PowerInterface::callbackSetPowerState(sd_bus_message* msg, void* context,
                                          sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            auto m = sdbusplus::message_t(msg);

            int state{};
            m.read(state);

            if (state != 1 && state != 0)
            {
                return sd_bus_error_set(error,
                                        "org.openbmc.ControlPower.Error.Failed",
                                        "Invalid power state");
            }
            lg2::info("callbackSetPowerState: {STATE}", "STATE", state);

            auto pwrObj = static_cast<PowerInterface*>(context);
            pwrObj->setState(state);

            m.new_method_return().method_return();
        }
        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
    }
    else
    {
        // The message or context were null
        lg2::error("Unable to service setPowerState method callback");
        return -1;
    }

    return 1;
}

int PowerInterface::callbackSetPowerSupplyError(
    sd_bus_message* msg, void* context, sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            auto m = sdbusplus::message_t(msg);

            std::string psError{};
            m.read(psError);
            lg2::info("callbackSetPowerSupplyError: {PSERROR}", "PSERROR",
                      psError);

            auto pwrObj = static_cast<PowerInterface*>(context);
            pwrObj->setPowerSupplyError(psError);

            m.new_method_return().method_return();
        }
        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
    }
    else
    {
        // The message or context were null
        lg2::error("Unable to service setPowerSupplyError method callback");
        return -1;
    }

    return 1;
}

void PowerInterface::emitPowerGoodSignal()
{
    lg2::info("emitPowerGoodSignal");
    serverInterface.new_signal("PowerGood").signal_send();
}

void PowerInterface::emitPowerLostSignal()
{
    lg2::info("emitPowerLostSignal");
    serverInterface.new_signal("PowerLost").signal_send();
}

void PowerInterface::emitPropertyChangedSignal(const char* property)
{
    lg2::info("emitPropertyChangedSignal: {PROPERTY}", "PROPERTY", property);
    serverInterface.property_changed(property);
}

const sdbusplus::vtable::vtable_t PowerInterface::vtable[] = {
    sdbusplus::vtable::start(),
    // Method setPowerState takes an int parameter and returns void
    sdbusplus::vtable::method("setPowerState", "i", "", callbackSetPowerState),
    // Method getPowerState takes no parameters and returns int
    sdbusplus::vtable::method("getPowerState", "", "i", callbackGetPowerState),
    // Signal PowerGood
    sdbusplus::vtable::signal("PowerGood", ""),
    // Signal PowerLost
    sdbusplus::vtable::signal("PowerLost", ""),
    // Property pgood is type int, read only, and uses the emits_change flag
    sdbusplus::vtable::property("pgood", "i", callbackGetPgood,
                                sdbusplus::vtable::property_::emits_change),
    // Property state is type int, read only, and uses the emits_change flag
    sdbusplus::vtable::property("state", "i", callbackGetState,
                                sdbusplus::vtable::property_::emits_change),
    // Property pgood_timeout is type int, read write, and uses the emits_change
    // flag
    sdbusplus::vtable::property("pgood_timeout", "i", callbackGetPgoodTimeout,
                                callbackSetPgoodTimeout,
                                sdbusplus::vtable::property_::emits_change),
    // Method setPowerSupplyError takes a string parameter and returns void
    sdbusplus::vtable::method("setPowerSupplyError", "s", "",
                              callbackSetPowerSupplyError),
    sdbusplus::vtable::end()};

} // namespace phosphor::power::sequencer
