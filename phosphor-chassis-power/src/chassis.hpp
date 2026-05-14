/**
 * Copyright © 2026 IBM Corporation
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

#include "chassis_power_system_interface.hpp"
#include "chassis_status_monitor.hpp"
#include "gpio.hpp"
#include "services.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <filesystem>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::chassis
{

/**
 * @enum ChassisState
 *
 * Current state of a chassis.
 */
enum class ChassisState
{
    Missing,  // Chassis is not present
    Off,      // Chassis is present but powered off
    On,       // Chassis is present and powered on
    Faulted   // Chassis has a power fault
};

constexpr auto DbusReadDelay = 120; //SHELDON:ADDED: needs to be 60

/**
 * @class Chassis
 *
 * A chassis within the system.
 *
 * Chassis are large enclosures that can be independently powered off and on by
 * the BMC.  Small and mid-sized systems may contain a single chassis.  In a
 * large rack-mounted system, each drawer may correspond to a chassis.
 *
 * A C++ Chassis object only needs to be created if the physical chassis
 * contains chassis that need to be configured or monitored.
 */
class Chassis
{
  public:
    // Specify which compiler-generated methods we want
    Chassis() = delete;
    Chassis(const Chassis&) = delete;
    Chassis(Chassis&&) = delete;
    Chassis& operator=(const Chassis&) = delete;
    Chassis& operator=(Chassis&&) = delete;
    ~Chassis() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param number Chassis number within the system.  Chassis numbers start at
     *               1 because chassis 0 represents the entire system.
     * @param services Platform services provider
     * @param event Event loop for timer operations
     * @param presencePath Presence absolute path for this chassis
     * @param gpioPins GPIOs within this chassis, if any.  The vector should
     *                 contain GPIOs to perform operations on.
     */
    explicit Chassis(unsigned int number, Services& services,
                     const sdeventplus::Event& event,
                     std::optional<std::string> presencePath = std::nullopt,
                     std::vector<std::unique_ptr<Gpio>> gpioPins =
                         std::vector<std::unique_ptr<Gpio>>{}) :
        number{number}, services{services},
        presencePath{std::move(presencePath)},
        gpios{std::move(gpioPins)}, eventLoop{event}
    {
        if (number < 1)
        {
            throw std::invalid_argument{
                "Invalid chassis number: " + std::to_string(number)};
        }
    }

    /**
     * Returns the chassis number within the system.
     *
     * @return chassis number
     */
    unsigned int getNumber() const
    {
        return number;
    }

    /**
     * Returns the Presence path for this chassis, if any.
     *
     * @return presence path, or std::nullopt if not specified
     */
    const std::optional<std::string>& getPresencePath() const
    {
        return presencePath;
    }

    /**
     * Returns the GPIO objects within this chassis, if any.
     *
     * The vector contains GPIO objects to perform operations.
     *
     * @return GPIO objects in chassis
     */
    const std::vector<std::unique_ptr<Gpio>>& getGpios() const
    {
        return gpios;
    }

    /**
     * Returns the cached presence GPIO value for this chassis.
     *
     * @return presence GPIO value
     */
    std::optional<int> getPresenceGPIOValue() const
    {
        return presenceGPIOValue;
    }

    /**
     * Returns the cached latched fault state for this chassis.
     *
     * @return latched fault GPIO value
     */
    std::optional<int> getFaultLatchedValue() const
    {
        return faultLatchedValue;
    }

    /**
     * Returns the cached unlatched fault state for this chassis.
     *
     * @return unlatched fault GPIO value
     */
    std::optional<int> getFaultUnlatchedValue() const
    {
        return faultUnlatchedValue;
    }

    /**
     * Initialize the PowerSystemInputs D-Bus interface for this chassis.
     *
     * @param bus D-Bus bus object
     *
     * @return true if interface was created and set, false otherwise
     */
    bool initializePowerSystemInputsInterface(sdbusplus::bus_t& bus);

    /**
     * Initialize the chassis status monitor for this chassis.
     */
    void initializeStatusMonitor();

    /**
     * Monitors the status of the chassis.
     */
    void monitor();

    /**
     * Set the system status monitor for this chassis.
     *
     * @param monitor Shared pointer to system status monitor
     */
    void setSystemStatusMonitor(
        const std::shared_ptr<ChassisStatusMonitor>& monitor);

    /**
     * Returns the PowerSystemInputs D-Bus interface for this chassis.
     *
     * @return interface pointer (may be nullptr if not created)
     */
    const std::unique_ptr<ChassisPowerSystemInterface>&
        getPowerSystemInputsInterface() const
    {
        return powerSystemInputsInterface;
    }

    /**
     * Returns the chassis status monitor for the system.
     *
     * @return monitor pointer (may be nullptr if not created)
     */
    const std::shared_ptr<ChassisStatusMonitor>& getSystemMonitor() const
    {
        return systemMonitor;
    }

    /**
     * Clears the error history for all GPIOs in this chassis.
     *
     * This should be called when the system reboots.
     */
    void clearErrorHistory();

    /**
     * Check if chassis is present via presence path
     *
     * @return true if file exists on the presence path, false otherwise
     */
    bool getPresenceFromPath() const;

    /**
     * Check if chassis is present, based on GPIO and presence path.
     *
     * @return true if chassis is present, false otherwise
     */
    bool getPresenceValue() const
    {
        return presenceValue;
    }

    /**
     * Initialize the chassis presence value to true. Only call this for the
     * chassis containing the running BMC.
     *
     */
    void initializePresence();

    /*
     * Check if chassis is present.
     *
     * @return true if chassis is present, false otherwise
     */
    bool isPresent();

    /**
     * Get the current state of the chassis.
     *
     * @return current chassis state
     */
    ChassisState getState() const
    {
        return currentState;
    }

    /**
     * Handle BMC reset scenario (R-PCP-2).
     * Manages GPIOs based on chassis presence and power state.
     *
     * @param bus D-Bus bus object
     */
    void handleBMCReset();

    /**
     * Handle system boot scenario (R-PCP-3).
     * Called when chassis power state changes.
     *
     * @param powerOn true if chassis powered on, false if powered off
     */
    void handlePowerStateChange(bool powerOn);

    /**
     * Handle power fault scenario (R-PCP-5).
     * Disables GPIOs and sets fault status.
     *
     * @param stdbyPowerFault true if this chassis latched fault enabled.
     */
    void handlePowerFault(bool stdbyPowerFault = false);

    /**
     * Enable or disable the reset enable and fault reset GPIOs.
     *
     * @param enable true to enable, false to disable
     * @param gpioNamePattern GPIO name pattern to use for getGpioByName()
     */
    void setGpiosEnabled(const std::string& gpioNamePattern, bool enable);

    /**
     * Check if chassis has a power fault.
     *
     * @param gpioNamePattern GPIO name pattern to use for getGpioByName()
     *
     * @return true if fault detected, false otherwise
     */
    bool hasPowerFault(const std::string& gpioNamePattern);

    /**
     * Check if chassis had lost standby power fault.
     *
     * @return true if fault detected, false otherwise
     */
    bool hasLostStandbyPowerFault();

    /**
     * Update the PowerSystemInputs status based on fault state.
     *
     * @param faulted true if faulted, false if good
     */
    void updatePowerSystemInputsStatus(bool faulted);

    /**
     * Get GPIO by type.
     *
     * @param type GPIO type to find
     * @return pointer to GPIO, or nullptr if not found
     */
    Gpio* getGpioByName(const std::string& namePattern);

    /**
     * Log a PEL for power fault.
     */
    void logPowerFaultPEL();

    /**
     * Initialize the chassis status monitor for this chassis.
     *
     * @param bus D-Bus bus object
     *
     * @return true if status monitor was created, false otherwise
     */
    bool initializeStatusMonitor(sdbusplus::bus_t& bus);

  private:

    /**
     * Callback function for D-Bus power state property changes.
     * Called when the power state or pgood property changes.
     *
     * @param message D-Bus message containing the property change
     */
    void powerStateChangeCallback(sdbusplus::message_t& message);

    /**
     * Timer callback for retrying handleBMCReset() when power status
     * cannot be determined.
     */
    void handleBMCResetTimerCallback();

    /**
     * Checks if a chassis is powered on.
     *
     * @return std::optional<bool> containing true if powered on, false if
     *         powered off, or std::nullopt if status cannot be determined
     */
    std::optional<bool> isChassisPoweredOn() const;

    /**
     * Chassis status monitor.
     *
     * Monitors chassis via D-Bus. May be null if initialization failed.
     */
    std::unique_ptr<phosphor::power::util::BMCChassisStatusMonitor>
        statusMonitor;

    /**
     * Reads the current and previous GPIO values, applies deglitching logic,
     * and updates the cached value if it has changed.
     *
     * @param gpio GPIO object to read from
     * @param gpioValue Reference to the cached GPIO value to update
     *
     * @return true if the GPIO value changed, false otherwise
     */
    bool gpioValueChanged(Gpio& gpio, std::optional<int>& gpioValue);

    /**
     * Checks if the system is powered on.
     *
     * @return true if the system is powered on, false otherwise.
     */
    bool isSystemPoweredOn() const;

    /**
     * System status monitor
     *
     * Shared pointer to monitor owned by System class.
     */
    std::shared_ptr<ChassisStatusMonitor> systemMonitor;

    /**
     * Notify Phosphor Inventory Manager of presence change
     *
     * @param bus D-Bus bus object
     * @param present True if chassis is present, false if missing
     */
    void notifyInventoryManager(sdbusplus::bus_t& bus, bool present);

    /**
     * Handle chassis presence gpio change
     *
     * @param readFailure True if GPIO read failed, false otherwise
     */
    void handlePresenceChange(bool readFailure);

    /**
     * Chassis number within the system.
     *
     * Chassis numbers start at 1 because chassis 0 represents the entire
     * system.
     */
    const unsigned int number{};

    /**
     * System services (D-Bus, GPIO, etc.).
     */
    Services& services;

    /**
     * Presence path for this chassis, if any.
     */
    const std::optional<std::string> presencePath{};

    /**
     * Cached presence value for this chassis.
     */
    bool presenceValue{false};

    /**
     * GPIO objects within this chassis, if any.
     *
     * The vector contains GPIO objects to perform operations.
     */
    std::vector<std::unique_ptr<Gpio>> gpios{};

    /**
     * Cached presence GPIO value for this chassis.
     */
    std::optional<int> presenceGPIOValue{};

    /**
     * Cached latched fault state for this chassis.
     */
    std::optional<int> faultLatchedValue{};

    /**
     * Cached unlatched fault state for this chassis.
     */
    std::optional<int> faultUnlatchedValue{};

    /**
     * Substring used to identify chassis presence GPIOs by name.
     */
    static constexpr std::string_view presenceName{"presence-chassis"};

    /**
     * Substring used to identify latched fault GPIOs by name.
     */
    static constexpr std::string_view faultLatchedName{"fault-latched"};

    /**
     * Substring used to identify unlatched fault GPIOs by name.
     */
    static constexpr std::string_view faultUnlatchedName{"fault-unlatched"};

    /**
     * D-Bus PowerSystemInputs interface for this chassis.
     */
    std::unique_ptr<ChassisPowerSystemInterface> powerSystemInputsInterface{};

    /**
     * Current state of the chassis.
     */
    ChassisState currentState{ChassisState::Missing};

    /**
     * Previous power state (pgood) of the chassis.
     */
    bool previousPowerState{false};

    bool NO_OVERRIDE{false};

    /**
     * Event loop for timer operations.
     */
    std::optional<sdeventplus::Event> eventLoop{};

    /**
     * Timer for retrying handleBMCReset() when power status cannot be determined.
     * Set to 60 seconds.
     */
    std::unique_ptr<sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        bmcResetRetryTimer{};

    /**
     * Flag to track if the BMC reset retry timer has already been used.
     * Ensures the timer only fires once.
     */
    bool bmcResetRetryTimerUsed{false};

    /**
     * D-Bus match object for monitoring power state changes.
     * Subscribes to PropertiesChanged signals on the power interface.
     */
    std::unique_ptr<sdbusplus::bus::match_t> powerStateMatch{};
};

} // namespace phosphor::power::chassis
