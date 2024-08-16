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

#include "compatible_system_types_finder.hpp"

#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <exception>
#include <format>
#include <ranges>
#include <regex>
#include <stdexcept>
#include <utility>
#include <variant>

namespace phosphor::power::util
{

const std::string compatibleInterfaceService =
    "xyz.openbmc_project.EntityManager";

const std::string compatibleInterface =
    "xyz.openbmc_project.Inventory.Decorator.Compatible";

const std::string namesProperty = "Names";

CompatibleSystemTypesFinder::CompatibleSystemTypesFinder(sdbusplus::bus_t& bus,
                                                         Callback callback) :
    callback{std::move(callback)}
{
    interfaceFinder = std::make_unique<DBusInterfacesFinder>(
        bus, compatibleInterfaceService,
        std::vector<std::string>{compatibleInterface},
        std::bind_front(&CompatibleSystemTypesFinder::interfaceFoundCallback,
                        this));
}

void CompatibleSystemTypesFinder::interfaceFoundCallback(
    [[maybe_unused]] const std::string& path,
    [[maybe_unused]] const std::string& interface,
    const DbusPropertyMap& properties)
{
    try
    {
        // Get the property containing the list of compatible names
        auto it = properties.find(namesProperty);
        if (it == properties.end())
        {
            throw std::runtime_error{
                std::format("{} property not found", namesProperty)};
        }
        auto names = std::get<std::vector<std::string>>(it->second);
        if (!names.empty())
        {
            // If all the compatible names are system or chassis types
            std::regex pattern{"\\.(system|chassis)\\.", std::regex::icase};
            if (std::ranges::all_of(names, [&pattern](auto&& name) {
                    return std::regex_search(name, pattern);
                }))
            {
                // Call callback with compatible system type names
                callback(names);
            }
        }
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Unable to obtain properties of Compatible interface: {ERROR}",
            "ERROR", e);
    }
}

} // namespace phosphor::power::util
