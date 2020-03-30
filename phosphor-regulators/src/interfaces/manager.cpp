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
namespace server
{

Manager::Manager(sdbusplus::bus::bus& bus, const char* path) :
    _serverInterface(bus, path, interface, _vtable, this)
{
}

int Manager::_callbackConfigure(sd_bus_message* msg, void* context,
                                sd_bus_error* error)
{
    try
    {
        auto m = sdbusplus::message::message(msg);

        auto mgrObj = static_cast<Manager*>(context);
        mgrObj->configure();

        auto reply = m.new_method_return();

        reply.method_return();
    }
    catch (sdbusplus::internal_exception_t& e)
    {
        return sd_bus_error_set(error, e.name(), e.description());
    }

    return 1;
}

namespace details::Manager
{
static const auto _param_Configure =
    sdbusplus::utility::tuple_to_array(std::make_tuple('\0'));
static const auto _return_Configure =
    sdbusplus::utility::tuple_to_array(std::make_tuple('\0'));
} // namespace details::Manager

int Manager::_callbackMonitor(sd_bus_message* msg, void* context,
                              sd_bus_error* error)
{
    try
    {
        bool enable{};
        auto m = sdbusplus::message::message(msg);

        m.read(enable);

        auto mgrObj = static_cast<Manager*>(context);
        mgrObj->monitor(enable);

        auto reply = m.new_method_return();

        reply.method_return();
    }
    catch (sdbusplus::internal_exception_t& e)
    {
        return sd_bus_error_set(error, e.name(), e.description());
    }

    return 1;
}

namespace details::Manager
{
static const auto _param_Monitor = sdbusplus::utility::tuple_to_array(
    sdbusplus::message::types::type_id<bool>());
static const auto _return_Monitor =
    sdbusplus::utility::tuple_to_array(std::make_tuple('\0'));
} // namespace details::Manager

const sdbusplus::vtable::vtable_t Manager::_vtable[] = {
    sdbusplus::vtable::start(),
    sdbusplus::vtable::method(
        "Configure", details::Manager::_param_Configure.data(),
        details::Manager::_return_Configure.data(), _callbackConfigure),
    sdbusplus::vtable::method(
        "Monitor", details::Manager::_param_Monitor.data(),
        details::Manager::_return_Monitor.data(), _callbackMonitor),
    sdbusplus::vtable::end()};

} // namespace server
} // namespace regulators
} // namespace power
} // namespace phosphor
