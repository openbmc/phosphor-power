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
#include <iostream> //SHELDON:DEBUG:

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

// SHELDON: instead of setting up a monitor, the BMC reset should do the read,
//.         and we should upon good read set up the subscription to get changed
// event.
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
    for (const auto& gpio : gpios)
    {
        if (gpio->getDirection() == GpioDirection::Output)
        {
            continue;
        }

        const std::string& name = gpio->getName();
        std::cout << "SHELDON:A1:monitor()->Name:" << name << " \n";

        if (!gpio->foundLine())
        {
            if (!gpio->findLine())
            {
                std::cout << "SHELDON:    FAIL FAIL FAIL \n";
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
                {}
                if (checkLatchedFaultPending)
                {
                    std::cout << "SHELDON:    CATCH \n";

                    checkLatchedFault();
                    // Handle gpio read fail
                }

                if (changed)
                {
                    // Handle fault latched change
                    // lg2::info("SHELDON:TODO: faultLatchedName changed!");
                    std::cout << "SHELDON:                        :CHGD \n";
                }
                else // SHELDON:DEBUG:START:
                {
                    std::cout << "SHELDON:                        :NOT CHGD \n";
                } // SHELDON:DEBUG:STOP:
            }
        }
        else if (name.contains(faultUnlatchedName))
        {
            if (gpio->requestRead())
            {
                try
                {
                    changed = gpioValueChanged(*gpio, faultUnlatchedValue);
                    std::cout << "SHELDON:VALUE:AFTER:"
                              << faultUnlatchedValue.value_or(-1) << "\n";
                }
                catch (...)
                {
                    std::cout << "SHELDON:        CATCH \n";
                    // Handle gpio read fail
                }

                if (changed)
                {
                    std::cout << "SHELDON:                        :CHGD \n";
                    // Handle fault unlatched change
                    auto status = (faultUnlatchedValue == 1)
                                      ? PowerSystemInputs::Status::Fault
                                      : PowerSystemInputs::Status::Good;
                    setPowerSystemInputsStatus(status);
                    if (faultUnlatchedValue == 1)
                    {
                        std::cout
                            << "SHELDON:   FAULED -- FAULTED -- FAULTED\n";
                        // THIS IS A POWER FAULT
                        writeGpioByName("reset-enable", 0);
                        writeGpioByName("fault-reset", 1);

                        lg2::error(
                            "chassis{CHASSIS} power fault detected loss of standby power",
                            "CHASSIS", number);

                        currentState = ChassisState::Faulted;
                    }
                }
                else // SHELDON:DEBUG:START:
                {
                    std::cout << "SHELDON:                        :NOT CHGD \n";

                } // SHELDON:DEBUG:STOP:
            }
        }
        else // SHELDON:DEBUG:START:
        {
            std::cout << "SHELDON:    FAILED ! FAILED ! FAILED ! \n";
        } // SHELDON:DEBUG:STOP:
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
    std::cout << "SHELDON:writeGpioByName()----------->" << gpioNamePattern
              << ":" << enable << " \n";

    // Find GPIO by name
    Gpio* gpio = getGpioByName(gpioNamePattern);

    if (gpio != nullptr)
    {
        std::cout << "SHELDON:writeGpioByName()--->writeGPIO() \n";
        writeGPIO(*gpio, enable);
    }
    std::cout << "SHELDON:writeGpioByName()-----------<" << gpioNamePattern
              << ":" << enable << " \n";
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
        std::cout << "SHELDON:gpioValueChanged() catch \n";
        // No previous value available, use current value as new value
        if (value != gpioValue)
        {
            std::cout
                << "SHELDON:no PREVIOUS VALUE value now equal to gpioValue \n";
            gpioValue = value;
            return true;
        }
        std::cout << "SHELDON: catch with value == gpioValue \n";
        return false;
    }

    // Get deglitched value: use current if it matches previous,
    // otherwise keep the cached value
    int newGPIOValue =
        (value == previousValue) ? value : gpioValue.value_or(value);

    std::cout << "SHELDON:################################################# \n";
    std::cout << "SHELDON:PI->VALUE:             gpioValue:"
              << gpioValue.value_or(-1) << "\n";
    std::cout << "SHELDON:getValue():                value:" << value << "\n";
    std::cout << "SHELDON:getPreviousValue():previousValue:" << previousValue
              << "\n";
    std::cout << "SHELDON:deglitched:         newGPIOValue:" << newGPIOValue
              << "\n";
    std::cout << "SHELDON:################################################# \n";

    if (newGPIOValue != gpioValue)
    {
        std::cout
            << "SHELDON:gpioValueChanged() if newGPIOValue != PI->gpioValue \n";
        // Update value
        gpioValue = newGPIOValue;
        return true;
    }
    std::cout
        << "SHELDON:gpioValueChanged() if newGPIOValue == PI->gpioValue \n";
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

