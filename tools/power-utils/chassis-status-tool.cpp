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

constexpr auto numChassis = 8;
constexpr auto smallIndent = "    ";
constexpr auto normalIndent = "       ";
constexpr auto largeIndent = "           ";

void dump(sdbusplus::bus_t& bus, int chassisNumber, bool isVerbose)
{
    phosphor::power::util::ChassisStatusMonitorOptions monitorOptions{};

    monitorOptions.isPresentMonitored = true;
    monitorOptions.isAvailableMonitored = true;
    monitorOptions.isEnabledMonitored = true;
    monitorOptions.isPowerStateMonitored = true;
    monitorOptions.isPowerGoodMonitored = true;
    monitorOptions.isInputPowerStatusMonitored = true;
    monitorOptions.isPowerSuppliesStatusMonitored = true;

    phosphor::power::util::BMCChassisStatusMonitor chassis(
        bus, chassisNumber,
        std::format("/xyz/openbmc_project/inventory/system/chassis{}",
                    chassisNumber),
        monitorOptions);

    std::println("");
    std::println("Chassis {}", chassisNumber);

    bool hadException = false;

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
        std::println("{}Object Path: {}", indent, chassis.getPresentPath());
        std::println("{}Interface: {}", indent, chassis.getPresentInterface());
    }

    hadException = false;

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
        std::println("{}Object Path: {}", indent, chassis.getAvailablePath());
        std::println("{}Interface: {}", indent,
                     chassis.getAvailableInterface());
    }

    hadException = false;

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
        std::println("{}Object Path: {}", indent, chassis.getEnabledPath());
        std::println("{}Interface: {}", indent, chassis.getEnabledInterface());
    }

    hadException = false;

    std::print("{}Power state: ", smallIndent);
    try
    {
        std::println("{}",
                     chassis.getPowerState() == 1 ? "Power On" : "Power Off");
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
        std::println("{}Object Path: {}", indent, chassis.getPowerStatePath());
        std::println("{}Interface: {}", indent,
                     chassis.getPowerStateInterface());
    }

    hadException = false;

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
        std::println("{}Object Path: {}", indent, chassis.getPowerGoodPath());
        std::println("{}Interface: {}", indent,
                     chassis.getPowerGoodInterface());
    }

    hadException = false;

    std::print("{}Input Power Status: ", smallIndent);
    try
    {
        std::println(
            "{}", chassis.getInputPowerStatus() ==
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

    std::print("{}Power Supply Status: ", smallIndent);
    try
    {
        std::println(
            "{}", chassis.getPowerSuppliesStatus() ==
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

int main(int argc, char** argv)
{
    int chassisNumber = -1;
    CLI::App app{"Chassis status tool"};
    app.require_subcommand(1);
    auto dumpCmd = app.add_subcommand("dump", "Dump chassis status");
    auto isVerbose = false;
    dumpCmd
        ->add_option(
            "-c", chassisNumber,
            "Specify a number from 1 to n, where n is the number of the chassis")
        ->expected(1);
    dumpCmd
        ->add_flag("-v,--verbose", isVerbose,
                   "Include object and interface paths in output")
        ->expected(0);
    CLI11_PARSE(app, argc, argv);
    auto bus = sdbusplus::bus::new_default();
    if (app.got_subcommand("dump"))
    {
        if (chassisNumber > -1)
        {
            dump(bus, chassisNumber, isVerbose);
        }
        else
        {
            for (int i = 0; i <= numChassis; i++)
            {
                dump(bus, i, isVerbose);
            }
        }
    }

    std::println("");

    return 0;
}
