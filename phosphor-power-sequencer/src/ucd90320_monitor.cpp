/**
 * Copyright © 2021 IBM Corporation
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

#include "ucd90320_monitor.hpp"

#include "utility.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <fstream>
#include <map>
#include <string>

namespace phosphor::power::sequencer
{

using json = nlohmann::json;
using namespace phosphor::logging;
using namespace phosphor::power;

const std::string compatibleInterface =
    "xyz.openbmc_project.Configuration.IBMCompatibleSystem";
const std::string compatibleNamesProperty = "Names";

UCD90320Monitor::UCD90320Monitor(sdbusplus::bus::bus& bus, std::uint8_t i2cBus,
                                 std::uint16_t i2cAddress) :
    PowerSequencerMonitor(),
    bus{bus}, match{bus,
                    sdbusplus::bus::match::rules::interfacesAdded() +
                        sdbusplus::bus::match::rules::sender(
                            "xyz.openbmc_project.EntityManager"),
                    std::bind(&UCD90320Monitor::interfacesAddedHandler, this,
                              std::placeholders::_1)},
    pmbusInterface{
        fmt::format("/sys/bus/i2c/devices/{}-{:04x}", i2cBus, i2cAddress)
            .c_str(),
        "ucd9000", 0}

{
    // Use the compatible system types information, if already available, to
    // load the configuration file
    findCompatibleSystemTypes();
}

void UCD90320Monitor::findCompatibleSystemTypes()
{
    try
    {
        auto subTree = util::getSubTree(bus, "/xyz/openbmc_project/inventory",
                                        compatibleInterface, 0);

        auto objectIt = subTree.cbegin();
        if (objectIt != subTree.cend())
        {
            const auto& objPath = objectIt->first;

            // Get the first service name
            auto serviceIt = objectIt->second.cbegin();
            if (serviceIt != objectIt->second.cend())
            {
                std::string service = serviceIt->first;
                if (!service.empty())
                {
                    std::vector<std::string> compatibleSystemTypes;

                    // Get compatible system types property value
                    util::getProperty(compatibleInterface,
                                      compatibleNamesProperty, objPath, service,
                                      bus, compatibleSystemTypes);

                    log<level::DEBUG>(
                        fmt::format("Found compatible systems: {}",
                                    compatibleSystemTypes)
                            .c_str());
                    // Use compatible systems information to find config file
                    findConfigFile(compatibleSystemTypes);
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Compatible system types information is not available.
    }
}

void UCD90320Monitor::findConfigFile(
    const std::vector<std::string>& compatibleSystemTypes)
{
    // Expected config file path name:
    // /usr/share/phosphor-power-sequencer/UCD90320Monitor_<systemType>.json

    // Add possible file names based on compatible system types (if any)
    for (const std::string& systemType : compatibleSystemTypes)
    {
        // Check if file exists
        std::filesystem::path pathName{
            "/usr/share/phosphor-power-sequencer/UCD90320Monitor_" +
            systemType + ".json"};
        if (std::filesystem::exists(pathName))
        {
            log<level::INFO>(
                fmt::format("Config file path: {}", pathName.string()).c_str());
            parseConfigFile(pathName);
            break;
        }
    }
}

void UCD90320Monitor::interfacesAddedHandler(sdbusplus::message::message& msg)
{
    // Only continue if message is valid and rails / pins have not already been
    // found
    if (!msg || !rails.empty())
    {
        return;
    }

    try
    {
        // Read the dbus message
        sdbusplus::message::object_path objPath;
        std::map<std::string,
                 std::map<std::string, std::variant<std::vector<std::string>>>>
            interfaces;
        msg.read(objPath, interfaces);

        // Find the compatible interface, if present
        auto itIntf = interfaces.find(compatibleInterface);
        if (itIntf != interfaces.cend())
        {
            // Find the Names property of the compatible interface, if present
            auto itProp = itIntf->second.find(compatibleNamesProperty);
            if (itProp != itIntf->second.cend())
            {
                // Get value of Names property
                const auto& propValue = std::get<0>(itProp->second);
                if (!propValue.empty())
                {
                    log<level::INFO>(
                        fmt::format(
                            "InterfacesAdded for compatible systems: {}",
                            propValue)
                            .c_str());

                    // Use compatible systems information to find config file
                    findConfigFile(propValue);
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Error trying to read interfacesAdded message.
    }
}

void UCD90320Monitor::parseConfigFile(const std::filesystem::path& pathName)
{
    try
    {
        std::ifstream file{pathName};
        json rootElement = json::parse(file);

        // Parse rail information from config file
        auto railsIterator = rootElement.find("rails");
        if (railsIterator != rootElement.end())
        {
            for (const auto& railElement : *railsIterator)
            {
                std::string rail = railElement.get<std::string>();
                rails.emplace_back(std::move(rail));
            }
        }
        else
        {
            log<level::ERR>(
                fmt::format("No rails found in configuration file: {}",
                            pathName.string())
                    .c_str());
        }
        log<level::DEBUG>(fmt::format("Found rails: {}", rails).c_str());

        // Parse pin information from config file
        auto pinsIterator = rootElement.find("pins");
        if (pinsIterator != rootElement.end())
        {
            for (const auto& pinElement : *pinsIterator)
            {
                auto nameIterator = pinElement.find("name");
                auto lineIterator = pinElement.find("line");

                if (nameIterator != pinElement.end() &&
                    lineIterator != pinElement.end())
                {
                    std::string name = (*nameIterator).get<std::string>();
                    int line = (*lineIterator).get<int>();

                    Pin pin;
                    pin.name = name;
                    pin.line = line;
                    pins.emplace_back(std::move(pin));
                }
                else
                {
                    log<level::ERR>(
                        fmt::format(
                            "No name or line found within pin in configuration file: {}",
                            pathName.string())
                            .c_str());
                }
            }
        }
        else
        {
            log<level::ERR>(
                fmt::format("No pins found in configuration file: {}",
                            pathName.string())
                    .c_str());
        }
        log<level::DEBUG>(
            fmt::format("Found number of pins: {}", rails.size()).c_str());
    }
    catch (const std::exception& e)
    {
        // Log error message in journal
        log<level::ERR>(std::string("Exception parsing configuration file: " +
                                    std::string(e.what()))
                            .c_str());
    }
}

} // namespace phosphor::power::sequencer
