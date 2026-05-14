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

#include <chrono>
#include <filesystem>
#include <fstream>

namespace phosphor::power::chassis
{

using namespace phosphor::power::util;
using namespace std::string_literals;

bool Chassis::initializePowerSystemInputsInterface(sdbusplus::bus_t& bus)
{
    auto chassisInputPowerStatusPath =
        std::format(CHASSIS_INPUT_POWER_STATUS_PATH, number);

    // Create the D-Bus interface object for this chassis
    try
    {
        // TODO: Update to set status to fault when gpio reads are
        // implemented
        powerSystemInputsInterface =
            std::make_unique<ChassisPowerSystemInterface>(
                bus, chassisInputPowerStatusPath.c_str(),
                PowerSystemInputs::Status::Good);
        return true;
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to initialize PowerSystemInputs interface for chassis {CHASSIS}: {ERROR}",
            "CHASSIS", number, "ERROR", e.what());
        return false;
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

void Chassis::clearErrorHistory()
{
    for (const auto& gpio : gpios)
    {
        gpio->clearErrorHistory();
    }
}

//SHELDON: instead of setting up a monitor, the BMC reset should do the read,
//.         and we should upon good read set up the subscription to get changed event.
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

        // Set up D-Bus subscription for power state changes//SHELDON::BOB1 start
        auto chassisPowerPath = std::format(CHASSIS_POWER_PATH, number);

        auto matchRule = sdbusplus::bus::match::rules::propertiesChanged(
            chassisPowerPath, POWER_IFACE);

        powerStateMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus, matchRule,
            [this](sdbusplus::message_t& msg) {
                this->powerStateChangeCallback(msg);
            });

        lg2::info("Set up power state monitoring for chassis {CHASSIS}",
                  "CHASSIS", number);//SHELDON::BOB1 stop



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
        lg2::error("Chassis{CHASSIS} Status monitor not initialized",
            "CHASSIS", number);
        return std::nullopt;
    }

    try
    {
        return statusMonitor->isPoweredOn(); // SHELDON:?TODO: change this to do read, and remove Monitor, and if this reads enable callback!
    }
    catch (const std::exception& e)
    {
        lg2::error("Chassis{CHASSIS} isChassisPoweredOn(): {ERROR}",
            "CHASSIS", number, "ERROR", e.what());
        return std::nullopt;
    }
}

void Chassis::monitor()
{
    for (const auto& gpio : gpios)
    {
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
                    changed = gpioValueChanged(*gpio, presenceGPIOValue);
                    if (changed)
                    {
                        handlePresenceChange(false);
                    }
                }
                catch (...)
                {
                    // gpio read fail, handle presence change
                    handlePresenceChange(true);
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
                {
                    // Handle gpio read fail
                }

                if (changed)
                {
                    // Handle fault latched change
                    // lg2::info("SHELDON:TODO: faultLatchedName changed!");
                }
                gpio->release();//SHELDON:DEBUG:remove
            }
        }
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
                    //SHELDON:DEBUG: R-PCP-6 ????
                    if (gpio->getValue() == 1)
                    {
                        handlePowerFault(hasPowerFault("fault-latched"));
                    }
                }
                gpio->release();//SHELDON:DEBUG:remove
            }
        }
    }
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

bool Chassis::getPresenceFromPath() const
{
    if (!presencePath.has_value())
    {
        return false;
    }

    try
    {
        return std::filesystem::exists(presencePath.value());
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Error checking presence path for chassis {CHASSIS}: {ERROR}",
            "CHASSIS", number, "ERROR", e);
        return false;
    }
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


bool Chassis::isPresent()
{
    // Check presence via GPIO if configured
    Gpio* presenceGpio = getGpioByName("presence-chassis");
    if (presenceGpio != nullptr)
    {
        try
        {
            if (!presenceGpio->foundLine())
            {
                presenceGpio->findLine();
            }
            if (presenceGpio->foundLine() && presenceGpio->requestRead())
            {
                int value = presenceGpio->getValue();

                // GPIO is active (1 with active high polarity) means present
                if (value == 1)
                {
                    return true;
                }
            }
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Failed to read presence GPIO for chassis {CHASSIS}: {ERROR}",
                "CHASSIS", number, "ERROR", e.what());
        }
    }

    // Check presence via file path if configured
    if (presencePath.has_value())
    {
        if (std::filesystem::exists(presencePath.value()))
        {
            return true;
        }
    }

    // If neither method indicates presence, chassis is missing
    return false;
}

Gpio* Chassis::getGpioByName(const std::string& namePattern)
{
    for (auto& gpio : gpios)
    {
        if (gpio->getName().find(namePattern) != std::string::npos)
        {
            return gpio.get();
        }
    }
    return nullptr;
}

