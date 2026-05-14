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

#include "chassis.hpp"

#include "types.hpp"
#include "utility.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/clock.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace phosphor::power::chassis
{

using namespace phosphor::power::util;
using namespace std::string_literals;

bool Chassis::initializePowerSystemInputsInterface(
    PowerSystemInputs::Status initialStatus)
{
    auto chassisInputPowerStatusPath =
        std::format(CHASSIS_INPUT_POWER_STATUS_PATH, number);

    // Create the D-Bus interface object for this chassis
    try
    {
        powerSystemInputsInterface =
            std::make_unique<ChassisPowerSystemInterface>(
                services.getBus(), chassisInputPowerStatusPath.c_str(),
                initialStatus);
        return true;
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to initialize PowerSystemInputs interface for chassis {CHASSIS}: {ERROR}",
            "CHASSIS", number, "ERROR", e);
        return false;
    }
}

void Chassis::setPowerSystemInputsStatus(PowerSystemInputs::Status status)
{
    if (!powerSystemInputsInterface)
    {
        initializePowerSystemInputsInterface(status);
    }
    else
    {
        powerSystemInputsInterface->status(status);
    }
}

void Chassis::setSystemStatusMonitor(
    const std::shared_ptr<ChassisStatusMonitor>& monitor)
{
    systemMonitor = monitor;
}

bool Chassis::isSystemPoweredOn() const
{
    if (!systemMonitor)
    {
        lg2::error("System monitor not initialized for chassis {CHASSIS}",
                   "CHASSIS", number);
        return false;
    }

    try
    {
        return systemMonitor->getPowerGood();
    }
    catch (const std::exception& e)
    {
        return false;
    }
}
void Chassis::startLatchedFaultCheck()
{
    latchedFaultPELLogged = false;
    checkLatchedFaultPending = true;
    checkLatchedFault();
}

void Chassis::checkLatchedFault()
{
    if (faultLatchedValue.has_value())
    {
        if (faultLatchedValue.value() == 1)
        {
            if (handleLatchedFault())
            {
                checkLatchedFaultPending = false;
            }
        }
        else
        {
            checkLatchedFaultPending = false;
        }
    }
}

void Chassis::clearErrorHistory()
{
    for (const auto& gpio : gpios)
    {
        gpio->clearErrorHistory();
    }
}

bool Chassis::initializeStatusMonitor(sdbusplus::bus_t& bus)
{
    try
    {
        phosphor::power::util::ChassisStatusMonitorOptions options;
        options.isPowerGoodMonitored = true;
        options.isPowerStateMonitored = true;

        auto inventoryPath = std::format(
            "/xyz/openbmc_project/inventory/system/chassis{}", number);

        statusMonitor =
            std::make_unique<phosphor::power::util::BMCChassisStatusMonitor>(
                bus, number, inventoryPath, options);

        // Set up D-Bus subscription for power state changes
        auto chassisPowerPath = std::format(CHASSIS_POWER_PATH, number);

        auto matchRule = sdbusplus::bus::match::rules::propertiesChanged(
            chassisPowerPath, POWER_IFACE);

        powerStateMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus, matchRule, [this](sdbusplus::message_t& msg) {
                this->powerStateChangeCallback(msg);
            });

        lg2::info("Set up power state monitoring for chassis {CHASSIS}",
                  "CHASSIS", number);

        return true;
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to initialize status monitor for chassis {CHASSIS}: {ERROR}",
            "CHASSIS", number, "ERROR", e);
        return false;
    }
}

std::optional<bool> Chassis::isChassisPoweredOn() const
{
    if (!statusMonitor)
    {
        lg2::error("Chassis{CHASSIS} Status monitor not initialized", "CHASSIS",
                   number);
        return std::nullopt;
    }

    try
    {
        return statusMonitor->isPoweredOn();
    }
    catch (const std::exception& e)
    {
        lg2::error("Chassis{CHASSIS} isChassisPoweredOn(): {ERROR}", "CHASSIS",
                   number, "ERROR", e.what());
        return std::nullopt;
    }
}

