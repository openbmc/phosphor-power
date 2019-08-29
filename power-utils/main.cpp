/**
 * Copyright © 2019 IBM Corporation
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
#include "argument.hpp"
#include "version.hpp"

#include <iostream>
#include <phosphor-logging/log.hpp>

using namespace witherspoon::power;
using namespace phosphor::logging;

int main(int argc, char** argv)
{
    ArgumentParser args{argc, argv};
    auto psuPath = args["getversion"];
    if (psuPath.empty())
    {
        log<level::ERR>("PSU Inventory path argument required");
        args.usage(argv);
        exit(1);
    }

    // For now only getversion is supported
    auto version = version::getVersion(psuPath);
    printf("%s", version.c_str());
    return version.empty() ? 1 : 0;
}
