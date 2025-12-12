/**
 * Copyright © 2025 IBM Corporation
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

#include "chassis_status_monitor.hpp"

#include "types.hpp"

#include <format>

namespace phosphor::power::util
{

constexpr auto INVENTORY_MGR_SERVICE = "xyz.openbmc_project.Inventory.Manager";
constexpr auto POWER_SEQUENCER_SERVICE = "org.openbmc.control.Power";
constexpr auto CHASSIS_INPUT_POWER_SERVICE =
    "xyz.openbmc_project.Power.Chassis";
constexpr auto POWER_SUPPLY_SERVICE = "xyz.openbmc_project.Power.PSUMonitor";

constexpr auto CHASSIS_POWER_PATH = "/org/openbmc/control/power{}";
constexpr auto CHASSIS_INPUT_POWER_STATUS_PATH =
    "/xyz/openbmc_project/power/chassis/chassis{}";
constexpr auto POWER_SUPPLIES_STATUS_PATH =
    "/xyz/openbmc_project/power/power_supplies/chassis{}/psus";

BMCChassisStatusMonitor::BMCChassisStatusMonitor(
    sdbusplus::bus_t& bus, size_t number, const std::string& inventoryPath,
    const ChassisStatusMonitorOptions& options) :
    bus{bus}, number{number}, inventoryPath{inventoryPath}, options{options}
{
    chassisPowerPath = std::format(CHASSIS_POWER_PATH, number);
    chassisInputPowerStatusPath =
        std::format(CHASSIS_INPUT_POWER_STATUS_PATH, number);
    powerSuppliesStatusPath = std::format(POWER_SUPPLIES_STATUS_PATH, number);
    addMatches();
    getAllProperties();
}

void BMCChassisStatusMonitor::addMatches()
{
    if (options.isPresentMonitored || options.isAvailableMonitored ||
        options.isEnabledMonitored)
    {
        addNameOwnerChangedMatch(INVENTORY_MGR_SERVICE);
        addInterfacesAddedMatch(inventoryPath);
        if (options.isPresentMonitored)
        {
            addPropertiesChangedMatch(inventoryPath, INVENTORY_IFACE);
        }
        if (options.isAvailableMonitored)
        {
            addPropertiesChangedMatch(inventoryPath, AVAILABILITY_IFACE);
        }
        if (options.isEnabledMonitored)
        {
            addPropertiesChangedMatch(inventoryPath, ENABLE_IFACE);
        }
    }

    if (options.isPowerStateMonitored || options.isPowerGoodMonitored)
    {
        addNameOwnerChangedMatch(POWER_SEQUENCER_SERVICE);
        addInterfacesAddedMatch(chassisPowerPath);
        addPropertiesChangedMatch(chassisPowerPath, POWER_IFACE);
    }

    if (options.isInputPowerStatusMonitored)
    {
        addNameOwnerChangedMatch(CHASSIS_INPUT_POWER_SERVICE);
        addInterfacesAddedMatch(chassisInputPowerStatusPath);
        addPropertiesChangedMatch(chassisInputPowerStatusPath,
                                  POWER_SYSTEM_INPUTS_IFACE);
    }

    if (options.isPowerSuppliesStatusMonitored)
    {
        addNameOwnerChangedMatch(POWER_SUPPLY_SERVICE);
        addInterfacesAddedMatch(powerSuppliesStatusPath);
        addPropertiesChangedMatch(powerSuppliesStatusPath,
                                  POWER_SYSTEM_INPUTS_IFACE);
    }
}

template <typename T>
void BMCChassisStatusMonitor::getProperty(
    const std::string& service, const std::string& path,
    const std::string& interface, const std::string& propertyName,
    std::optional<T>& optionalValue)
{
    try
    {
        T value;
        util::getProperty(interface, propertyName, path, service, bus, value);
        optionalValue = value;
    }
    catch (...)
    {}
}

void BMCChassisStatusMonitor::getInventoryManagerProperties()
{
    if (options.isPresentMonitored)
    {
        getProperty(INVENTORY_MGR_SERVICE, inventoryPath, INVENTORY_IFACE,
                    PRESENT_PROP, isPresentValue);
    }

    if (options.isAvailableMonitored)
    {
        getProperty(INVENTORY_MGR_SERVICE, inventoryPath, AVAILABILITY_IFACE,
                    AVAILABLE_PROP, isAvailableValue);
    }

    if (options.isEnabledMonitored)
    {
        getProperty(INVENTORY_MGR_SERVICE, inventoryPath, ENABLE_IFACE,
                    ENABLED_PROP, isEnabledValue);
    }
}

void BMCChassisStatusMonitor::getPowerSequencerProperties()
{
    if (options.isPowerStateMonitored)
    {
        getProperty(POWER_SEQUENCER_SERVICE, chassisPowerPath, POWER_IFACE,
                    POWER_STATE_PROP, powerStateValue);
    }

    if (options.isPowerGoodMonitored)
    {
        getProperty(POWER_SEQUENCER_SERVICE, chassisPowerPath, POWER_IFACE,
                    POWER_GOOD_PROP, powerGoodValue);
    }
}

void BMCChassisStatusMonitor::getChassisInputPowerProperties()
{
    if (options.isInputPowerStatusMonitored)
    {
        getProperty(CHASSIS_INPUT_POWER_SERVICE, chassisInputPowerStatusPath,
                    POWER_SYSTEM_INPUTS_IFACE, STATUS_PROP,
                    inputPowerStatusValue);
    }
}

void BMCChassisStatusMonitor::getPowerSupplyProperties()
{
    if (options.isPowerSuppliesStatusMonitored)
    {
        getProperty(POWER_SUPPLY_SERVICE, powerSuppliesStatusPath,
                    POWER_SYSTEM_INPUTS_IFACE, STATUS_PROP,
                    powerSuppliesStatusValue);
    }
}

template <typename T>
void BMCChassisStatusMonitor::storeProperty(const DbusPropertyMap& properties,
                                            const std::string& propertyName,
                                            std::optional<T>& optionalValue)
{
    try
    {
        auto it = properties.find(propertyName);
        if (it != properties.end())
        {
            optionalValue = std::get<T>(it->second);
        }
    }
    catch (...)
    {}
}

void BMCChassisStatusMonitor::storeProperties(const DbusPropertyMap& properties,
                                              const std::string& path,
                                              const std::string& interface)
{
    try
    {
        if (interface == INVENTORY_IFACE)
        {
            storeProperty(properties, PRESENT_PROP, isPresentValue);
        }
        else if (interface == AVAILABILITY_IFACE)
        {
            storeProperty(properties, AVAILABLE_PROP, isAvailableValue);
        }
        else if (interface == ENABLE_IFACE)
        {
            storeProperty(properties, ENABLED_PROP, isEnabledValue);
        }
        else if (interface == POWER_IFACE)
        {
            storeProperty(properties, POWER_STATE_PROP, powerStateValue);
            storeProperty(properties, POWER_GOOD_PROP, powerGoodValue);
        }
        else if (interface == POWER_SYSTEM_INPUTS_IFACE)
        {
            if (path == chassisInputPowerStatusPath)
            {
                storeProperty(properties, STATUS_PROP, inputPowerStatusValue);
            }
            else if (path == powerSuppliesStatusPath)
            {
                storeProperty(properties, STATUS_PROP,
                              powerSuppliesStatusValue);
            }
        }
    }
    catch (...)
    {}
}

void BMCChassisStatusMonitor::nameOwnerChangedCallback(
    sdbusplus::message_t& message)
{
    try
    {
        std::string name, oldOwner, newOwner;
        message.read(name, oldOwner, newOwner);
        if (!newOwner.empty())
        {
            if (name == INVENTORY_MGR_SERVICE)
            {
                getInventoryManagerProperties();
            }
            else if (name == POWER_SEQUENCER_SERVICE)
            {
                getPowerSequencerProperties();
            }
            else if (name == CHASSIS_INPUT_POWER_SERVICE)
            {
                getChassisInputPowerProperties();
            }
            else if (name == POWER_SUPPLY_SERVICE)
            {
                getPowerSupplyProperties();
            }
        }
    }
    catch (...)
    {}
}

void BMCChassisStatusMonitor::interfacesAddedCallback(
    sdbusplus::message_t& message)
{
    try
    {
        sdbusplus::message::object_path path;
        std::map<std::string, DbusPropertyMap> interfaces;
        message.read(path, interfaces);
        for (const auto& [interface, properties] : interfaces)
        {
            storeProperties(properties, path, interface);
        }
    }
    catch (...)
    {}
}

void BMCChassisStatusMonitor::propertiesChangedCallback(
    sdbusplus::message_t& message)
{
    try
    {
        std::string interface;
        DbusPropertyMap changedProperties;
        std::vector<std::string> invalidatedProperties;
        message.read(interface, changedProperties, invalidatedProperties);
        storeProperties(changedProperties, message.get_path(), interface);
    }
    catch (...)
    {}
}

} // namespace phosphor::power::util
