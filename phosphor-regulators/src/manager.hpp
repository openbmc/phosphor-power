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
#pragma once

#include "system.hpp"

#include <interfaces/manager_interface.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace phosphor::power::regulators
{

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto objPath = "/xyz/openbmc_project/power/regulators/manager";
constexpr auto sysDbusObj = "/xyz/openbmc_project/inventory";
constexpr auto sysDbusPath = "/xyz/openbmc_project/inventory/system";
constexpr auto sysDbusIntf = "xyz.openbmc_project.Inventory.Item.System";
constexpr auto sysDbusProp = "Identifier";

using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

using ManagerObject = sdbusplus::server::object::object<
    phosphor::power::regulators::interface::ManagerInterface>;

class Manager : public ManagerObject
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /**
     * Constructor
     * Creates a manager over the regulators.
     *
     * @param bus the dbus bus
     * @param event the sdevent event
     */
    Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /**
     * Overridden manager object's configure method
     */
    void configure() override;

    /**
     * Overridden manager object's monitor method
     *
     * @param enable Enable or disable regulator monitoring
     */
    void monitor(bool enable) override;

    /**
     * Timer expired callback function
     */
    void timerExpired();

    /**
     * Callback function to handle receiving a HUP signal
     * to reload the configuration data.
     *
     * @param sigSrc sd_event_source signal wrapper
     * @param sigInfo signal info on signal fd
     */
    void sighupHandler(sdeventplus::source::Signal& sigSrc,
                       const struct signalfd_siginfo* sigInfo);

    /**
     * Callback function to handle interfacesAdded dbus signals
     *
     * @param msg Expanded sdbusplus message data
     */
    void signalHandler(sdbusplus::message::message& msg);

  private:
    /**
     * Set the JSON configuration data filename
     *
     * @param fName filename without `.json` extension
     */
    inline void setFileName(const std::string& fName)
    {
        fileName = fName;
        if (!fileName.empty())
        {
            // Replace all spaces with underscores
            std::replace(fileName.begin(), fileName.end(), ' ', '_');
            fileName.append(".json");
        }
    };

    /**
     * Get the JSON configuration data filename from dbus
     *
     * @return JSON configuration data filename
     */
    const std::string getFileNameDbus();

    /**
     * Finds the JSON configuration file.
     *
     * Looks for the config file in the test directory and standard directory.
     *
     * Throws an exception if the file cannot be found or a file system error
     * occurs.
     *
     * The base name of the config file must have already been obtained and
     * stored in the fileName data member.
     *
     * @return absolute path to config file
     */
    std::filesystem::path findConfigFile();

    /**
     * Loads the JSON configuration file.
     *
     * Looks for the config file in the test directory and standard directory.
     *
     * If the config file is found, it is parsed and the resulting information
     * is stored in the system data member.
     *
     * If the config file cannot be found or parsing fails, an error is logged.
     *
     * The base name of the config file must have already been obtained and
     * stored in the fileName data member.
     */
    void loadConfigFile();

    /**
     * The dbus bus
     */
    sdbusplus::bus::bus& bus;

    /**
     * Event to loop on
     */
    sdeventplus::Event eventLoop;

    /**
     * List of event timers
     */
    std::vector<Timer> timers{};

    /**
     * List of dbus signal matches
     */
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> signals{};

    /**
     * JSON configuration file base name.
     */
    std::string fileName{};

    /**
     * Computer system being controlled and monitored by the BMC.
     *
     * Contains the information loaded from the JSON configuration file.
     * Contains nullptr if the configuration file has not been loaded.
     */
    std::unique_ptr<System> system{};
};

} // namespace phosphor::power::regulators
