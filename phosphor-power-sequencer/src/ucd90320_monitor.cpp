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

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <map>
#include <string>

namespace phosphor::power::sequencer
{

using namespace phosphor::logging;
using namespace phosphor::power;

const std::string compatibleInterface =
    "xyz.openbmc_project.Configuration.IBMCompatibleSystem";
const std::string compatibleNamesProperties = "Names";

UCD90320Monitor::UCD90320Monitor(sdbusplus::bus::bus& bus, std::uint8_t i2cBus,
                                 std::uint16_t i2cAddress) :
    PowerSequencerMonitor(),
    bus{bus}, interface{fmt::format("/sys/bus/i2c/devices/{}-{:04x}", i2cBus,
                                    i2cAddress)
                            .c_str(),
                        "ucd9000", 0}
{
    // Subscribe to D-Bus interfacesAdded signal from Entity Manager.  This
    // notifies us if the compatible interface becomes available later.
    match = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::sender(
                "xyz.openbmc_project.EntityManager"),
        std::bind(&UCD90320Monitor::interfacesAddedHandler, this,
                  std::placeholders::_1));

    // Use the compatible system types information, if already available, to
    // load the configuration file
    findCompatibleSystemTypes();
}

void UCD90320Monitor::findCompatibleSystemTypes()
{
    try
    {
        util::DbusSubtree subTree = util::getSubTree(
            bus, "/xyz/openbmc_project/inventory", compatibleInterface, 0);

        auto objectIt = subTree.cbegin();
        if (objectIt != subTree.cend())
        {
            std::string objPath = objectIt->first;

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
                                      compatibleNamesProperties, objPath,
                                      service, bus, compatibleSystemTypes);

                    log<level::INFO>(fmt::format("Found compatible systems: {}",
                                                 compatibleSystemTypes)
                                         .c_str());
                    // Use compatible systems information to find config file
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Compatible system types information is not available.
    }
}

void UCD90320Monitor::interfacesAddedHandler(sdbusplus::message::message& msg)
{
    // Verify message is valid
    if (!msg)
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
            auto itProp = itIntf->second.find(compatibleNamesProperties);
            if (itProp != itIntf->second.cend())
            {
                // Get value of Names property
                auto propValue = std::get<0>(itProp->second);
                if (!propValue.empty())
                {
                    log<level::INFO>(
                        fmt::format(
                            "InterfacesAdded for compatible systems: {}",
                            propValue)
                            .c_str());

                    // Use compatible systems information to find config file
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Error trying to read interfacesAdded message.
    }
}

} // namespace phosphor::power::sequencer
