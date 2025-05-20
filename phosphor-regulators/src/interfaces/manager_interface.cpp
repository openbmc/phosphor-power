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

#include "manager_interface.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>

#include <string>
#include <tuple>

namespace phosphor
{
namespace power
{
namespace regulators
{
namespace interface
{

ManagerInterface::ManagerInterface(sdbusplus::bus_t& bus, const char* path) :
    _serverInterface(bus, path, interface, _vtable, this)
{}

int ManagerInterface::callbackConfigure(sd_bus_message* msg, void* context,
                                        sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            auto m = sdbusplus::message_t(msg);

            auto mgrObj = static_cast<ManagerInterface*>(context);
            mgrObj->configure();

            auto reply = m.new_method_return();

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
        lg2::error("Unable to service Configure method callback");
        return -1;
    }

    return 1;
}

int ManagerInterface::callbackMonitor(sd_bus_message* msg, void* context,
                                      sd_bus_error* error)
{
    if (msg != nullptr && context != nullptr)
    {
        try
        {
            bool enable{};
            auto m = sdbusplus::message_t(msg);

            m.read(enable);

            auto mgrObj = static_cast<ManagerInterface*>(context);
            mgrObj->monitor(enable);

            auto reply = m.new_method_return();

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
        lg2::error("Unable to service Monitor method callback");
        return -1;
    }

    return 1;
}

const sdbusplus::vtable::vtable_t ManagerInterface::_vtable[] = {
    sdbusplus::vtable::start(),
    // No configure method parameters and returns void
    sdbusplus::vtable::method("Configure", "", "", callbackConfigure),
    // Monitor method takes a boolean parameter and returns void
    sdbusplus::vtable::method("Monitor", "b", "", callbackMonitor),
    sdbusplus::vtable::end()};

} // namespace interface
} // namespace regulators
} // namespace power
} // namespace phosphor
