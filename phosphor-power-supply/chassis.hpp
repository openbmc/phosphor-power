#pragma once

#include "power_supply.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/State/Decorator/PowerSystemInputs/server.hpp>

#include <map>
#include <string>
#include <vector>

struct supported_psu_configuration
{
    int powerSupplyCount;
    std::vector<uint64_t> inputVoltage;
    bool powerConfigFullLoad;
};

using namespace phosphor::power::psu;
using namespace phosphor::logging;

namespace phosphor::power::chassis
{

using PowerSystemInputsInterface = sdbusplus::xyz::openbmc_project::State::
    Decorator::server::PowerSystemInputs;
using PowerSystemInputsObject =
    sdbusplus::server::object_t<PowerSystemInputsInterface>;

// Validation timeout. Allow 30s to detect if new EM interfaces show up in D-Bus
// before performing the validation.
// Previously the timer was set to 10 seconds was too short, it results in
// incorrect errors being logged, but no real consequence of longer timeout.
constexpr auto validationTimeout = std::chrono::seconds(30);

/**
 * @class Chassis
 *
 * @brief This class will create an object used to manage and monitor a list of
 *        power supply devices attached to the chassis.
 */
class Chassis
{
  public:
    Chassis() = delete;
    ~Chassis() = default;
    Chassis(const Chassis&) = delete;
    Chassis& operator=(const Chassis&) = delete;
    Chassis(Chassis&&) = delete;
    Chassis& operator=(Chassis&&) = delete;

    /**
     * @brief Constructor to read configuration from D-Bus.
     *
     * @param[in] bus - D-Bus bus object
     * @param[in] chassisPath - Chassis path
     */
    Chassis(sdbusplus::bus_t& bus, const std::string& chassisPath);

    /**
     * @brief Get the status of Power on.
     */
    bool isPowerOn()
    {
        return powerOn;
    }

  private:
    /**
     * @brief The D-Bus object
     */
    sdbusplus::bus_t& bus;

    /**
     * @brief The timer that performs power supply validation as the entity
     *        manager interfaces show up in d-bus.
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        validationTimer;

    /** @brief True if the power is on. */
    bool powerOn = false;

    /** @brief True if power control is in the window between chassis pgood loss
     *         and power off.
     */
    bool powerFaultOccurring = false;

    /** @brief True if an error for a brownout has already been logged. */
    bool brownoutLogged = false;

    /** @brief Used as part of subscribing to power on state changes*/
    std::string powerService;

    /** @brief Used to subscribe to D-Bus power on state changes */
    std::unique_ptr<sdbusplus::bus::match_t> powerOnMatch;

    /** @brief Used to subscribe to D-Bus power supply presence changes */
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> presenceMatches;

    /** @brief Used to subscribe to Entity Manager interfaces added */
    std::unique_ptr<sdbusplus::bus::match_t> entityManagerIfacesAddedMatch;

    /**
     * @brief Flag to indicate if the validateConfig() function should be run.
     *        Set to false once the configuration has been validated to avoid
     *        running multiple times due to interfaces added signal. Set to
     *        true during power off to trigger the validation on power on.
     */
    bool runValidateConfig = true;

    /**
     * @brief Map of supported PSU configurations that include the model name
     *        and their properties.
     */
    std::map<std::string, supported_psu_configuration> supportedConfigs;

    /**
     * @brief The vector for power supplies.
     */
    std::vector<std::unique_ptr<PowerSupply>> psus;

    /**
     * @brief The device driver name for all power supplies.
     */
    std::string driverName;

    /**
     * @brief Chassis D-Bus object path
     */
    std::string chassisPath;

    /**
     * @brief The Chassis slot number
     */
    uint64_t chassisSlotNumber;

    /**
     * @brief Get PSU properties from D-Bus, use that to build a power supply
     *        object.
     *
     * @param[in] properties - A map of property names and values
     *
     */
    void getPSUProperties(util::DbusPropertyMap& properties);

    /**
     * @brief Get PSU configuration from D-Bus
     */
    void getPSUConfiguration();

    /**
     * @brief Initialize the chassis's supported configuration from the
     *        Supported Configuration D-Bus object provided by the Entity
     *        Manager.
     */
    void getSupportedConfiguration();

    /**
     * @brief Callback for inventory property changes
     *
     * Process change of the Power Supply presence.
     *
     * @param[in]  msg - Data associated with the Present change signal
     **/
    void psuPresenceChanged(sdbusplus::message_t& msg);

    /**
     * @brief Helper function to populate the PSU supported configuration
     *
     * @param[in] properties - A map of property names and values
     */
    void populateSupportedConfiguration(
        const util::DbusPropertyMap& properties);

    /**
     * @brief Build the device driver name for the power supply.
     *
     * @param[in] i2cbus - i2c bus
     * @param[in] i2caddr - i2c bus address
     */
    void buildDriverName(uint64_t i2cbus, uint64_t i2caddr);

    /**
     * @brief Find PSU with device driver name, then populate the device
     *        driver name to all PSUs (including missing PSUs).
     */
    void populateDriverName();

    /**
     * @brief Get object path slot number
     *
     * @param[in] Object interface (Entity Manager or Inventory Manager)
     * @param[in] Object Path
     * @return uint64_t Slot number
     */
    uint64_t getSlotNumber(const std::string& service,
                           const std::string& objectPath);
};

} // namespace phosphor::power::chassis
