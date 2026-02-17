#include "config.h"

#include "chassis_status_monitor.hpp"

#include <CLI/CLI.hpp>
#include <sdbusplus/bus.hpp>

#define num_chassis 8

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

    std::cout << std::endl << "Chassis " << chassisNumber << ":" << std::endl;
    std::cout << "    Present: ";
    try
    {
        std::cout << (chassis.isPresent() == 1 ? "True" : "False") << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Unknown" << std::endl;
        std::cerr << "       " << e.what() << std::endl;
    }
    std::cout << "    Available: ";
    try
    {
        std::cout << (chassis.isAvailable() == 1 ? "True" : "False")
                  << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Unknown" << std::endl;
        std::cerr << "       " << e.what() << std::endl;
    }
    std::cout << "    Enabled: ";
    try
    {
        std::cout << (chassis.isEnabled() == 1 ? "True" : "False") << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Unknown" << std::endl;
        std::cerr << "       " << e.what() << std::endl;
    }
    std::cout << "    Power state: ";
    try
    {
        std::cout << (chassis.getPowerState() == 1 ? "Power On" : "Power Off")
                  << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Unknown" << std::endl;
        std::cerr << "       " << e.what() << std::endl;
    }
    std::cout << "    Power Good: ";
    try
    {
        std::cout << (chassis.getPowerGood() == 1 ? "Powered On"
                                                  : "Powered Off")
                  << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Unknown" << std::endl;
        std::cerr << "       " << e.what() << std::endl;
    }
    std::cout << "    Input Power Status: ";
    try
    {
        std::cout
            << (chassis.getInputPowerStatus() ==
                        phosphor::power::util::PowerSystemInputs::Status::Good
                    ? "Good"
                    : "Fault")
            << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Unknown" << std::endl;
        std::cerr << "       " << e.what() << std::endl;
    }
    std::cout << "    Power Supply Status: ";
    try
    {
        std::cout
            << (chassis.getPowerSuppliesStatus() ==
                        phosphor::power::util::PowerSystemInputs::Status::Good
                    ? "Good"
                    : "Fault")
            << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Unknown" << std::endl;
        std::cerr << "       " << e.what() << std::endl;
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
            for (int i = 0; i <= num_chassis; i++)
            {
                dump(bus, i);
            }
        }
    }

    std::cout << std::endl;

    return 0;
}