void Chassis::monitor()
{
    bool presenceGPIOChanged = false;
    bool presenceGPIOReadFailure = false;
    std::optional<bool> oldPresencePathValue = presencePathValue;

    getPresenceFromPath();
    bool presencePathChanged = (presencePathValue != oldPresencePathValue);

    for (const auto& gpio : gpios)
    {
        if (gpio->getDirection() == GpioDirection::Output)
        {
            continue;
        }

        const std::string& name = gpio->getName();

        if (!gpio->foundLine())
        {
            if (!gpio->findLine())
            {
                continue;
            }
        }

        bool changed = false;

        if (name.contains(presenceName))
        {
            if (gpio->requestRead())
            {
                try
                {
                    presenceGPIOChanged =
                        gpioValueChanged(*gpio, presenceGPIOValue);
                }
                catch (...)
                {
                    presenceGPIOReadFailure = true;
                }
                // Other apps will need to read this line.
                gpio->release();
            }
        }
        else if (name.contains(faultLatchedName))
        {
            if (gpio->requestRead())
            {
                try
                {
                    changed = gpioValueChanged(*gpio, faultLatchedValue);
                }
                catch (...)
                {}
                if (checkLatchedFaultPending)
                {
                    checkLatchedFault();
                    // Handle gpio read fail
                }

                if (changed)
                {
                    // Handle fault latched change
                    // lg2::info("SHELDON:TODO: faultLatchedName changed!");
                }
            }
        }
        // SHELDON:DEBUG: what is up with this presenceValue ??
        // else if (name.contains(faultUnlatchedName) && presenceValue)
        else if (name.contains(faultUnlatchedName))
        {
            if (gpio->requestRead())
            {
                try
                {
                    changed = gpioValueChanged(*gpio, faultUnlatchedValue);
                }
                catch (...)
                {
                    // Handle gpio read fail
                }

                if (changed)
                {
                    // Handle fault unlatched change
                    auto status = (faultUnlatchedValue == 1)
                                      ? PowerSystemInputs::Status::Fault
                                      : PowerSystemInputs::Status::Good;
                    setPowerSystemInputsStatus(status);
                    if (faultUnlatchedValue == 1)
                    {
                        // THIS IS A POWER FAULT
                        writeGpioByName("reset-enable", 0);
                        writeGpioByName("fault-reset", 1);

                        lg2::error(
                            "chassis{CHASSIS} power fault detected loss of standby power",
                            "CHASSIS", number);

                        currentState = ChassisState::Faulted;
                    }
                }
            }
        }
    }

    if (presenceGPIOChanged || presencePathChanged || presenceGPIOReadFailure)
    {
        handlePresenceChange(presenceGPIOReadFailure);
    }
}

bool Chassis::handleLatchedFault()
{
    if (!latchedFaultPELLogged)
    {
        lg2::info("Chassis {CHASSIS} handling latched fault", "CHASSIS",
                  number);

        std::map<std::string, std::string> additionalData{
            {"CHASSIS_NUMBER", std::to_string(number)}};

        services.logError(
            "xyz.openbmc_project.Power.BMC.Reset.ChassisPreviouslyLostPower",
            Entry::Level::Error, additionalData);

        latchedFaultPELLogged = true;
    }

    auto* gpio = getGpioByName(faultResetName);
    if (gpio == nullptr)
    {
        lg2::error(
            "Chassis {CHASSIS}: fault-reset GPIO not found; cannot reset latched fault",
            "CHASSIS", number);
        return true;
    }

    return (writeGPIO(*gpio, 1) && writeAndReleaseGPIO(*gpio, 0));
}

bool Chassis::writeAndReleaseGPIO(Gpio& gpio, int value)
{
    if (!writeGPIO(gpio, value))
    {
        return false;
    }
    gpio.release();
    return true;
}

bool Chassis::writeGPIO(Gpio& gpio, int value)
{
    if (!gpio.foundLine())
    {
        gpio.findLine();
    }

    if (gpio.requestWrite(value))
    {
        try
        {
            gpio.setValue(value);
            return true;
        }
        catch (...)
        {}
    }

    return false;
}

