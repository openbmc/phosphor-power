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

#include <array>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>
#include <cold_redundancy.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <phosphor-logging/elog-errors.hpp>
#include <regex>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>

static constexpr const std::array<const char*, 3> psuInterfaceTypes = {
    "xyz.openbmc_project.Configuration.pmbus"};
static const constexpr char* inventoryPath =
    "/xyz/openbmc_project/inventory/system";
static const constexpr char* eventPath = "/xyz/openbmc_project/State/Decorator";

static std::vector<std::unique_ptr<PowerSupply>> powerSupplies;

ColdRedundancy::ColdRedundancy(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    std::shared_ptr<sdbusplus::asio::connection>& systemBus,
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>>& matches) :
    filterTimer(io),
    systemBus(systemBus)
{

    io.post([this, &io, &objectServer, &systemBus]() { createPSU(systemBus); });
    std::function<void(sdbusplus::message::message&)> eventHandler =
        [this, &io, &objectServer,
         &systemBus](sdbusplus::message::message& message) {
            if (message.is_method_error())
            {
                std::cerr << "callback method error\n";
                return;
            }
            filterTimer.expires_after(std::chrono::seconds(1));
            filterTimer.async_wait([this, &io, &objectServer, &systemBus](
                                       const boost::system::error_code& ec) {
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

    std::function<void(sdbusplus::message::message&)> eventCollect =
        [&](sdbusplus::message::message& message) {
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
            catch (const sdbusplus::exception::exception& e)
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
                    if (std::get<bool>(findEvent->second))
                    {
                        psu->state = PSUState::normal;
                    }
                    else
                    {
                        psu->state = PSUState::acLost;
                    }
                }
            }
        };

    for (const char* type : psuInterfaceTypes)
    {
        auto match = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*systemBus),
            "type='signal',member='PropertiesChanged',path_namespace='" +
                std::string(inventoryPath) + "',arg0namespace='" + type + "'",
            eventHandler);
        matches.emplace_back(std::move(match));
    }

    for (const char* eventType : psuEventInterface)
    {
        auto eventMatch = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*systemBus),
            "type='signal',member='PropertiesChanged',path_namespace='" +
                std::string(eventPath) + "',arg0namespace='" + eventType + "'",
            eventCollect);
        matches.emplace_back(std::move(eventMatch));
    }

    io.run();
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
                      GetSubTreeType subtree) {
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
                                        PropertyMapType propMap) {
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
    name(name),
    bus(bus), address(address), order(order)
{
    getPSUEvent(psuEventInterface, dbusConnection, name, state);
}

PowerSupply::~PowerSupply()
{
}
