/**
 * Copyright © 2026 IBM Corporation
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

#include "chassis_status_monitor.hpp"

#include <CLI/CLI.hpp>
#include <sdbusplus/bus.hpp>

#include <print>

constexpr auto numProperties = 7;
constexpr auto smallIndent = "    ";
constexpr auto normalIndent = "       ";
constexpr auto largeIndent = "           ";

/**
 * Get the number of chassis defined in the system by querying D-Bus.
 *
 * @param bus D-Bus bus object
 * @return number of chassis on the systems
 */
int getChassisCount(sdbusplus::bus_t& bus)
{
    auto service = "xyz.openbmc_project.Power.Chassis";
    auto objectPath = "/xyz/openbmc_project/power/chassis";
    auto interfacePath = "org.freedesktop.DBus.Introspectable";
    try
    {
        auto method = bus.new_method_call(service, objectPath, interfacePath,
                                          "Introspect");
        auto reply = bus.call(method);
        std::string introspectXml;
        reply.read(introspectXml);

        int count = 0;
        size_t pos = 0;
        while ((pos = introspectXml.find("<node name=\"chassis", pos)) !=
               std::string::npos)
        {
            count++;
            pos++;
        }
        if (count > 0)
        {
            return count;
        }
    }
    catch (const std::exception& e)
    {
        std::println(stderr, "Warning: Failed to get chassis count: {}",
                     e.what());
    }
    return -1;
}

void display(sdbusplus::bus_t& bus, int chassisNumber,
             const std::vector<bool>& properties, bool isVerbose)
{
    phosphor::power::util::ChassisStatusMonitorOptions monitorOptions{};

    monitorOptions.isPresentMonitored = properties[0];
    monitorOptions.isAvailableMonitored = properties[1];
    monitorOptions.isEnabledMonitored = properties[2];
    monitorOptions.isPowerStateMonitored = properties[3];
    monitorOptions.isPowerGoodMonitored = properties[4];
    monitorOptions.isInputPowerStatusMonitored = properties[5];
    monitorOptions.isPowerSuppliesStatusMonitored = properties[6];

    phosphor::power::util::BMCChassisStatusMonitor chassis(
        bus, chassisNumber,
        std::format("/xyz/openbmc_project/inventory/system/chassis{}",
                    chassisNumber),
        monitorOptions);

    std::println("");
    std::println("Chassis {}", chassisNumber);

    bool hadException = false;

    if (properties[0])
    {
        std::print("{}Present: ", smallIndent);
        try
        {
            std::println("{}", chassis.isPresent() == 1 ? "True" : "False");
        }
        catch (const std::exception& e)
        {
            hadException = true;
            std::println("Unknown");
            std::println(stderr, "{}{}", normalIndent, e.what());
        }

        if (isVerbose)
        {
            std::string indent = hadException ? largeIndent : normalIndent;
            std::println("{}Object Path: {}", indent,
                         chassis.getInventoryPath());
            std::println("{}Interface: {}", indent,
                         chassis.getPresentInterface());
        }
        hadException = false;
    }

    if (properties[1])
    {
        std::print("{}Available: ", smallIndent);
        try
        {
            std::println("{}", chassis.isAvailable() == 1 ? "True" : "False");
        }
        catch (const std::exception& e)
        {
            hadException = true;
            std::println("Unknown");
            std::println(stderr, "{}{}", normalIndent, e.what());
        }

        if (isVerbose)
        {
            std::string indent = hadException ? largeIndent : normalIndent;
            std::println("{}Object Path: {}", indent,
                         chassis.getInventoryPath());
            std::println("{}Interface: {}", indent,
                         chassis.getAvailableInterface());
        }
        hadException = false;
    }

    if (properties[2])
    {
        std::print("{}Enabled: ", smallIndent);
        try
        {
            std::println("{}", chassis.isEnabled() == 1 ? "True" : "False");
        }
        catch (const std::exception& e)
        {
            hadException = true;
            std::println("Unknown");
            std::println(stderr, "{}{}", normalIndent, e.what());
        }

        if (isVerbose)
        {
            std::string indent = hadException ? largeIndent : normalIndent;
            std::println("{}Object Path: {}", indent,
                         chassis.getInventoryPath());
            std::println("{}Interface: {}", indent,
                         chassis.getEnabledInterface());
        }
        hadException = false;
    }

    if (properties[3])
    {
        std::print("{}Power state: ", smallIndent);
        try
        {
            std::println("{}", chassis.getPowerState() == 1 ? "Power On"
                                                            : "Power Off");
        }
        catch (const std::exception& e)
        {
            hadException = true;
            std::println("Unknown");
            std::println(stderr, "{}{}", normalIndent, e.what());
        }

        if (isVerbose)
        {
            std::string indent = hadException ? largeIndent : normalIndent;
            std::println("{}Object Path: {}", indent,
                         chassis.getChassisPowerPath());
            std::println("{}Interface: {}", indent,
                         chassis.getPowerInterface());
        }
        hadException = false;
    }

    if (properties[4])
    {
        std::print("{}Power Good: ", smallIndent);
        try
        {
            std::println("{}", chassis.getPowerGood() == 1 ? "Powered On"
                                                           : "Powered Off");
        }
        catch (const std::exception& e)
        {
            hadException = true;
            std::println("Unknown");
            std::println(stderr, "{}{}", normalIndent, e.what());
        }

        if (isVerbose)
        {
            std::string indent = hadException ? largeIndent : normalIndent;
            std::println("{}Object Path: {}", indent,
                         chassis.getChassisPowerPath());
            std::println("{}Interface: {}", indent,
                         chassis.getPowerInterface());
        }
        hadException = false;
    }

    if (properties[5])
    {
        std::print("{}Input Power Status: ", smallIndent);
        try
        {
            std::println(
                "{}",
                chassis.getInputPowerStatus() ==
                        phosphor::power::util::PowerSystemInputs::Status::Good
                    ? "Good"
                    : "Fault");
        }
        catch (const std::exception& e)
        {
            hadException = true;
            std::println("Unknown");
            std::println(stderr, "{}{}", normalIndent, e.what());
        }

        if (isVerbose)
        {
            std::string indent = hadException ? largeIndent : normalIndent;
            std::println("{}Object Path: {}", indent,
                         chassis.getInputPowerStatusPath());
            std::println("{}Interface: {}", indent,
                         chassis.getPowerSystemInputsInterface());
        }
        hadException = false;
    }

    if (properties[6])
    {
        std::print("{}Power Supply Status: ", smallIndent);
        try
        {
            std::println(
                "{}",
                chassis.getPowerSuppliesStatus() ==
                        phosphor::power::util::PowerSystemInputs::Status::Good
                    ? "Good"
                    : "Fault");
        }
        catch (const std::exception& e)
        {
            hadException = true;
            std::println("Unknown");
            std::println(stderr, "{}{}", normalIndent, e.what());
        }

        if (isVerbose)
        {
            std::string indent = hadException ? largeIndent : normalIndent;
            std::println("{}Object Path: {}", indent,
                         chassis.getPowerSuppliesStatusPath());
            std::println("{}Interface: {}", indent,
                         chassis.getPowerSystemInputsInterface());
        }
    }
}