bool Chassis::hasPowerFault(const std::string& gpioNamePattern)
{

    // Check unlatched fault GPIO
    Gpio* faultUnlatchedGpio = getGpioByName(gpioNamePattern);
    if (faultUnlatchedGpio != nullptr)
    {
        try
        {
            if (!faultUnlatchedGpio->foundLine())
            {
                faultUnlatchedGpio->findLine();
            }
            if (faultUnlatchedGpio->foundLine() &&
                faultUnlatchedGpio->requestRead())
            {
                int value = faultUnlatchedGpio->getValue();

                if (gpioNamePattern == "fault-unlatched")
                {
                    // Active (1) means fault
                    if (value == 1)
                    {
                        lg2::error("----------------- power-chassis{CHASSIS}-standby-fault-unlatched:Sx_STBY_PG:{VALUE} -> ACTIVE -> BAD_SB_POWER",
                            "CHASSIS", number, "VALUE", value);
                        lg2::error("                      This signal is used to tell the live state of the sled. Does it have standby power right now?");
                        lg2::error("                      0 = vin bad, 1 = vin good");
                        return true;
                    }
                    lg2::error("----------------- power-chassis{CHASSIS}-standby-fault-unlatched:Sx_STBY_PG:{VALUE} -> not active -> GOOD_SB_POWER",
                            "CHASSIS", number, "VALUE", value);
                    lg2::error("                      This signal is used to tell the live state of the sled. Does it have standby power right now?");
                    lg2::error("                      0 = vin bad, 1 = vin good");
                }
                else
                {
                    // Active (1) means fault
                    if (value == 1)
                    {
                        lg2::error("----------------- power-chassis{CHASSIS}-standby-fault-latched:Sx_STBY_PG_FAULT_LATCHED_N:{VALUE} -> ACTIVE -> SB POWER_BAD",
                            "CHASSIS", number, "VALUE", value);
                        lg2::error("                      This signal tells the BMC which sled lost standby power.");
                        lg2::error("                      It is used so that we can log an error (741x) after the fact against the sled that caused the reset.");
                        lg2::error("                      0 = vin bad, 1 = vin good");
                    return true;
                    }
                    lg2::error("----------------- power-chassis{CHASSIS}-standby-fault-latched:Sx_STBY_PG_FAULT_LATCHED_N:{VALUE} -> not active -> SB POWER_GOOD",
                            "CHASSIS", number, "VALUE", value);
                    lg2::error("                      This signal tells the BMC which sled lost standby power.");
                    lg2::error("                      It is used so that we can log an error (741x) after the fact against the sled that caused the reset.");
                    lg2::error("                      0 = vin bad, 1 = vin good");
                }
            }
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Failed to read GPIO {PATTERN} for chassis {CHASSIS}: {ERROR}",
                "PATTERN", gpioNamePattern, "CHASSIS", number, "ERROR", e.what());
        }
    }
    return false;
}

void Chassis::setGpiosEnabled(const std::string& gpioNamePattern, bool enable)
{
    // Find GPIO by name
    Gpio* resetEnableGpio = getGpioByName(gpioNamePattern);

    if (resetEnableGpio != nullptr)
    {
        try
        {
            if (!resetEnableGpio->foundLine())
            {
                resetEnableGpio->findLine();
            }
            if (resetEnableGpio->foundLine())
            {
                int value = enable ? 1 : 0;
                if (resetEnableGpio->requestWrite(value))
                {
                    std::string gpioStatus = "disabled";
                    if (value == 1)
                    {
                        gpioStatus = "enabled";
                    }
                    lg2::info("       chassis{CHASSIS}:{PATTERN}:gpio-write({VALUE})-{STATUS}",
                            "CHASSIS", number, "PATTERN", gpioNamePattern, "VALUE", value,
                            "STATUS", gpioStatus);

                    resetEnableGpio->setValue(value);
                }
                resetEnableGpio->release();
            }
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Failed to set {PATTERN} GPIO for chassis {CHASSIS}: {ERROR}",
                "PATTERN", gpioNamePattern, "CHASSIS", number, "ERROR", e.what());
        }
    }
}

void Chassis::updatePowerSystemInputsStatus(bool faulted)
{
    if (powerSystemInputsInterface)
    {
        auto newStatus = faulted ? PowerSystemInputs::Status::Fault
                                 : PowerSystemInputs::Status::Good;
        powerSystemInputsInterface->status(newStatus);
        lg2::info("Set PowerSystemInputs status to {STATUS} for chassis "
                  "{CHASSIS}",
                  "STATUS",
                  (faulted ? "Fault" : "Good"),
                  "CHASSIS",
                  number);
    }
}