// SHELDON:TODO:
// from BRENDAN: I don't like this name just because there are two power fault
//      pels: currently missing and previously lost.
void Chassis::logPowerFaultPEL()
{
    lg2::info("Logging power fault PEL for chassis {CHASSIS}", "CHASSIS",
              number);

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
    std::cout << "SHELDON:DEBUG:handleBMCReset()->HERE-HERE-HERE \n";
    lg2::info(
        "SHELDON:DEBUG:handleBMCReset():chassis{CHASSIS}: Present:{PRESENT} ###############################################",
        "CHASSIS", number, "PRESENT", presenceValue);

    // Check if chassis is NOT present
    if (!presenceValue)
    {
        std::cout << "SHELDON:TODO:handleBMCReset()-> NOT PRESENT \n";
        // For missing sleds, disable GPIOs
        currentState = ChassisState::Missing;
        writeGpioByName("reset-enable", 0);
        writeGpioByName("fault-reset", 1);
        return;
    }
    else // SHELDON:DEBUG:START:
    {
        std::cout
            << "SHELDON:DEBUG:handleBMCReset()-> PRESENT PRESENT PRESENT \n";
    } // SHELDON:DEBUG:STOP:

    // SHELDON:DELETE: this case of clearing will be done the next 1Sec. Timer.
    // Chassis is present, check for fault
    // This signal is used to tell the live state of the sled. Does it have
    // standby power
    if (faultLatchedValue)
    {
        std::cout
            << "SHELDON:DELETE:handleBMCReset()->fault-latched HAS FAULT \n";
    }

    if (faultUnlatchedValue == 1)
    {
        std::cout << "SHELDON:handleBMCReset()->fault-unlatched HAS FAULT \n";
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
        std::cout << "SHELDON:TODO:handleBMCReset()-> NO POWER STATUS \n";
        // Cannot determine power status - start 60 second timer to retry (only
        // once)
        if (!bmcResetRetryTimerUsed)
        {
            std::cout << "SHELDON:TODO:handleBMCReset()-> MISSING set timer \n";
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
            std::cout << "SHELDON:TODO:handleBMCReset()-> timer used \n";
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
        std::cout << "SHELDON:handleBMCReset()->NO POWER \n";
        lg2::info("Chassis{CHASSIS} pgood status found as Off", "CHASSIS",
                  number);
        currentState = ChassisState::Off;
        writeGpioByName("reset-enable", 0);
        writeGpioByName("fault-reset", 1);
    }
    // power status is true.
    else
    {
        std::cout << "SHELDON:handleBMCReset()->POWERED ---- POWERED \n";
        lg2::info("Chassis{CHASSIS} pgood status found as On", "CHASSIS",
                  number);
        currentState = ChassisState::On;
        writeGpioByName("reset-enable", 1);
        writeGpioByName("fault-reset", 0);
        setPowerSystemInputsStatus(PowerSystemInputs::Status::Good);
    }
    std::cout << "SHELDON:handleBMCReset()->END \n";
}

void Chassis::handlePowerStateChange(bool powerOn)
{
    std::cout << "SHELDON:handlePowerStateChange()->HERE-HERE-HERE \n";
    lg2::info("handling power state change for chassis{CHASSIS}: {STATE}",
              "CHASSIS", number, "STATE", (powerOn ? "On" : "Off"));

    // SHELDON:DELETE: if (!isPresent())
    if (!presenceValue)
    {
        std::cout << "SHELDON:TODO:handlePowerStateChange()-> NOT PRESENT \n";
        lg2::info(
            "Chassis {CHASSIS} is not present, ignoring power state change",
            "CHASSIS", number);
        return;
    }
    else // SHELDON:DEBUG:START:
    {
        std::cout
            << "SHELDON:TODO:handlePowerStateChange()-> PRESENT PRESENT PRESENT \n";
    } // SHELDON:DEBUG:STOP:

    // if powered on
    if (powerOn)
    {
        std::cout << "SHELDON:handlePowerStateChange()->powered ON ON ON \n";
        // R-PCP-3: Chassis is present, check for fault
        // This signal is used to tell the live state of the sled. Does it have
        // standby power
        if (faultUnlatchedValue == 1)
        {
            std::cout
                << "SHELDON:handlePowerStateChange() :  UNLATCHED FAULT !! \n";
            // Power fault detected during boot
            lg2::error(
                "Chassis {CHASSIS} failed to power on due to power fault",
                "CHASSIS", number);
            currentState = ChassisState::Faulted;
            writeGpioByName("reset-enable", 0);
            writeGpioByName("fault-reset", 0);
            setPowerSystemInputsStatus(PowerSystemInputs::Status::Fault);
            logPowerFaultPEL();
        }
        // else this is a clean power on.
        else
        {
            std::cout << "SHELDON:handlePowerStateChange()->Good Power On \n";
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
            std::cout
                << "SHELDON:handlePowerStateChange()->powered off from On Dis,En \n";
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
            std::cout
                << "SHELDON:handlePowerStateChange()->powered off from Off Dis,Dis \n";
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
    bool presencePathPresent = getPresenceFromPath();
    bool newPresence = presenceValue;

    // if GPIO present && present path
    if (presenceGPIOValue == 1 && presencePathPresent)
    {
        newPresence = true;
        lg2::info("Chassis {CHASSIS} confirmed present", "CHASSIS", number);
    }
    // else if read failure and present path
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
    // else if read failure and no presence
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

            writeGpioByName("reset-enable", 0);
            writeGpioByName("fault-reset", 1);
        }

        newPresence = false;
    }
    // else if GPIO present and not present path.
    // SHELDON:QUESTION: can not this be an OR case on the if?
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