int main(int argc, char** argv)
{
    auto numChassis = -1;
    auto bus = sdbusplus::bus::new_default();
    int chassisNumber = -1;
    std::vector<bool> properties(numProperties, false);
    std::vector<std::string> propertyNames;
    auto isVerbose = false;

    CLI::App app{"Chassis status tool"};

    app.require_subcommand(0, 1);
    auto displayCmd = app.add_subcommand(
        "display", "Display chassis status (default if no subcommand given)");
    displayCmd->fallthrough();

    auto cOption =
        app.add_option(
               "-c", chassisNumber,
               "Specify a number from 1 to n, where n is the number of the chassis")
            ->expected(1);
    auto nOption =
        app.add_option(
               "-n", numChassis,
               "Specify a number n, include all the chassis up to and including n")
            ->expected(1);
    app.add_option("-p", propertyNames, "Specify what properties to display")
        ->expected(1, numProperties);
    app.add_flag("-v,--verbose", isVerbose,
                 "Include object and interface paths in output")
        ->expected(0);

    nOption->excludes(cOption);

    CLI11_PARSE(app, argc, argv);

    // Only find the chassis count if -c and -n options are not used
    if (numChassis == -1 && chassisNumber == -1)
    {
        numChassis = getChassisCount(bus);
    }

    // If no properties specified, show all
    if (propertyNames.empty())
    {
        properties.assign(numProperties, true);
    }
    else
    {
        std::map<std::string, size_t> propIndex = {
            {"present", 0},      {"available", 1}, {"enabled", 2},
            {"powerState", 3},   {"powerGood", 4}, {"inputPower", 5},
            {"powerSupplies", 6}};

        // Set only the specified properties to true
        for (const auto& prop : propertyNames)
        {
            if (propIndex.count(prop))
            {
                properties[propIndex[prop]] = true;
            }
            else
            {
                std::println(stderr, "Error: Unknown property '{}'", prop);
                return 1;
            }
        }
    }

    if (app.got_subcommand("display") || app.get_subcommands().empty())
    {
        if (chassisNumber > -1)
        {
            display(bus, chassisNumber, properties, isVerbose);
        }
        else
        {
            for (int i = 0; i <= numChassis; i++)
            {
                display(bus, i, properties, isVerbose);
            }
        }
    }

    std::println("");

    return 0;
}