void Chassis::logPowerFaultPEL()
{
    lg2::info("Logging power fault PEL for chassis {CHASSIS}", "CHASSIS", number);

    std::map<std::string, std::string> additionalData;
    additionalData["CHASSIS_NUMBER"] = std::to_string(number);

    // Callout todo PFEBMC-5344
    additionalData["CALLOUT_INVENTORY_PATH"] =
        "/xyz/openbmc_project/inventory/system/chassis/" +
        std::to_string(number);
    additionalData["CALLOUT_PRIORITY"] = "H";

    services.logError(
        "xyz.openbmc_project.Power.ChassisPowerMissing",
        Entry::Level::Error, additionalData);
}

void Chassis::handleBMCResetTimerCallback()
{
    lg2::info("chassis{CHASSIS} {READDELAY} Sec. Timer expired, handleBMCResetTimerCallback() retrying handleBMCReset() ###############################################",
              "CHASSIS", number, "READDELAY", DbusReadDelay);

    // Mark timer as used to prevent it from being restarted
    bmcResetRetryTimerUsed = true;

    // Disable the timer to ensure it only fires once
    if (bmcResetRetryTimer)
    {
        bmcResetRetryTimer->setEnabled(false);
        lg2::info("chassis{CHASSIS} Disabled BMC reset retry timer",
                  "CHASSIS", number);
    }

    handleBMCReset();
}

void Chassis::handleBMCReset()
{
    // Check if chassis is present
    bool present = isPresent();

    lg2::info("SHELDON:DEBUG:handleBMCReset():chassis{CHASSIS}: Present:{PRESENT} ###############################################",
        "CHASSIS", number, "PRESENT", present);

    if (!present)
    {
        // For missing sleds, disable GPIOs
        currentState = ChassisState::Missing;
        setGpiosEnabled("reset-enable", false); // disable
        setGpiosEnabled("fault-reset", true);  // enable
        return;
    }

    // Chassis is present, check for fault
    // This signal is used to tell the live state of the sled. Does it have standby power
    if (hasPowerFault("fault-unlatched"))
    {
        // If fault detected, disable GPIOs and set status to Fault
        lg2::error("Chassis{CHASSIS} has power fault", "CHASSIS", number);
        currentState = ChassisState::Faulted;
        setGpiosEnabled("reset-enable", false); // disable
        setGpiosEnabled("fault-reset", true); // enable
        updatePowerSystemInputsStatus(true);
        return;
    }

    // Check chassis power state and power good from D-Bus
    auto powerStatus = isChassisPoweredOn();

    if (!powerStatus.has_value())
    {
        // Cannot determine power status - start 60 second timer to retry (only once)
        if (!bmcResetRetryTimerUsed)
        {
            currentState = ChassisState::Missing;
            // Create timer if it doesn't exist
            if (!bmcResetRetryTimer && eventLoop.has_value())
            {
                bmcResetRetryTimer = std::make_unique<
                    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
                    eventLoop.value(),
                    [this](auto&) { this->handleBMCResetTimerCallback(); });
            }

            // Start or restart the timer for 60 seconds
            if (bmcResetRetryTimer)
            {
                bmcResetRetryTimer->restartOnce(std::chrono::seconds(DbusReadDelay));
                lg2::error("Chassis{CHASSIS}, start {READDELAY} Second timer for attempt on reading pgood",
                    "CHASSIS", number, "READDELAY", DbusReadDelay);
            }
            else
            {
                lg2::error("Chassis{CHASSIS}, Failed to create timer", "CHASSIS", number);
                setGpiosEnabled("reset-enable", false); // disable
                setGpiosEnabled("fault-reset", false);  // disable
            }
        }
        else
        {
            // Timer already used, cannot determine power status after retry
            // Assume chassis is powered off
            lg2::error("Chassis{CHASSIS} pgood status timed out ({READDELAY} sec.), assuming it's Off",
                       "CHASSIS", number, "READDELAY", DbusReadDelay);
            currentState = ChassisState::Off;
            setGpiosEnabled("reset-enable", false); // disable
            setGpiosEnabled("fault-reset", true);   // enable
        }
        return;
    }
    // power status is false.
    else if (!powerStatus.value())
    {
        lg2::info("Chassis{CHASSIS} pgood status found as Off", "CHASSIS", number);
        currentState = ChassisState::Off;
        setGpiosEnabled("reset-enable", false); // disable
        setGpiosEnabled("fault-reset", true); // enable
    }
    // power status is true.
    else
    {
        lg2::info("Chassis{CHASSIS} pgood status found as On", "CHASSIS", number);
        currentState = ChassisState::On;
        setGpiosEnabled("reset-enable", true); // enable
        setGpiosEnabled("fault-reset", false); // disable
        updatePowerSystemInputsStatus(false); //SHELDON:QUESTION: where did this come from ? VALIDATE
    }
}

