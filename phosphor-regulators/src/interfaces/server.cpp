#include <algorithm>
#include <map>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <string>
#include <tuple>
#include <variant>

#include <xyz/openbmc_project/Power/Regulators/Manager/server.hpp>



namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Power
{
namespace Regulators
{
namespace server
{

Manager::Manager(bus::bus& bus, const char* path)
        : _xyz_openbmc_project_Power_Regulators_Manager_interface(
                bus, path, interface, _vtable, this), _intf(bus.getInterface())
{
}


int Manager::_callback_Configure(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    try
    {
        auto m = message::message(msg);
#if 1
        {
            auto tbus = m.get_bus();
            sdbusplus::server::transaction::Transaction t(tbus, m);
            sdbusplus::server::transaction::set_id
                (std::hash<sdbusplus::server::transaction::Transaction>{}(t));
        }
#endif


        auto o = static_cast<Manager*>(context);
        o->configure();

        auto reply = m.new_method_return();
        // No data to append on reply.

        reply.method_return();
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        return sd_bus_error_set(error, e.name(), e.description());
    }

    return true;
}

namespace details
{
namespace Manager
{
static const auto _param_Configure =
        utility::tuple_to_array(std::make_tuple('\0'));
static const auto _return_Configure =
        utility::tuple_to_array(std::make_tuple('\0'));
}
}

int Manager::_callback_Monitor(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    try
    {
        auto m = message::message(msg);
#if 1
        {
            auto tbus = m.get_bus();
            sdbusplus::server::transaction::Transaction t(tbus, m);
            sdbusplus::server::transaction::set_id
                (std::hash<sdbusplus::server::transaction::Transaction>{}(t));
        }
#endif

        bool enable{};

        m.read(enable);

        auto o = static_cast<Manager*>(context);
        o->monitor(enable);

        auto reply = m.new_method_return();
        // No data to append on reply.

        reply.method_return();
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        return sd_bus_error_set(error, e.name(), e.description());
    }

    return true;
}

namespace details
{
namespace Manager
{
static const auto _param_Monitor =
        utility::tuple_to_array(message::types::type_id<
                bool>());
static const auto _return_Monitor =
        utility::tuple_to_array(std::make_tuple('\0'));
}
}




const vtable::vtable_t Manager::_vtable[] = {
    vtable::start(),

    vtable::method("Configure",
                   details::Manager::_param_Configure
                        .data(),
                   details::Manager::_return_Configure
                        .data(),
                   _callback_Configure),

    vtable::method("Monitor",
                   details::Manager::_param_Monitor
                        .data(),
                   details::Manager::_return_Monitor
                        .data(),
                   _callback_Monitor),
    vtable::end()
};

} // namespace server
} // namespace Regulators
} // namespace Power
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus
