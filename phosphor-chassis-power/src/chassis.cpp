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
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

#include <filesystem>

namespace phosphor::power::chassis
{

using namespace phosphor::power::util;

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
                }
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
                }
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
