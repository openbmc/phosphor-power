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

#include <CLI/CLI.hpp>

#include <chrono>
#include <string>

int main(int argc, char** argv)
{
    CLI::App app{"OpenBMC UCD90320 Power Sequencer Monitor"};

    std::string action;
    app.add_option("-a,--action", action,
                   "Action: pgood-monitor or runtime-monitor")
        ->required()
        ->check(CLI::IsMember({"pgood-monitor", "runtime-monitor"}));

    long i{0};
    app.add_option("-i,--interval", i, "Interval in milliseconds")
        ->required()
        ->check(CLI::PositiveNumber);

    CLI11_PARSE(app, argc, argv);

    std::chrono::milliseconds interval{i};

    return 0;
}
