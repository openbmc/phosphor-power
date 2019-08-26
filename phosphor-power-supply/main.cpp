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
#include <CLI/CLI.hpp>

int main(int argc, char* argv[])
{
    auto rc = -1;

    CLI::App app{"OpenBMC Power Supply Unit Monitor"};
    // Read the arguments.
    CLI11_PARSE(app, argc, argv);

    rc = 0;

    return rc;
}
