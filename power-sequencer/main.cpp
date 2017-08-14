/**
 * Copyright © 2017 IBM Corporation
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
#include <chrono>
#include <iostream>
#include "argument.hpp"

using namespace witherspoon::power;

int main(int argc, char** argv)
{
    ArgumentParser args{argc, argv};
    auto action = args["action"];

    if (action != "pgood-monitor")
    {
        std::cerr << "Invalid action\n";
        args.usage(argv);
        exit(EXIT_FAILURE);
    }

    auto i = strtoul(args["interval"].c_str(), nullptr, 10);
    if (i == 0)
    {
        std::cerr << "Invalid interval value\n";
        exit(EXIT_FAILURE);
    }

    std::chrono::seconds interval{i};

    return EXIT_SUCCESS;
}
