#pragma once

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <string>

namespace witherspoon
{
namespace power
{
namespace util
{

constexpr auto PROPERTY_INTF = "org.freedesktop.DBus.Properties";

/**
 * @brief Get the service name from the mapper for the
 *        interface and path passed in.
 *
 * @param[in] path - the D-Bus path name
 * @param[in] interface - the D-Bus interface name
 * @param[in] bus - the D-Bus object
 *
 * @return The service name
 */
std::string getService(const std::string& path,
                       const std::string& interface,
                       sdbusplus::bus::bus& bus);

/**
 * @brief Read a D-Bus property
 *
 * @param[in] interface - the interface the property is on
 * @param[in] propertName - the name of the property
 * @param[in] path - the D-Bus path
 * @param[in] service - the D-Bus service
 * @param[in] bus - the D-Bus object
 * @param[out] value - filled in with the property value
 */
template<typename T>
void getProperty(const std::string& interface,
                 const std::string& propertyName,
                 const std::string& path,
                 const std::string& service,
                 sdbusplus::bus::bus& bus,
                 T& value)
{
    sdbusplus::message::variant<T> property;

    auto method = bus.new_method_call(service.c_str(),
            path.c_str(),
            PROPERTY_INTF,
            "Get");

    method.append(interface, propertyName);

    auto reply = bus.call(method);
    if (reply.is_method_error())
    {
        using namespace phosphor::logging;
        log<level::ERR>("Error in property get call",
                entry("PATH=%s", path.c_str()),
                entry("PROPERTY=%s", propertyName.c_str()));
        //
        // TODO openbmc/openbmc#851 - Once available, throw returned error
        throw std::runtime_error("Error in property get call");
    }

    reply.read(property);
    value = sdbusplus::message::variant_ns::get<T>(property);
}

/**
 * Powers off the system and logs an error
 * saying it was due to a power fault.
 *
 * @param[in] bus - D-Bus object
 */
void powerOff(sdbusplus::bus::bus& bus);

}
}
}