void Chassis::handlePowerStateChange(bool powerOn)
{
    lg2::info("handling power state change for chassis{CHASSIS}: {STATE}",
              "CHASSIS", number, "STATE", (powerOn ? "On" : "Off"));

    if (!isPresent())
    {
        lg2::info("Chassis {CHASSIS} is not present, ignoring power state change",
                  "CHASSIS", number);
        return;
    }

    // if powered on
    if (powerOn)
    {
        // R-PCP-3: Chassis is present, check for fault
        // This signal is used to tell the live state of the sled. Does it have standby power
        if (hasPowerFault("fault-unlatched"))
        {
            // Power fault detected during boot
            lg2::error(
                "Chassis {CHASSIS} failed to power on due to power fault",
                "CHASSIS", number);
            currentState = ChassisState::Faulted;
            setGpiosEnabled("reset-enable", false); // disable
            setGpiosEnabled("fault-reset", false); // disable
            updatePowerSystemInputsStatus(true);
            logPowerFaultPEL();

        }
        // else this is a clean power on.
        else
        {
            // Successful power on
            lg2::info("Chassis {CHASSIS} powered on successfully", "CHASSIS",
                      number);
            currentState = ChassisState::On;
            setGpiosEnabled("reset-enable", true); // enable
            setGpiosEnabled("fault-reset", true); // enable
            updatePowerSystemInputsStatus(false);
        }
    }
    // else powered off.
    else
    {
        // R-PCP-4: During system power off, disable GPIOs
        lg2::info("Chassis {CHASSIS} powered off, disabling GPIOs", "CHASSIS",
                  number);
        currentState = ChassisState::Off;
        setGpiosEnabled("reset-enable", false); // disable
        setGpiosEnabled("fault-reset", false); // disable
    }

    previousPowerState = powerOn;
}

void Chassis::handlePowerFault(bool stdbyPowerFault)
{
    // Runtime fault scenario

    if (!isPresent())
    {
        lg2::info("Chassis{CHASSIS} is not present, ignoring fault",
                  "CHASSIS", number);
        return;
    }

    std::string stbyFaultDetectedAnswer = "with";
    if (!stdbyPowerFault)
    {
        stbyFaultDetectedAnswer = "without";
        setGpiosEnabled("reset-enable", false); // disable
        setGpiosEnabled("fault-reset", true);   // enable
    }

    lg2::error("chassis{CHASSIS} power fault detected {STRING} loss of standby power",
        "CHASSIS", number, "STRING", stbyFaultDetectedAnswer);

    currentState = ChassisState::Faulted;
    updatePowerSystemInputsStatus(true);    // SHELDON:QUESTION: where did this come from?
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
            lg2::error("chassis{CHASSIS}: interface no POWER_IFACE", "CHASSIS", number);
            return;
        }

        // Check if power state or pgood changed
        bool isPoweredOn = false;

        auto pgoodIt = properties.find(POWER_GOOD_PROP);
        if (pgoodIt != properties.end())
        {
            isPoweredOn |= std::get<int>(pgoodIt->second);
        }


        if (isPoweredOn != previousPowerState)
        {
            lg2::error("SHELDON:DEBUG:B1 chassis{CHASSIS}:powerStateChangeCallback() : PRE:{PRESTATE}-> NEW:{NEWSTATE}",
                    "CHASSIS", number,
                    "NEWSTATE", (isPoweredOn ? "On" : "Off"),
                    "PRESTATE", (previousPowerState ? "On" : "Off"));

            previousPowerState = isPoweredOn;
            handlePowerStateChange(isPoweredOn);
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
    bool presencePathPresent = getPresenceFromPath();
    bool newPresence = presenceValue;

    if (presenceGPIOValue == 1 && presencePathPresent)
    {
        newPresence = true;
        lg2::info("Chassis {CHASSIS} confirmed present", "CHASSIS", number);
    }
    else if ((presenceGPIOValue == 0 || readFailure) && presencePathPresent)
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
        }

        newPresence = true;
    }
    else if ((presenceGPIOValue == 0 || readFailure) && !presencePathPresent)
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
    else if (presenceGPIOValue == 1 && !presencePathPresent)
    {
        newPresence = true;
    }

    if (newPresence != presenceValue)
    {
        presenceValue = newPresence;
        notifyInventoryManager(services.getBus(), presenceValue);
    }
}
} // namespace phosphor::power::chassis
