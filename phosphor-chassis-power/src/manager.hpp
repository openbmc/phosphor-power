#pragma once

#include "compatible_system_types_finder.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace phosphor::power::chassis
{

class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    Manager(sdbusplus::bus_t& bus, const sdeventplus::Event& event);

    /**
     * Callback that is called when a list of compatible system types is found.
     *
     * @param types Compatible system types for the current system ordered from
     *              most to least specific
     */
    void compatibleSystemTypesFound(const std::vector<std::string>& types);
    /**
     * Returns whether the JSON configuration file has been loaded.
     *
     * @return true if config file loaded, false otherwise
     */
    bool isConfigFileLoaded() const
    {
        // If System object exists, the config file has been loaded
        return (system);
    }

  private:
    /**
     * Callback to begin failure process after not finding a Compatible system.
     */
    void compatibleSystemTypesNotFoundCallback();
    /**
     * Finds the JSON configuration file.
     *
     * Looks for a configuration file based on the list of compatible system
     * types.  If no file is found, looks for a file with the default name.
     *
     * Looks for the file in the test directory and standard directory.
     *
     * Throws an exception if an operating system error occurs while checking
     * for the existence of a file.
     *
     * @return absolute path to config file, or an empty path if none found
     */
    std::filesystem::path findConfigFile();

    /**
     * Loads the JSON configuration file.
     *
     * Looks for the config file using findConfigFile().
     *
     * If the config file is found, it is parsed and the resulting information
     * is stored in the system data member.  If parsing fails, an error is
     * logged.
     */
    void loadConfigFile();

    /**
     * The D-Bus bus
     */
    sdbusplus::bus_t& bus [[maybe_unused]];

    /**
     * Event to loop on
     */
    const sdeventplus::Event& eventLoop [[maybe_unused]];

    /**
     * Timer to wait for a Compatible system.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>
        compatibleSystemsTimer;
    /**
     * Object that finds the compatible system types for the current system.
     */
    std::unique_ptr<util::CompatibleSystemTypesFinder> compatSysTypesFinder;

    /**
     * List of compatible system types for the current system.
     *
     * Used to find the JSON configuration file.
     */
    std::vector<std::string> compatibleSystemTypes{};
    /**
     * Computer system being controlled and monitored by the BMC.
     *
     * Temporary set as bool, todo make system class for PCP
     * Will contain the information loaded from the JSON configuration file.
     * Will contain nullptr if the configuration file has not been loaded.
     */
    bool system = false;
};

} // namespace phosphor::power::chassis
