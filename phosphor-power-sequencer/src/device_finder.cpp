/**
 * Copyright © 2024 IBM Corporation
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

#include "device_finder.hpp"

#include <phosphor-logging/lg2.hpp>

#include <exception>
#include <format>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace phosphor::power::sequencer
{

const std::string deviceInterfacesService = "xyz.openbmc_project.EntityManager";

const std::vector<std::string> deviceInterfaces{
    "xyz.openbmc_project.Configuration.UCD90160",
    "xyz.openbmc_project.Configuration.UCD90320"};

const std::string typeProperty = "Type";
const std::string nameProperty = "Name";
const std::string busProperty = "Bus";
const std::string addressProperty = "Address";

DeviceFinder::DeviceFinder(sdbusplus::bus_t& bus, Callback callback) :
    callback{std::move(callback)},
    interfacesFinder{
        bus, deviceInterfacesService, deviceInterfaces,
        std::bind_front(&DeviceFinder::interfaceFoundCallback, this)}
{}

void DeviceFinder::interfaceFoundCallback(
    [[maybe_unused]] const std::string& path, const std::string& interface,
    const DbusPropertyMap& properties)
{
    try
    {
        DeviceProperties device{};

        auto value = getPropertyValue(properties, typeProperty);
        device.type = std::get<std::string>(value);

        value = getPropertyValue(properties, nameProperty);
        device.name = std::get<std::string>(value);

        value = getPropertyValue(properties, busProperty);
        device.bus = static_cast<uint8_t>(std::get<uint64_t>(value));

        value = getPropertyValue(properties, addressProperty);
        device.address = static_cast<uint16_t>(std::get<uint64_t>(value));

        callback(device);
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Unable to obtain properties of interface {INTERFACE}: {ERROR}",
            "INTERFACE", interface, "ERROR", e);
    }
}

const DbusVariant&
    DeviceFinder::getPropertyValue(const DbusPropertyMap& properties,
                                   const std::string& propertyName)
{
    auto it = properties.find(propertyName);
    if (it == properties.end())
    {
        throw std::runtime_error{
            std::format("{} property not found", propertyName)};
    }
    return it->second;
}

} // namespace phosphor::power::sequencer
