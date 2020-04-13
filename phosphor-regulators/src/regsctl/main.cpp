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

#include <algorithm>
#include <getopt.h>
#include <iostream>
#include <string>

using namespace phosphor::power::regulators::control;

void usage(char** argv)
{
    std::cerr << "Usage: " << argv[0] << " [options]\n";
    std::cerr << "Options:\n";
    std::cerr << "    --help    Show this help\n";
    std::cerr << "    --config  Configure the regulators\n";
    std::cerr << "    --monitor [enable|disable]\n"
        "              Enable or disable the monitoring of regulators\n";
    std::cerr << std::flush;
}

int main(int argc, char* argv[])
{
    auto rc = 0;

    const char* optionstr = "cm:h";
    const option options[] =
    {
        { "config", no_argument, NULL, 'c' },
        { "monitor", required_argument, NULL, 'm' },
        { "help", no_argument, NULL, 'h' },
        { 0, 0, 0, 0}
    };

    auto opt = 0;
    while ((opt = getopt_long(argc, argv, optionstr, options, NULL)) != -1)
    {
        switch (opt)
        {
            case 'c':
                callMethod("Configure");
                break;
            case 'm':
            {
                std::string state = optarg;
                // Handle being case-insensitive for monitoring state argument
                std::transform(state.begin(), state.end(), state.begin(),
                    tolower);
                if (state == "enable")
                {
                    callMethod("Monitor", true);
                }
                else if (state == "disable")
                {
                    callMethod("Monitor", false);
                }
                else
                {
                    std::cerr << "regsctl: Specify to \"enable\" or "
                        "\"disable\" monitoring\n";
                    usage(argv);
                }
                break;
            }
            default:
                usage(argv);
        }
    }

    return rc;
}
