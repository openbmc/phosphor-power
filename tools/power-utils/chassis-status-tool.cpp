#include "config.h"

#include "chassis_status_monitor.hpp"

#include <CLI/CLI.hpp>
#include <sdbusplus/bus.hpp>

#define num_chassis 8

void dump(sdbusplus::bus_t& bus, int chassis_number)
{
    phosphor::power::util::ChassisStatusMonitorOptions monitorOptions{};

    if (chassis_number < 0 || chassis_number > 8)
    {
        std::cerr << "Invalid chassis number" << std::endl;
        return;
    }
    phosphor::power::util::BMCChassisStatusMonitor chassis(
        bus, chassis_number, "/xyz/openbmc_project/inventory/system/chassis",
        monitorOptions);

    try
    {
        std::cout << (chassis.isPresent() == 1 ? "Chassis Present"
                                               : "Chassis Not Present")
                  << std::endl;
    }
    catch (...)
    {}
    try
    {
        std::cout << (chassis.isAvailable() == 1 ? "Chassis Available"
                                                 : "Chassis Not Available")
                  << std::endl;
    }
    catch (...)
    {}
    try
    {
        std::cout << (chassis.isEnabled() == 1 ? "Chassis Enabled"
                                               : "Chassis Not Enabled")
                  << std::endl;
    }
    catch (...)
    {}

    try
    {
        std::cout << (chassis.getPowerState() == 1
                          ? "Chassis Power On Requested"
                          : "Chassis Power Off Requested")
                  << std::endl;
    }
    catch (...)
    {}
    try
    {
        std::cout << (chassis.getPowerGood() == 1 ? "Chassis Powered On"
                                                  : "Chassis Powered Off")
                  << std::endl;
    }
    catch (...)
    {}

    try
    {
        std::cout
            << (chassis.getInputPowerStatus() ==
                        phosphor::power::util::PowerSystemInputs::Status::Good
                    ? "Input Power Status Good"
                    : "Input Power Status Fault")
            << std::endl;
    }
    catch (...)
    {}
    try
    {
        std::cout
            << (chassis.getPowerSuppliesStatus() ==
                        phosphor::power::util::PowerSystemInputs::Status::Good
                    ? "Power Supply Status Good"
                    : "Power Supply Status Fault")
            << std::endl;
    }
    catch (...)
    {}
}

int main(int argc, char** argv)
{
    std::string chassisArguments;
    bool rawOutput = false;
    CLI::App app{"Chassis status tool for OpenBMC"};
    auto action = app.add_option_group("Action");

    action
        ->add_option("-d", chassisArguments, "Dump relevant chassis properties")
        ->expected(0, 1);
    CLI11_PARSE(app, argc, argv);
    std::string ret;

    auto bus = sdbusplus::bus::new_default();
    if (!chassisArguments.empty())
    {
        auto chassis_number = (std::atoi(chassisArguments.c_str()));
        dump(bus, chassis_number);
    }
    else
    {
        for (int i = 0; i <= num_chassis; i++)
        {
            std::cout << "Chassis " << i << ":" << std::endl;
            dump(bus, i);
        }
    }

    printf("%s", ret.c_str());
    if (!rawOutput)
    {
        printf("\n");
    }
    return ret.empty() ? 1 : 0;
}
