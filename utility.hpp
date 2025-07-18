#pragma once

#include "pmbus.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <string>
#include <vector>

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
constexpr auto ENTITY_MGR_SERVICE = "xyz.openbmc_project.EntityManager";

using DbusPath = std::string;
using DbusProperty = std::string;
using DbusService = std::string;
using DbusInterface = std::string;
using DbusInterfaceList = std::vector<DbusInterface>;
using DbusSubtree =
    std::map<DbusPath, std::map<DbusService, DbusInterfaceList>>;
using DbusVariant =
    std::variant<bool, uint64_t, std::string, std::vector<uint64_t>,
                 std::vector<std::string>>;
using DbusPropertyMap = std::map<DbusProperty, DbusVariant>;
/**
 * @brief Get the service name from the mapper for the
 *        interface and path passed in.
 *
 * @param[in] path - the D-Bus path name
 * @param[in] interface - the D-Bus interface name
 * @param[in] bus - the D-Bus object
 * @param[in] logError - log error when no service found
 *
 * @return The service name
 */
std::string getService(const std::string& path, const std::string& interface,
                       sdbusplus::bus_t& bus, bool logError = true);

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
                 sdbusplus::bus_t& bus, T& value)
{
    std::variant<T> property;

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTF, "Get");

    method.append(interface, propertyName);

    auto reply = bus.call(method);

    reply.read(property);
    value = std::get<T>(property);
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
                 sdbusplus::bus_t& bus, T& value)
{
    std::variant<T> propertyValue(value);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTF, "Set");

    method.append(interface, propertyName, propertyValue);

    auto reply = bus.call(method);
}

/**
 * @brief Get all D-Bus properties
 *
 * @param[in] bus - the D-Bus object
 * @param[in] path - the D-Bus object path
 * @param[in] interface - the D-Bus interface name
 * @param[in] service - the D-Bus service name (optional)
 *
 * @return DbusPropertyMap - Map of property names and values
 */
DbusPropertyMap getAllProperties(sdbusplus::bus_t& bus, const std::string& path,
                                 const std::string& interface,
                                 const std::string& service = std::string());

/** @brief Get subtree from the object mapper.
 *
 * Helper function to find objects, services, and interfaces.
 * See:
 * https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md
 *
 * @param[in] bus - The D-Bus object.
 * @param[in] path - The root of the tree to search.
 * @param[in] interface - Interface in the subtree to search for
 * @param[in] depth - The number of path elements to descend.
 *
 * @return DbusSubtree - Map of object paths to a map of service names to their
 *                       interfaces.
 */
DbusSubtree getSubTree(sdbusplus::bus_t& bus, const std::string& path,
                       const std::string& interface, int32_t depth);

/** @brief Get subtree from the object mapper.
 *
 * Helper function to find objects, services, and interfaces.
 * See:
 * https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md
 *
 * @param[in] bus - The D-Bus object.
 * @param[in] path - The root of the tree to search.
 * @param[in] interfaces - Interfaces in the subtree to search for.
 * @param[in] depth - The number of path elements to descend.
 *
 * @return DbusSubtree - Map of object paths to a map of service names to their
 *                       interfaces.
 */
DbusSubtree getSubTree(sdbusplus::bus_t& bus, const std::string& path,
                       const std::vector<std::string>& interfaces,
                       int32_t depth);

/** @brief GetAssociatedSubTreePaths wrapper from the object mapper.
 *
 * Helper function to find object paths that implement a certain
 * interface and are also an association endpoint.
 * See:
 * https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md
 *
 * @param[in] bus - The D-Bus object.
 * @param[in] associationPath - The association it must be an endpoint of.
 * @param[in] path - The root of the tree to search.
 * @param[in] interfaces - The interfaces in the subtree to search for
 * @param[in] depth - The number of path elements to descend.
 *
 * @return std::vector<DbusPath> - The object paths.
 */
std::vector<DbusPath> getAssociatedSubTreePaths(
    sdbusplus::bus_t& bus,
    const sdbusplus::message::object_path& associationPath,
    const sdbusplus::message::object_path& path,
    const std::vector<std::string>& interfaces, int32_t depth);

/**
 * Logs an error and powers off the system.
 *
 * @tparam T - error that will be logged before the power off
 * @param[in] bus - D-Bus object
 */
template <typename T>
void powerOff(sdbusplus::bus_t& bus)
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
 * @param[in] bus - D-Bus object
 * @param[in] defaultState - The default state if the function fails to get
 *                           the power state.
 *
 * @return true if power is on, otherwise false;
 *         defaultState if it fails to get the power state.
 */
bool isPoweredOn(sdbusplus::bus_t& bus, bool defaultState = false);

/**
 * Get all PSU inventory paths from D-Bus
 *
 * @param[in] bus - D-Bus object
 *
 * @return The list of PSU inventory paths
 */
std::vector<std::string> getPSUInventoryPaths(sdbusplus::bus_t& bus);

/**
 * @detail Get all chassis inventory paths from D-Bus
 *
 * @param[in] bus - D-Bus object
 *
 * @return The list of chassis inventory paths
 */
std::vector<std::string> getChassisInventoryPaths(sdbusplus::bus_t& bus);

/**
 * @brief Retrieve the chassis ID for the given D-Bus inventory path.
 * Query the D-Bus interface for inventory object path and retrieve its unique
 * ID (ID specified by SlotNumber)
 *
 * @param[in] bus - D-Bus object
 * @param[in] path - The inventory manager D-Bus path for a chassis.
 *
 * @return uint64_t - Chassis unique ID
 */
uint64_t getChassisInventoryUniqueId(sdbusplus::bus_t& bus,
                                     const std::string& path);
/**
 * @brief Retrieve the parent chassis unique ID from the Entity Manager.
 * Given a D-Bus object path, this function extracts the parent path (board or
 * chassis) and retrieve the chassis unique ID from Entity Manager.
 *
 * @param[in] bus - D-Bus object
 * @param[in] path - The EntityManager D-Bus path for hardware within a chassis
 * or board.
 *
 * @return uint64_t - Chassis unique ID
 *
 * @note This function assumes the parent object path contains property in
 * Entity Manager.
 */
uint64_t getParentEMUniqueId(sdbusplus::bus_t& bus, const std::string& path);

} // namespace util
} // namespace power
} // namespace phosphor