void Chassis::writeGpioByName(const std::string& gpioNamePattern, int enable)
{
    // Find GPIO by name
    Gpio* gpio = getGpioByName(gpioNamePattern);

    if (gpio != nullptr)
    {
        writeGPIO(*gpio, enable);
    }
}

Gpio* Chassis::getGpioByName(const std::string_view name) const
{
    for (const auto& gpio : gpios)
    {
        if (gpio->getName().contains(name))
        {
            return gpio.get();
        }
    }

    return nullptr;
}

bool Chassis::gpioValueChanged(Gpio& gpio, std::optional<int>& gpioValue)
{
    int value;
    int previousValue;
    value = gpio.getValue();

    try
    {
        previousValue = gpio.getPreviousValue();
    }
    catch (...)
    {
        // No previous value available, use current value as new value
        if (value != gpioValue)
        {
            gpioValue = value;
            return true;
        }
        return false;
    }

    // Get deglitched value: use current if it matches previous,
    // otherwise keep the cached value
    int newGPIOValue =
        (value == previousValue) ? value : gpioValue.value_or(value);

    if (newGPIOValue != gpioValue)
    {
        // Update value
        gpioValue = newGPIOValue;
        return true;
    }
    return false;
}

std::optional<bool> Chassis::getPresenceFromPath()
{
    if (!presencePath.has_value())
    {
        return std::nullopt;
    }

    try
    {
        presencePathValue = std::filesystem::exists(presencePath.value());
    }
    catch (const std::exception& e)
    {
        presencePathValue = false;
        lg2::error(
            "Error checking presence path for chassis {CHASSIS}: {ERROR}",
            "CHASSIS", number, "ERROR", e);
    }

    return presencePathValue;
}

void Chassis::notifyInventoryManager(sdbusplus::bus_t& bus, bool present)
{
    try
    {
        auto invPath = std::format("/system/chassis{}", number);
        // Get the inventory manager service
        auto invMgrService =
            getService(INVENTORY_OBJ_PATH, INVENTORY_MGR_IFACE, bus, false);
        if (invMgrService.empty())
        {
            lg2::error("Inventory manager not available for chassis {CHASSIS}",
                       "CHASSIS", number);
            return;
        }

        // Build the property map for Notify
        DbusPropertyMap properties;
        properties[PRESENT_PROP] = present;

        // Build the interface map
        std::map<std::string, DbusPropertyMap> interfaces;
        interfaces[INVENTORY_IFACE] = std::move(properties);

        // Build the object map with object_path key for Notify
        std::map<sdbusplus::object_path, std::map<std::string, DbusPropertyMap>>
            objectMap;
        objectMap[sdbusplus::object_path(invPath)] = std::move(interfaces);

        // Call Notify method on inventory manager
        auto method =
            bus.new_method_call(invMgrService.c_str(), INVENTORY_OBJ_PATH,
                                INVENTORY_MGR_IFACE, "Notify");
        method.append(objectMap);
        bus.call(method);

        lg2::info("Notified PIM chassis {CHASSIS} Present is {PRESENT}",
                  "CHASSIS", number, "PRESENT", present);
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to notify inventory manager of Present property for chassis {CHASSIS}: {ERROR}",
            "CHASSIS", number, "ERROR", e);
    }
}

void Chassis::handleBMCResetTimerCallback()
{
    lg2::info(
        "chassis{CHASSIS} {READDELAY} Sec. Timer expired, handleBMCResetTimerCallback() retrying handleBMCReset() ###############################################",
        "CHASSIS", number, "READDELAY", DbusReadDelay);

    // Mark timer as used to prevent it from being restarted
    bmcResetRetryTimerUsed = true;

    // Disable the timer to ensure it only fires once
    if (bmcResetRetryTimer)
    {
        bmcResetRetryTimer->setEnabled(false);
        lg2::info("chassis{CHASSIS} Disabled BMC reset retry timer", "CHASSIS",
                  number);
    }

    handleBMCReset();
}

