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
#include "power_supply.hpp"
#include "psu_manager.hpp"
#include "utility.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <filesystem>

using namespace phosphor::power;

void getJSONProperties(const std::string& path, sdbusplus::bus::bus& bus,
                       sys_properties& p,
                       std::vector<std::unique_ptr<PowerSupply>>& psus)
{
    using namespace phosphor::logging;

    nlohmann::json configFileJSON = util::loadJSONFromFile(path.c_str());

    if (configFileJSON == nullptr)
    {
        throw std::runtime_error("Failed to load JSON configuration file");
    }

    p.pollInterval = configFileJSON["SystemProperties"]["pollInterval"];
    p.minPowerSupplies = configFileJSON["SystemProperties"]["MinPowerSupplies"];
    p.maxPowerSupplies = configFileJSON["SystemProperties"]["MaxPowerSupplies"];

    for (auto psuJSON : configFileJSON["PowerSupplies"])
    {
        std::string invpath = psuJSON["Inventory"];
        std::uint8_t i2cbus = psuJSON["Bus"];
        std::string i2caddr = psuJSON["Address"];
        auto psu = std::make_unique<PowerSupply>(bus, invpath, i2cbus, i2caddr);
        psus.emplace_back(std::move(psu));
    }
}

int main(int argc, char* argv[])
{
    try
    {
        using namespace phosphor::logging;

        CLI::App app{"OpenBMC Power Supply Unit Monitor"};

        std::string configfile;
        app.add_option("-c,--config", configfile,
                       "JSON configuration file path")
            ->check(CLI::ExistingFile);

        // Read the arguments.
        CLI11_PARSE(app, argc, argv);
        if (configfile.empty())
        {
            configfile = "/etc/phosphor-psu-monitor/psu_config.json";
        }

        if (!std::filesystem::exists(configfile))
        {
            log<level::ERR>("Configuration file does not exist",
                            entry("FILENAME=%s", configfile.c_str()));
            return -1;
        }

        auto bus = sdbusplus::bus::new_default();
        // Parse out the JSON properties needed to pass down to the PSU manager.
        sys_properties properties;
        std::vector<std::unique_ptr<PowerSupply>> psus;
        getJSONProperties(configfile, bus, properties, psus);

        auto event = sdeventplus::Event::get_default();

        // Attach the event object to the bus object so we can
        // handle both sd_events (for the timers) and dbus signals.
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

        manager::PSUManager manager(bus, event, properties, psus);

        return manager.run();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(e.what());
        return -2;
    }
    catch (...)
    {
        log<level::ERR>("Caught unexpected exception type");
        return -3;
    }
}
