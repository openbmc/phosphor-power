#pragma once

#include "power_supply.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

using namespace phosphor::power::psu;
using namespace phosphor::logging;

namespace phosphor
{
namespace power
{
namespace manager
{

struct json_properties
{
    int pollInterval;
};

/**
 * @class PSUManager
 *
 * This class will create an object used to manage and monitor a list of power
 * supply devices.
 */
class PSUManager
{
  public:
    PSUManager() = delete;
    ~PSUManager() = default;
    PSUManager(const PSUManager&) = delete;
    PSUManager& operator=(const PSUManager&) = delete;
    PSUManager(PSUManager&&) = delete;
    PSUManager& operator=(PSUManager&&) = delete;

    /**
     * Constructor
     *
     * @param[in] bus - D-Bus bus object
     * @param[in] e - event object
     * @param[in] configfile - string path to the configuration file
     */
    PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e,
               const std::string& configfile) :
        bus(bus)
    {
        // Parse out the JSON properties needed to pass down to the PSU manager.
        json_properties properties = {0};
        getJSONProperties(configfile, properties);

        using namespace sdeventplus;
        auto pollInterval = std::chrono::milliseconds(properties.pollInterval);
        timer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
            e, std::bind(&PSUManager::analyze, this), pollInterval);

        // Subscribe to power state changes
        powerService = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);
        powerOnMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::propertiesChanged(POWER_OBJ_PATH,
                                                            POWER_IFACE),
            [this](auto& msg) { this->powerStateChanged(msg); });

        initialize();
    }

    void getJSONProperties(const std::string& path, json_properties& p)
    {
        using namespace phosphor::logging;

        nlohmann::json configFileJSON = util::loadJSONFromFile(path.c_str());

        if (configFileJSON == nullptr)
        {
            throw std::runtime_error("Failed to load JSON configuration file");
        }

        p.pollInterval = configFileJSON.at("pollInterval");
    }

    /**
     * Initializes the manager.
     *
     * Get current BMC state, ...
     */
    void initialize()
    {
        // When state = 1, system is powered on
        int32_t state = 0;

        try
        {
            // Use getProperty utility function to get power state.
            util::getProperty<int32_t>(POWER_IFACE, "state", POWER_OBJ_PATH,
                                       powerService, bus, state);

            if (state)
            {
                powerOn = true;
            }
            else
            {
                powerOn = false;
            }
        }
        catch (std::exception& e)
        {
            log<level::INFO>("Failed to get power state. Assuming it is off.");
            powerOn = false;
        }

        clearFaults();
        updateInventory();
    }

    /**
     * Starts the timer to start monitoring the list of devices.
     */
    int run()
    {
        // return timer.get_event().loop();
        return timer->get_event().loop();
    }

    /**
     * This function will be called in various situations in order to clear
     * any fault status bits that may have been set, in order to start over
     * with a clean state. Presence changes and power state changes will want
     * to clear any faults logged.
     */
    void clearFaults()
    {
        for (auto& psu : psus)
        {
            psu.clearFaults();
        }
    }

  private:
    /**
     * The D-Bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * The timer that runs to periodically check the power supplies.
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        timer;

    /**
     * Analyze the status of each of the power supplies.
     */
    void analyze()
    {
        for (auto& psu : psus)
        {
            psu.analyze();
        }
    }

    /** @brief True if the power is on. */
    bool powerOn = false;

    /** @brief Used as part of subscribing to power on state changes*/
    std::string powerService;

    /** @brief Used to subscribe to D-Bus power on state changes */
    std::unique_ptr<sdbusplus::bus::match_t> powerOnMatch;

    /**
     * @brief Callback for power state property changes
     *
     * Process changes to the powered on state property for the system.
     *
     * @param[in] msg - Data associated with the power state signal
     */
    void powerStateChanged(sdbusplus::message::message& msg);

    /**
     * @brief Adds properties to the inventory.
     *
     * Reads the values from the devices and writes them to the associated
     * power supply D-Bus inventory objects.
     *
     * This needs to be done on startup, and each time the presence state
     * changes.
     */
    void updateInventory()
    {
        for (auto& psu : psus)
        {
            psu.updateInventory();
        }
    }

    /**
     * @brief The vector for power supplies.
     */
    std::vector<PowerSupply> psus;
};

} // namespace manager
} // namespace power
} // namespace phosphor
