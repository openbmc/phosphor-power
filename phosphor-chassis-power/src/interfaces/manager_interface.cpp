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
namespace chassis
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

const sdbusplus::vtable::vtable_t ManagerInterface::_vtable[] = {
    sdbusplus::vtable::start(),
    // No configure method parameters and returns void
    sdbusplus::vtable::method("Configure", "", "", callbackConfigure),
    sdbusplus::vtable::end()};
} // namespace interface
} // namespace chassis
} // namespace power
} // namespace phosphor
