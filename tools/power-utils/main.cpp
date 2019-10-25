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
#include "updater.hpp"
#include "version.hpp"

#include <CLI/CLI.hpp>
#include <cassert>
#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;

int main(int argc, char** argv)
{

    std::string psuPath;
    std::vector<std::string> versions;
    std::vector<std::string> updateArguments;

    CLI::App app{"PSU utils app for OpenBMC"};
    app.add_option("-g,--get-version", psuPath,
                   "Get PSU version from inventory path");
    app.add_option("-c,--compare", versions,
                   "Compare and get the latest version");
    app.add_option("-u,--update", updateArguments,
                   "Update PSU firmware, expeting two arguments: "
                   "<PSU inventory path> <image-dir>")
        ->expected(2);
    app.require_option(1); // Only one option is supported
    CLI11_PARSE(app, argc, argv);

    std::string ret;

    if (!psuPath.empty())
    {
        ret = version::getVersion(psuPath);
    }
    if (!versions.empty())
    {
        ret = version::getLatest(versions);
    }
    if (!updateArguments.empty())
    {
        assert(updateArguments.size() == 2);
        ret = updater::update(updateArguments[0], updateArguments[1]);
    }

    printf("%s", ret.c_str());
    return ret.empty() ? 1 : 0;
}