void Chassis::handleBMCReset()
{
    lg2::info(
        "handleBMCReset():chassis{CHASSIS}: Present:{PRESENT} ###############################################",
        "CHASSIS", number, "PRESENT", presenceValue);

    // Check if chassis is NOT present
    if (!presenceValue)
    {
        // For missing sleds, disable GPIOs
        currentState = ChassisState::Missing;
        writeGpioByName("reset-enable", 0);
        writeGpioByName("fault-reset", 1);
        return;
    }

    if (faultUnlatchedValue == 1)
    {
        // If fault detected, disable GPIOs and set status to Fault
        lg2::error("Chassis{CHASSIS} has power fault", "CHASSIS", number);
        currentState = ChassisState::Faulted;
        writeGpioByName("reset-enable", 0);
        writeGpioByName("fault-reset", 1);
        setPowerSystemInputsStatus(PowerSystemInputs::Status::Fault);
        return;
    }

    // Check chassis power state and power good from D-Bus
    auto powerStatus = isChassisPoweredOn();

    if (!powerStatus.has_value())
    {
        // Cannot determine power status - start 60 second timer to retry (only
        // once)
        if (!bmcResetRetryTimerUsed)
        {
            currentState = ChassisState::Missing;
            // Create timer if it doesn't exist
            if (!bmcResetRetryTimer && eventLoop.has_value())
            {
                bmcResetRetryTimer =
                    std::make_unique<sdeventplus::utility::Timer<
                        sdeventplus::ClockId::Monotonic>>(
                        eventLoop.value(),
                        [this](auto&) { this->handleBMCResetTimerCallback(); });
            }

            // Start or restart the timer for 60 seconds
            if (bmcResetRetryTimer)
            {
                bmcResetRetryTimer->restartOnce(
                    std::chrono::seconds(DbusReadDelay));
                lg2::error(
                    "Chassis{CHASSIS}, start {READDELAY} Second timer for attempt on reading pgood",
                    "CHASSIS", number, "READDELAY", DbusReadDelay);
            }
            else
            {
                lg2::error("Chassis{CHASSIS}, Failed to create timer",
                           "CHASSIS", number);
                writeGpioByName("reset-enable", 0);
                writeGpioByName("fault-reset", 0);
            }
        }
        else
        {
            // Timer already used, cannot determine power status after retry
            // Assume chassis is powered off
            lg2::error(
                "Chassis{CHASSIS} pgood status timed out ({READDELAY} sec.), assuming it's Off",
                "CHASSIS", number, "READDELAY", DbusReadDelay);
            currentState = ChassisState::Off;
            writeGpioByName("reset-enable", 0);
            writeGpioByName("fault-reset", 1);
        }
        return;
    }
    // power status is false.
    else if (!powerStatus.value())
    {
        lg2::info("Chassis{CHASSIS} pgood status found as Off", "CHASSIS",
                  number);
        currentState = ChassisState::Off;
        writeGpioByName("reset-enable", 0);
        writeGpioByName("fault-reset", 1);
    }
    // power status is true.
    else
    {
        lg2::info("Chassis{CHASSIS} pgood status found as On", "CHASSIS",
                  number);
        currentState = ChassisState::On;
        writeGpioByName("reset-enable", 1);
        writeGpioByName("fault-reset", 0);
        setPowerSystemInputsStatus(PowerSystemInputs::Status::Good);
    }
}

