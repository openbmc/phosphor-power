#pragma once

#include "pmbus.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <string>

namespace phosphor
{
namespace power
{
namespace util
{

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto POWEROFF_TARGET = "obmc-chassis-hard-poweroff@0.target";
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
std::string getService(const std::string& path, const std::string& interface,
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
template <typename T>
void getProperty(const std::string& interface, const std::string& propertyName,
                 const std::string& path, const std::string& service,
                 sdbusplus::bus::bus& bus, T& value)
{
    sdbusplus::message::variant<T> property;

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTF, "Get");

    method.append(interface, propertyName);

    auto reply = bus.call(method);

    reply.read(property);
    value = sdbusplus::message::variant_ns::get<T>(property);
}

/**
 * @brief Write a D-Bus property
 *
 * @param[in] interface - the interface the property is on
 * @param[in] propertName - the name of the property
 * @param[in] path - the D-Bus path
 * @param[in] service - the D-Bus service
 * @param[in] bus - the D-Bus object
 * @param[in] value - the value to set the property to
 */
template <typename T>
void setProperty(const std::string& interface, const std::string& propertyName,
                 const std::string& path, const std::string& service,
                 sdbusplus::bus::bus& bus, T& value)
{
    sdbusplus::message::variant<T> propertyValue(value);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTF, "Set");

    method.append(interface, propertyName, propertyValue);

    auto reply = bus.call(method);
}

/**
 * Logs an error and powers off the system.
 *
 * @tparam T - error that will be logged before the power off
 * @param[in] bus - D-Bus object
 */
template <typename T>
void powerOff(sdbusplus::bus::bus& bus)
{
    phosphor::logging::report<T>();

    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                      SYSTEMD_INTERFACE, "StartUnit");

    method.append(POWEROFF_TARGET);
    method.append("replace");

    bus.call_noreply(method);
}

/**
 * Load json from a file
 *
 * @param[in] path - The path of the json file
 *
 * @return The nlohmann::json object
 */
nlohmann::json loadJSONFromFile(const char* path);

/**
 * Get PmBus access type from the json config
 *
 * @param[in] json - The json object
 *
 * @return The pmbus access type
 */
phosphor::pmbus::Type getPMBusAccessType(const nlohmann::json& json);

/**
 * Check if power is on
 *
 * @return true if power is on, otherwise false
 */
bool isPoweredOn(sdbusplus::bus::bus& bus);

/**
 * Get all PSU inventory paths from D-Bus
 *
 * @return The list of PSU inventory paths
 */
std::vector<std::string> getPSUInventoryPaths(sdbusplus::bus::bus& bus);

} // namespace util
} // namespace power
} // namespace phosphor
