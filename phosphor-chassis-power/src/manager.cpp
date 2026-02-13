#include "manager.hpp"

#include "format_utils.hpp"

#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/State/Chassis/server.hpp>

#include <chrono>
#include <exception>
#include <functional>
#include <span>
#include <thread>
#include <tuple>
#include <utility>

namespace phosphor::power::chassis
{

namespace fs = std::filesystem;

/**
 * Standard configuration file directory.  This directory is part of the
 * firmware install image.  It contains the standard version of the config file.
 */
const fs::path standardConfigFileDir{"/usr/share/phosphor-chassis-power"};
/**
 * Test configuration file directory.  This directory can contain a test version
 * of the config file.  The test version will override the standard version.
 */
const fs::path testConfigFileDir{"/etc/phosphor-chassis-power"};

constexpr auto busName = "xyz.openbmc_project.Power.Chassis";
constexpr std::chrono::minutes maxTimeToWaitForCompatTypes{5};
constexpr auto managerObjPath = "/xyz/openbmc_project/power/chassis/manager";

Manager::Manager(sdbusplus::bus_t& bus, const sdeventplus::Event& event) :
    ManagerObject{bus, managerObjPath}, bus{bus}, eventLoop{event}
{
    // Create object to find compatible system types for current system.
    // Note that some systems do not provide this information.
    compatSysTypesFinder = std::make_unique<util::CompatibleSystemTypesFinder>(
        bus, std::bind_front(&Manager::compatibleSystemTypesFound, this));

    // If no system types found so far, wait until one is found.
    if (compatibleSystemTypes.empty())
    {
        lg2::debug((std::format("Compatible system type not found")).c_str());
        configure();
    }
    // Obtain D-Bus service name
    bus.request_name(busName);
}
void Manager::configure()
{
    // Wait until the config file has been loaded or hit max wait time
    waitUntilConfigFileLoaded();

    // Verify config file has been loaded and System object is valid
    if (isConfigFileLoaded())
    {
        // Log info message in journal; config file path is important
        lg2::info(("Loaded configuration file"));
        // Configure the chassis devices in the system
    }
    else
    {
        // Write error message to journal
        lg2::error("Unable to load configuration file");

        // Throw InternalFailure to propagate error status to D-Bus client
        throw sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure{};
    }
}

void Manager::compatibleSystemTypesFound(const std::vector<std::string>& types)
{
    // If we don't already have compatible system types
    if (compatibleSystemTypes.empty())
    {
        std::string typesStr = format_utils::toString(std::span{types});
        lg2::info((std::format("Compatible system types found: {}", typesStr))
                      .c_str());
        // Store compatible system types
        compatibleSystemTypes = types;

        // Find and load JSON config file based on system types
        loadConfigFile();
    }
}
fs::path Manager::findConfigFile()
{
    // Build list of possible base file names
    std::vector<std::string> fileNames{};

    // Add possible file names based on compatible system types (if any)
    for (const std::string& systemType : compatibleSystemTypes)
    {
        // Look for file name that is entire system type + ".json"
        // Example: com.acme.Hardware.Chassis.Model.MegaServer.json
        fileNames.emplace_back(systemType + ".json");

        // Look for file name that is last node of system type + ".json"
        // Example: MegaServer.json
        std::string::size_type pos = systemType.rfind('.');
        if ((pos != std::string::npos) && ((systemType.size() - pos) > 1))
        {
            fileNames.emplace_back(systemType.substr(pos + 1) + ".json");
        }
    }

    // Look for a config file with one of the possible base names
    for (const std::string& fileName : fileNames)
    {
        // Check if file exists in test directory
        fs::path pathName{testConfigFileDir / fileName};
        if (fs::exists(pathName))
        {
            return pathName;
        }

        // Check if file exists in standard directory
        pathName = standardConfigFileDir / fileName;
        if (fs::exists(pathName))
        {
            return pathName;
        }
    }

    // No config file found; return empty path
    return fs::path{};
}
void Manager::loadConfigFile()
{
    try
    {
        // Find the absolute path to the config file
        fs::path pathName = findConfigFile();
        if (!pathName.empty())
        {
            // Log info message in journal; config file path is important
            lg2::info(
                ("Loading configuration file " + pathName.string()).c_str());
            system = true;

            // Parse the config file

            // Store config file information
        }
    }
    catch (const std::exception& e)
    {
        // Log error messages in journal
        lg2::error("Unable to load configuration file {ERROR}: {EXCEPTION}",
                   "ERROR", e.what(), "EXCEPTION", e);
    }
}
void Manager::waitUntilConfigFileLoaded()
{
    // If compatible system types is empty
    if (compatibleSystemTypes.empty())
    {
        // Loop until compatible system types found or waited max amount of time
        auto start = std::chrono::system_clock::now();
        std::chrono::system_clock::duration timeWaited{0};
        while (compatibleSystemTypes.empty() &&
               (timeWaited <= maxTimeToWaitForCompatTypes))
        {
            // Try to find list of compatible system types.  Force finder object
            // to re-find system types on D-Bus because we are not receiving
            // InterfacesAdded signals within this while loop.
            compatSysTypesFinder->refind();
            if (compatibleSystemTypes.empty())
            {
                // Not found; sleep 5 seconds
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(5s);
            }
            timeWaited = std::chrono::system_clock::now() - start;
        }
    }
}
} // namespace phosphor::power::chassis
