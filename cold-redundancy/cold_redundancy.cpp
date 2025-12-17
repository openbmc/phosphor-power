/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "types.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/post.hpp>
#include <boost/container/flat_set.hpp>
#include <cold_redundancy.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

namespace
{
constexpr const std::array<const char*, 1> psuInterfaceTypes = {
    "xyz.openbmc_project.Configuration.pmbus"};
std::string inventoryPath = std::string(INVENTORY_OBJ_PATH) + "/system";
const constexpr char* eventPath = "/xyz/openbmc_project/State/Decorator";
std::vector<std::unique_ptr<PowerSupply>> powerSupplies;
} // namespace

ColdRedundancy::ColdRedundancy(
    boost::asio::io_context& io,
    std::shared_ptr<sdbusplus::asio::connection>& systemBus) :
    filterTimer(io), systemBus(systemBus)
{
    post(io, [this, &systemBus]() { createPSU(systemBus); });
    std::function<void(sdbusplus::message_t&)> eventHandler =
        [this, &systemBus](sdbusplus::message_t&) {
            filterTimer.expires_after(std::chrono::seconds(1));
            filterTimer.async_wait(
                [this, &systemBus](const boost::system::error_code& ec) {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        return;
                    }
                    else if (ec)
                    {
                        std::cerr << "timer error\n";
                    }
                    createPSU(systemBus);
                });
        };

    std::function<void(sdbusplus::message_t&)> eventCollect =
        [&](sdbusplus::message_t& message) {
            std::string objectName;
            boost::container::flat_map<std::string, std::variant<bool>> values;
            std::string path = message.get_path();
            std::size_t slantingPos = path.find_last_of("/\\");
            if ((slantingPos == std::string::npos) ||
                ((slantingPos + 1) >= path.size()))
            {
                std::cerr << "Unable to get PSU state name from path\n";
                return;
            }
            std::string statePSUName = path.substr(slantingPos + 1);

            std::size_t hypenPos = statePSUName.find("_");
            if (hypenPos == std::string::npos)
            {
                std::cerr << "Unable to get PSU name from PSU path\n";
                return;
            }
            std::string psuName = statePSUName.substr(0, hypenPos);

            try
            {
                message.read(objectName, values);
            }
            catch (const sdbusplus::exception_t& e)
            {
                std::cerr << "Failed to read message from PSU Event\n";
                return;
            }

            for (auto& psu : powerSupplies)
            {
                if (psu->name != psuName)
                {
                    continue;
                }

                std::string psuEventName = "functional";
                auto findEvent = values.find(psuEventName);
                if (findEvent != values.end())
                {
                    bool* functional = std::get_if<bool>(&(findEvent->second));
                    if (functional == nullptr)
                    {
                        std::cerr << "Unable to get valid functional status\n";
                        continue;
                    }
                    if (*functional)
                    {
                        psu->state = CR::PSUState::normal;
                    }
                    else
                    {
                        psu->state = CR::PSUState::acLost;
                    }
                }
            }
        };

    using namespace sdbusplus::bus::match::rules;
    for (const char* type : psuInterfaceTypes)
    {
        auto match = std::make_unique<sdbusplus::bus::match_t>(
            static_cast<sdbusplus::bus_t&>(*systemBus),
            type::signal() + member("PropertiesChanged") +
                path_namespace(inventoryPath) + arg0namespace(type),
            eventHandler);
        matches.emplace_back(std::move(match));
    }

    for (const char* eventType : psuEventInterface)
    {
        auto eventMatch = std::make_unique<sdbusplus::bus::match_t>(
            static_cast<sdbusplus::bus_t&>(*systemBus),
            type::signal() + member("PropertiesChanged") +
                path_namespace(eventPath) + arg0namespace(eventType),
            eventCollect);
        matches.emplace_back(std::move(eventMatch));
    }
}

static const constexpr int psuDepth = 3;
// Check PSU information from entity-manager D-Bus interface and use the bus
// address to create PSU Class for cold redundancy.
void ColdRedundancy::createPSU(
    std::shared_ptr<sdbusplus::asio::connection>& conn)
{
    numberOfPSU = 0;
    powerSupplies.clear();

    // call mapper to get matched obj paths
    conn->async_method_call(
        [this, &conn](const boost::system::error_code ec,
                      CR::GetSubTreeType subtree) {
            if (ec)
            {
                std::cerr << "Exception happened when communicating to "
                             "ObjectMapper\n";
                return;
            }
            for (const auto& object : subtree)
            {
                std::string pathName = object.first;
                for (const auto& serviceIface : object.second)
                {
                    std::string serviceName = serviceIface.first;
                    for (const auto& interface : serviceIface.second)
                    {
                        // only get property of matched interface
                        bool isIfaceMatched = false;
                        for (const auto& type : psuInterfaceTypes)
                        {
                            if (type == interface)
                            {
                                isIfaceMatched = true;
                                break;
                            }
                        }
                        if (!isIfaceMatched)
                            continue;

                        conn->async_method_call(
                            [this, &conn,
                             interface](const boost::system::error_code ec,
                                        CR::PropertyMapType propMap) {
                                if (ec)
                                {
                                    std::cerr
                                        << "Exception happened when get all "
                                           "properties\n";
                                    return;
                                }

                                auto configName =
                                    std::get_if<std::string>(&propMap["Name"]);
                                if (configName == nullptr)
                                {
                                    std::cerr << "error finding necessary "
                                                 "entry in configuration\n";
                                    return;
                                }

                                auto configBus =
                                    std::get_if<uint64_t>(&propMap["Bus"]);
                                auto configAddress =
                                    std::get_if<uint64_t>(&propMap["Address"]);

                                if (configBus == nullptr ||
                                    configAddress == nullptr)
                                {
                                    std::cerr << "error finding necessary "
                                                 "entry in configuration\n";
                                    return;
                                }
                                for (auto& psu : powerSupplies)
                                {
                                    if ((static_cast<uint8_t>(*configBus) ==
                                         psu->bus) &&
                                        (static_cast<uint8_t>(*configAddress) ==
                                         psu->address))
                                    {
                                        return;
                                    }
                                }

                                uint8_t order = 0;

                                powerSupplies.emplace_back(
                                    std::make_unique<PowerSupply>(
                                        *configName,
                                        static_cast<uint8_t>(*configBus),
                                        static_cast<uint8_t>(*configAddress),
                                        order, conn));

                                numberOfPSU++;
                                std::vector<uint8_t> orders = {};
                                for (auto& psu : powerSupplies)
                                {
                                    orders.push_back(psu->order);
                                }
                            },
                            serviceName.c_str(), pathName.c_str(),
                            "org.freedesktop.DBus.Properties", "GetAll",
                            interface);
                    }
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory/system", psuDepth, psuInterfaceTypes);
}

PowerSupply::PowerSupply(
    std::string& name, uint8_t bus, uint8_t address, uint8_t order,
    const std::shared_ptr<sdbusplus::asio::connection>& dbusConnection) :
    name(name), bus(bus), address(address), order(order)
{
    CR::getPSUEvent(dbusConnection, name, state);
}
