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

void dump(sdbusplus::bus_t& bus, int chassisNumber)
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

    std::print("    Present: ");
    try
    {
        std::println("{}", chassis.isPresent() == 1 ? "True" : "False");
    }
    catch (const std::exception& e)
    {
        std::println("Unknown");
        std::println(stderr, "       {}", e.what());
    }

    std::print("    Available: ");
    try
    {
        std::println("{}", chassis.isAvailable() == 1 ? "True" : "False");
    }
    catch (const std::exception& e)
    {
        std::println("Unknown");
        std::println(stderr, "       {}", e.what());
    }

    std::print("    Enabled: ");
    try
    {
        std::println("{}", chassis.isEnabled() == 1 ? "True" : "False");
    }
    catch (const std::exception& e)
    {
        std::println("Unknown");
        std::println(stderr, "       {}", e.what());
    }

    std::print("    Power state: ");
    try
    {
        std::println("{}",
                     chassis.getPowerState() == 1 ? "Power On" : "Power Off");
    }
    catch (const std::exception& e)
    {
        std::println("Unknown");
        std::println(stderr, "       {}", e.what());
    }

    std::print("    Power Good: ");
    try
    {
        std::println("{}", chassis.getPowerGood() == 1 ? "Powered On"
                                                       : "Powered Off");
    }
    catch (const std::exception& e)
    {
        std::println("Unknown");
        std::println(stderr, "       {}", e.what());
    }

    std::print("    Input Power Status: ");
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
        std::println("Unknown");
        std::println(stderr, "       {}", e.what());
    }

    std::print("    Power Supply Status: ");
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
        std::println("Unknown");
        std::println(stderr, "       {}", e.what());
    }
}

int main(int argc, char** argv)
{
    int chassisNumber = -1;
    CLI::App app{"Chassis status tool"};
    auto commands = app.add_option_group("Commands");
    auto dump_cmd = commands->add_subcommand("dump", "Dump chassis status");
    dump_cmd
        ->add_option(
            "-c", chassisNumber,
            "Specify a number from 1 to n, where n is the number of the chassis")
        ->expected(1);
    CLI11_PARSE(app, argc, argv);
    auto bus = sdbusplus::bus::new_default();
    if (app.got_subcommand("dump"))
    {
        if (chassisNumber > -1)
        {
            dump(bus, chassisNumber);
        }
        else
        {
            for (int i = 0; i <= numChassis; i++)
            {
                dump(bus, i);
            }
        }
    }

    std::println("");

    return 0;
}
