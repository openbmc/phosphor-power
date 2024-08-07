/**
 * Copyright © 2024 IBM Corporation
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
#include "updater.hpp"
#include "version.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <cassert>
#include <format>

using namespace phosphor::logging;

int main(int argc, char** argv)
{
    std::string psuPath;
    std::vector<std::string> versions;
    bool rawOutput = false;
    std::vector<std::string> updateArguments;

    CLI::App app{"PSU utils app for OpenBMC"};
    auto action = app.add_option_group("Action");
    action->add_option("-g,--get-version", psuPath,
                       "Get PSU version from inventory path");
    action->add_option("-c,--compare", versions,
                       "Compare and get the latest version");
    action
        ->add_option("-u,--update", updateArguments,
                     "Update PSU firmware, expecting two arguments: "
                     "<PSU inventory path> <image-dir>")
        ->expected(2);
    action->require_option(1); // Only one option is supported
    app.add_flag("--raw", rawOutput, "Output raw text without linefeed");
    CLI11_PARSE(app, argc, argv);

    std::string ret;

    if (!psuPath.empty())
    {
        auto bus = sdbusplus::bus::new_default();
        ret = version::getVersion(bus, psuPath);
    }
    if (!versions.empty())
    {
        ret = version::getLatest(versions);
    }
    if (!updateArguments.empty())
    {
        assert(updateArguments.size() == 2);
        if (updater::update(updateArguments[0], updateArguments[1]))
        {
            ret = "Update successful";
        }
    }

    printf("%s", ret.c_str());
    if (!rawOutput)
    {
        printf("\n");
    }
    return ret.empty() ? 1 : 0;
}
