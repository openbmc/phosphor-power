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
#include "model.hpp"
#include "updater.hpp"
#include "utility.hpp"
#include "validator.hpp"
#include "version.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <cassert>
#include <filesystem>

int main(int argc, char** argv)
{
    std::string psuPathVersion, psuPathModel;
    std::vector<std::string> versions;
    bool validateUpdate = false;
    bool rawOutput = false;
    std::vector<std::string> updateArguments;

    CLI::App app{"PSU utils app for OpenBMC"};
    auto action = app.add_option_group("Action");
    action->add_option("-g,--get-version", psuPathVersion,
                       "Get PSU version from inventory path");
    action->add_option("-m,--get-model", psuPathModel,
                       "Get PSU model from inventory path");
    action->add_option("-c,--compare", versions,
                       "Compare and get the latest version");
    auto updateOpt =
        action
            ->add_option("-u,--update", updateArguments,
                         "Update PSU firmware, expecting two arguments: "
                         "<PSU inventory path> <image-dir>")
            ->expected(2)
            ->type_name("PSU_PATH IMAGE_DIR");
    app.add_flag(
           "--validate", validateUpdate,
           "Validate number of present PSU vs number of required PSUs and all PSUs have same model  before updating firmware")
        ->needs(updateOpt);
    action->require_option(1); // Only one option is supported
    app.add_flag("--raw", rawOutput, "Output raw text without linefeed");
    CLI11_PARSE(app, argc, argv);

    std::string ret;

    auto bus = sdbusplus::bus::new_default();
    if (!psuPathVersion.empty())
    {
        ret = version::getVersion(bus, psuPathVersion);
    }
    if (!psuPathModel.empty())
    {
        ret = model::getModel(bus, psuPathModel);
    }
    if (!versions.empty())
    {
        ret = version::getLatest(versions);
    }
    if (!updateArguments.empty())
    {
        assert(updateArguments.size() == 2);
        bool updateStatus = false;
        if (validateUpdate)
        {
            updateStatus = updater::validateAndUpdate(bus, updateArguments[0],
                                                      updateArguments[1]);
        }
        else
        {
            updateStatus =
                updater::update(bus, updateArguments[0], updateArguments[1]);
        }
        if (updateStatus)
        {
            ret = "Update successful";
            lg2::info("Successful update to PSU: {PSU}", "PSU",
                      updateArguments[0]);
        }
        else
        {
            lg2::error("Failed to update PSU: {PSU}", "PSU",
                       updateArguments[0]);
        }
    }

    printf("%s", ret.c_str());
    if (!rawOutput)
    {
        printf("\n");
    }
    return ret.empty() ? 1 : 0;
}
