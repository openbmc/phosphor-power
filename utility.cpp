/**
 * Copyright © 2017 IBM Corporation
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
#include "utility.hpp"

#include "types.hpp"

#include <fstream>

namespace phosphor
{
namespace power
{
namespace util
{

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

using namespace phosphor::logging;
using json = nlohmann::json;

std::string getService(const std::string& path, const std::string& interface,
                       sdbusplus::bus_t& bus, bool logError)
{
    auto method = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetObject");

    method.append(path);
    method.append(std::vector<std::string>({interface}));

    auto reply = bus.call(method);

    std::map<std::string, std::vector<std::string>> response;
    reply.read(response);

    if (response.empty())
    {
        if (logError)
        {
            log<level::ERR>(
                std::string("Error in mapper response for getting service name "
                            "PATH=" +
                            path + " INTERFACE=" + interface)
                    .c_str());
        }
        return std::string{};
    }

    return response.begin()->first;
}

DbusPropertyMap getAllProperties(sdbusplus::bus_t& bus, const std::string& path,
                                 const std::string& interface,
                                 const std::string& service)
{
    DbusPropertyMap properties;

    auto serviceStr = service;
    if (serviceStr.empty())
    {
        serviceStr = getService(path, interface, bus);
        if (serviceStr.empty())
        {
            return properties;
        }
    }

    auto method = bus.new_method_call(serviceStr.c_str(), path.c_str(),
                                      PROPERTY_INTF, "GetAll");
    method.append(interface);
    auto reply = bus.call(method);
    reply.read(properties);
    return properties;
}

DbusSubtree getSubTree(sdbusplus::bus_t& bus, const std::string& path,
                       const std::string& interface, int32_t depth)
{
    return getSubTree(bus, path, std::vector<std::string>({interface}), depth);
}

DbusSubtree getSubTree(sdbusplus::bus_t& bus, const std::string& path,
                       const std::vector<std::string>& interfaces,
                       int32_t depth)
{
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                          MAPPER_INTERFACE, "GetSubTree");
    mapperCall.append(path);
    mapperCall.append(depth);
    mapperCall.append(interfaces);

    auto reply = bus.call(mapperCall);

    DbusSubtree response;
    reply.read(response);
    return response;
}

std::vector<DbusPath> getAssociatedSubTreePaths(
    sdbusplus::bus_t& bus,
    const sdbusplus::message::object_path& associationPath,
    const sdbusplus::message::object_path& path,
    const std::vector<std::string>& interfaces, int32_t depth)
{
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                          MAPPER_INTERFACE,
                                          "GetAssociatedSubTreePaths");
    mapperCall.append(associationPath);
    mapperCall.append(path);
    mapperCall.append(depth);
    mapperCall.append(interfaces);

    auto reply = bus.call(mapperCall);

    std::vector<DbusPath> response;
    reply.read(response);
    return response;
}

json loadJSONFromFile(const char* path)
{
    std::ifstream ifs(path);
    if (!ifs.good())
    {
        log<level::ERR>(std::string("Unable to open file "
                                    "PATH=" +
                                    std::string(path))
                            .c_str());
        return nullptr;
    }
    auto data = json::parse(ifs, nullptr, false);
    if (data.is_discarded())
    {
        log<level::ERR>(std::string("Failed to parse json "
                                    "PATH=" +
                                    std::string(path))
                            .c_str());
        return nullptr;
    }
    return data;
}

phosphor::pmbus::Type getPMBusAccessType(const json& json)
{
    using namespace phosphor::pmbus;
    Type type;

    auto typeStr = json.at("inventoryPMBusAccessType");

    if (typeStr == "Hwmon")
    {
        type = Type::Hwmon;
    }
    else if (typeStr == "DeviceDebug")
    {
        type = Type::DeviceDebug;
    }
    else if (typeStr == "Debug")
    {
        type = Type::Debug;
    }
    else if (typeStr == "HwmonDeviceDebug")
    {
        type = Type::HwmonDeviceDebug;
    }
    else
    {
        type = Type::Base;
    }
    return type;
}

bool isPoweredOn(sdbusplus::bus_t& bus, bool defaultState)
{
    int32_t state = defaultState;

    try
    {
        // When state = 1, system is powered on
        auto service = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);
        getProperty<int32_t>(POWER_IFACE, "state", POWER_OBJ_PATH, service, bus,
                             state);
    }
    catch (const std::exception& e)
    {
        log<level::INFO>("Failed to get power state.");
    }
    return state != 0;
}

std::vector<std::string> getPSUInventoryPaths(sdbusplus::bus_t& bus)
{
    std::vector<std::string> paths;
    auto method = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetSubTreePaths");
    method.append(INVENTORY_OBJ_PATH);
    method.append(0); // Depth 0 to search all
    method.append(std::vector<std::string>({PSU_INVENTORY_IFACE}));
    auto reply = bus.call(method);

    reply.read(paths);
    return paths;
}

} // namespace util
} // namespace power
} // namespace phosphor
