/**
 * Copyright © 2020 IBM Corporation
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

#include "utility.hpp"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

using namespace phosphor::power::regulators::control;

int main(int argc, char* argv[])
{
    auto rc = 0;

    try
    {
        bool monitorEnable = false;
        bool monitorDisable = false;

        CLI::App app{"Regulators control app for OpenBMC phosphor-regulators"};

        // Add dbus methods group
        auto methods = app.add_option_group("Methods");
        // Configure method
        CLI::App* config =
            methods->add_subcommand("config", "Configure regulators");
        config->set_help_flag("-h,--help", "Configure regulators method help");
        // Monitor method
        CLI::App* monitor =
            methods->add_subcommand("monitor", "Monitor regulators");
        monitor->set_help_flag("-h,--help", "Monitor regulators method help");
        monitor->add_flag("-e,--enable", monitorEnable,
                          "Enable regulator monitoring");
        monitor->add_flag("-d,--disable", monitorDisable,
                          "Disable regulator monitoring");
        // Monitor subcommand requires only 1 option be provided
        monitor->require_option(1);
        // Methods group requires only 1 subcommand to be given
        methods->require_subcommand(1);

        CLI11_PARSE(app, argc, argv);

        if (app.got_subcommand("config"))
        {
            callMethod("Configure");
        }
        else if (app.got_subcommand("monitor"))
        {
            callMethod("Monitor", monitorEnable);
        }
    }
    catch (const std::exception& e)
    {
        rc = 1;
        std::cerr << e.what() << std::endl;
    }

    return rc;
}