void Chassis::handlePowerStateChange(bool powerOn)
{
    lg2::info("handling power state change for chassis{CHASSIS}: {STATE}",
              "CHASSIS", number, "STATE", (powerOn ? "On" : "Off"));

    if (!presenceValue)
    {
        lg2::info(
            "Chassis {CHASSIS} is not present, ignoring power state change",
            "CHASSIS", number);
        return;
    }

    // if powered on
    if (powerOn)
    {
        // R-PCP-3: Chassis is present, check for fault
        // This signal is used to tell the live state of the sled. Does it have
        // standby power
        if (faultUnlatchedValue == 1)
        {
            // Power fault detected during boot
            lg2::error(
                "Chassis {CHASSIS} failed to power on due to power fault",
                "CHASSIS", number);
            currentState = ChassisState::Faulted;
            writeGpioByName("reset-enable", 0);
            writeGpioByName("fault-reset", 0);
            setPowerSystemInputsStatus(PowerSystemInputs::Status::Fault);

            // SHELDON:TODO: Description of error log.
            // SHELDON:TODO: handler with services ????
            lg2::info("Logging power fault PEL for chassis {CHASSIS}",
                      "CHASSIS", number);

            std::map<std::string, std::string> additionalData;
            additionalData["CHASSIS_NUMBER"] = std::to_string(number);

            // Callout todo PFEBMC-5344
            additionalData["CALLOUT_INVENTORY_PATH"] =
                "/xyz/openbmc_project/inventory/system/chassis/" +
                std::to_string(number);
            additionalData["CALLOUT_PRIORITY"] = "H";

            services.logError("xyz.openbmc_project.Power.ChassisPowerMissing",
                              Entry::Level::Error, additionalData);
        }
        // else this is a clean power on.
        else
        {
            // Successful power on
            lg2::info("Chassis {CHASSIS} powered on successfully", "CHASSIS",
                      number);
            currentState = ChassisState::On;
            writeGpioByName("reset-enable", 1);
            writeGpioByName("fault-reset", 1);
            setPowerSystemInputsStatus(PowerSystemInputs::Status::Good);
        }
    }
    // else powered off.
    else
    {
        // if powered off from powered on state.
        // SHELDON:TEST: if (currentState == ChassisState::On)
        if (previousPowerState == true)
        {
            // R-PCP-4: During system power off, disable GPIOs
            // SHELDON:TODO:
            // from BRENDAN:
            // This is a MISS.. up in Manager there is a system
            //               level power state check.. move there, and call all
            //               chassis.
            // from BRENDAN: This isn't checking the system power state.
            // isSystemPoweredOn() ????   possibly this ????
            lg2::info("Chassis {CHASSIS} powered off from on, disabling GPIOs",
                      "CHASSIS", number);
            currentState = ChassisState::Faulted;
            writeGpioByName("reset-enable", 0);
            writeGpioByName("fault-reset", 1);
            // SHELDON:QUESTION: is below needed?
            setPowerSystemInputsStatus(PowerSystemInputs::Status::Fault);
        }
        // else this is powered off from powered off or faulted state.
        else
        {
            // R-PCP-4: During system power off, disable GPIOs
            lg2::info("Chassis {CHASSIS} powered off, disabling GPIOs",
                      "CHASSIS", number);
            currentState = ChassisState::Off;
            writeGpioByName("reset-enable", 0);
            writeGpioByName("fault-reset", 0);
            // SHELDON:QUESTION: is below needed?
            setPowerSystemInputsStatus(PowerSystemInputs::Status::Good);
        }
    }

    previousPowerState = powerOn;
}

void Chassis::powerStateChangeCallback(sdbusplus::message_t& message)
{
    try
    {
        std::string interface;
        std::map<std::string, std::variant<int>> properties;

        message.read(interface, properties);

        if (interface != POWER_IFACE)
        {
            lg2::error("chassis{CHASSIS}: interface no POWER_IFACE", "CHASSIS",
                       number);
            return;
        }

        // Check if power state or pgood changed
        bool isPoweredOn = false;

        auto pgoodIt = properties.find(POWER_GOOD_PROP);
        if (pgoodIt != properties.end())
        {
            isPoweredOn |= std::get<int>(pgoodIt->second);
            // SHELDON:QUESTION: should this change to -> isChassisPoweredOn()
        }

        if (isPoweredOn != previousPowerState)
        {
            lg2::error(
                "SHELDON:DEBUG:B1 chassis{CHASSIS}:powerStateChangeCallback() : PRE:{PRESTATE}-> NEW:{NEWSTATE}",
                "CHASSIS", number, "NEWSTATE", (isPoweredOn ? "On" : "Off"),
                "PRESTATE", (previousPowerState ? "On" : "Off"));

            previousPowerState = isPoweredOn;
            handlePowerStateChange(isPoweredOn); // SHELDON:DEBUG:change to DH.
            // SHELDON:QUESTION: what is 'DH' from sheldons note above?
        }
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Error processing power state change for chassis{CHASSIS}: {ERROR}",
            "CHASSIS", number, "ERROR", e.what());
    }
}

void Chassis::initializePresence()
{
    presenceValue = true;
}

void Chassis::handlePresenceChange(bool readFailure)
{
    bool newPresence = presenceValue;

    // Presence Path not defined
    if (!presencePath.has_value())
    {
        newPresence = (presenceGPIOValue == 1) && !readFailure;

        if (!newPresence && presenceValue && isSystemPoweredOn())
        {
            std::map<std::string, std::string> data;
            data["CHASSIS_NUMBER"] = std::to_string(number);

            services.logError(
                "xyz.openbmc_project.Power.Chassis.Missing.ShouldBePresent",
                Entry::Level::Error, data);
        }
    }
    // Both true
    else if (presenceGPIOValue == 1 && presencePathValue.value())
    {
        newPresence = true;
        lg2::info("Chassis {CHASSIS} confirmed present", "CHASSIS", number);
    }
    // Both false
    else if ((presenceGPIOValue == 0 || readFailure) &&
             !presencePathValue.value())
    {
        lg2::info("Chassis {CHASSIS} confirmed absent", "CHASSIS", number);

        if (presenceValue && isSystemPoweredOn())
        {
            // Only log error if system is on and chassis is present,
            // to avoid flooding the error log on a powered-off system.
            std::map<std::string, std::string> data;
            data["CHASSIS_NUMBER"] = std::to_string(number);

            if (presencePath.has_value())
            {
                data["PRESENCE_PATH"] = presencePath.value();
            }

            // Callout the specific chassis that went missing
            services.logError(
                "xyz.openbmc_project.Power.Chassis.Missing.ShouldBePresent",
                Entry::Level::Error, data);
        }

        newPresence = false;
    }
    // Both have different values
    else if ((presenceGPIOValue == 0 || readFailure) &&
             presencePathValue.value())
    {
        if (presenceValue && isSystemPoweredOn())
        {
            // Only log error if system is on and chassis is present,
            // to avoid flooding the error log on a powered-off system.
            std::map<std::string, std::string> data;
            data["CHASSIS_NUMBER"] = std::to_string(number);
            if (presencePath.has_value())
            {
                data["PRESENCE_PATH"] = presencePath.value();
            }

            // Callout the system
            services.logError(
                "xyz.openbmc_project.Power.Chassis.PresentDetection.Incorrect",
                Entry::Level::Error, data);

            writeGpioByName("reset-enable", 0);
            writeGpioByName("fault-reset", 1);
        }

        newPresence = true;
    }
    else if (presenceGPIOValue == 1 && !presencePathValue.value())
    {
        newPresence = true;
    }
    // GPIO not defined
    else if (!presenceGPIOValue.has_value())
    {
        newPresence = presencePathValue.value();
        if (!presencePathValue.value() && isSystemPoweredOn())
        {
            std::map<std::string, std::string> data;
            data["CHASSIS_NUMBER"] = std::to_string(number);

            if (presencePath.has_value())
            {
                data["PRESENCE_PATH"] = presencePath.value();
            }

            // Callout the specific chassis that went missing
            services.logError(
                "xyz.openbmc_project.Power.Chassis.Missing.ShouldBePresent",
                Entry::Level::Error, data);
        }
    }

    if (newPresence != presenceValue || !presenceInitialized)
    {
        presenceValue = newPresence;
        presenceInitialized = true;
        notifyInventoryManager(services.getBus(), presenceValue);
    }
}

} // namespace phosphor::power::chassis
