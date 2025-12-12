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
#pragma once

#include "utility.hpp"

#include <stddef.h> // for size_t

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/State/Decorator/PowerSystemInputs/server.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace phosphor::power::util
{

using PowerSystemInputs = sdbusplus::server::xyz::openbmc_project::state::
    decorator::PowerSystemInputs;

/**
 * @struct ChassisStatusMonitorOptions
 *
 * Options that define what types of monitoring are enabled in a
 * ChassisStatusMonitor object.
 */
struct ChassisStatusMonitorOptions
{
    /**
     * Specifies whether to monitor the Present property in the
     * xyz.openbmc_project.Inventory.Item interface for the chassis inventory
     * path.
     */
    bool isPresentMonitored{false};

    /**
     * Specifies whether to monitor the Available property in the
     * xyz.openbmc_project.State.Decorator.Availability interface for the
     * chassis inventory path.
     */
    bool isAvailableMonitored{false};

    /**
     * Specifies whether to monitor the Enabled property in the
     * xyz.openbmc_project.Object.Enable interface for the chassis inventory
     * path.
     */
    bool isEnabledMonitored{false};

    /**
     * Specifies whether to monitor the state property in the
     * org.openbmc.control.Power interface for the chassis.
     */
    bool isPowerStateMonitored{false};

    /**
     * Specifies whether to monitor the pgood property in the
     * org.openbmc.control.Power interface for the chassis.
     */
    bool isPowerGoodMonitored{false};

    /**
     * Specifies whether to monitor the Status property in the
     * xyz.openbmc_project.State.Decorator.PowerSystemInputs interface for the
     * status of input power to the chassis.
     */
    bool isInputPowerStatusMonitored{false};

    /**
     * Specifies whether to monitor the Status property in the
     * xyz.openbmc_project.State.Decorator.PowerSystemInputs interface for the
     * status of power supplies power to the chassis.
     */
    bool isPowerSuppliesStatusMonitored{false};
};

/**
 * @class ChassisStatusMonitor
 *
 * Abstract base class for monitoring the state of a chassis.
 *
 * The chassis state is monitored using D-Bus interfaces and properties. The
 * types of monitoring are configured by specifying a
 * ChassisStatusMonitorOptions instance to the sub-class constructor.
 */
class ChassisStatusMonitor
{
  public:
    ChassisStatusMonitor() = default;
    ChassisStatusMonitor(const ChassisStatusMonitor&) = delete;
    ChassisStatusMonitor(ChassisStatusMonitor&&) = delete;
    ChassisStatusMonitor& operator=(const ChassisStatusMonitor&) = delete;
    ChassisStatusMonitor& operator=(ChassisStatusMonitor&&) = delete;
    virtual ~ChassisStatusMonitor() = default;

    /**
     * Returns the chassis number being monitored.
     *
     * Chassis numbers start at 1 since chassis 0 represents the entire system.
     *
     * @return chassis number
     */
    virtual size_t getNumber() const = 0;

    /**
     * Returns the chassis inventory path being monitored.
     *
     * @return chassis inventory path
     */
    virtual const std::string& getInventoryPath() const = 0;

    /**
     * Returns the options defining which types of monitoring are enabled.
     *
     * @return monitoring options
     */
    virtual const ChassisStatusMonitorOptions& getOptions() const = 0;

    /**
     * Returns whether the chassis is present.
     *
     * Returns true if monitoring this property is not enabled.
     *
     * Throws an exception if the property value could not be obtained.
     *
     * @return true is chassis is present, false otherwise
     */
    virtual bool isPresent() = 0;

    /**
     * Returns whether the chassis is available.
     *
     * Returns true if monitoring this property is not enabled.
     *
     * Throws an exception if the property value could not be obtained.
     *
     * @return true is chassis is available, false otherwise
     */
    virtual bool isAvailable() = 0;

    /**
     * Returns whether the chassis is enabled.
     *
     * If the Enabled property is false, it means that the chassis has been put
     * in hardware isolation (guarded).
     *
     * Returns true if monitoring this property is not enabled.
     *
     * Throws an exception if the property value could not be obtained.
     *
     * @return true is chassis is enabled, false otherwise
     */
    virtual bool isEnabled() = 0;

    /**
     * Returns the chassis power state.
     *
     * This is the last requested power state.
     *
     * Throws an exception if monitoring this property is not enabled or the
     * property value could not be obtained.
     *
     * @return 0 if power off requested, 1 if power on requested
     */
    virtual int getPowerState() = 0;

    /**
     * Returns the chassis power good status.
     *
     * This indicates whether the chassis has been successfully powered on
     * from a hardware perspective (chassis pgood asserted).
     *
     * Throws an exception if monitoring this property is not enabled or the
     * property value could not be obtained.
     *
     * @return 0 if chassis is powered off, 1 is chassis powered on
     */
    virtual int getPowerGood() = 0;

    /**
     * Returns whether this chassis is powered on.
     *
     * Requires both power good and power state monitoring.
     *
     * Throws an exception if power good or power state monitoring is not
     * enabled. Throws an exception if the property values could not be
     * obtained.
     *
     * @return true if chassis is powered on, false otherwise
     */
    virtual bool isPoweredOn() = 0;

    /**
     * Returns whether this chassis is powered off.
     *
     * Requires both power good and power state monitoring.
     *
     * Throws an exception if power good or power state monitoring is not
     * enabled. Throws an exception if the property values could not be
     * obtained.
     *
     * @return true if chassis is powered off, false otherwise
     */
    virtual bool isPoweredOff() = 0;

    /**
     * Returns the chassis input power status.
     *
     * Returns PowerSystemInputs::Status::Good if this monitoring is not
     * enabled.
     *
     * Throws an exception if the property value could not be obtained.
     *
     * @return chassis input power status
     */
    virtual PowerSystemInputs::Status getInputPowerStatus() = 0;

    /**
     * Returns whether the chassis input power status is good.
     *
     * Returns true if this monitoring is not enabled.
     *
     * Throws an exception if the property value could not be obtained.
     *
     * @return true if chassis input power is good, false otherwise
     */
    virtual bool isInputPowerGood() = 0;

    /**
     * Returns the power supplies power status.
     *
     * Returns PowerSystemInputs::Status::Good if this monitoring is not
     * enabled.
     *
     * Throws an exception if the property value could not be obtained.
     *
     * @return power supplies power status
     */
    virtual PowerSystemInputs::Status getPowerSuppliesStatus() = 0;

    /**
     * Returns whether the power supplies power status is good.
     *
     * Returns true if this monitoring is not enabled.
     *
     * Throws an exception if the property value could not be obtained.
     *
     * @return true if power supplies power is good, false otherwise
     */
    virtual bool isPowerSuppliesPowerGood() = 0;
};

/**
 * @class BMCChassisStatusMonitor
 *
 * Implementation of the ChassisStatusMonitor interface using the standard BMC
 * APIs.
 */
class BMCChassisStatusMonitor : public ChassisStatusMonitor
{
  public:
    BMCChassisStatusMonitor() = delete;
    BMCChassisStatusMonitor(const BMCChassisStatusMonitor&) = delete;
    BMCChassisStatusMonitor(BMCChassisStatusMonitor&&) = delete;
    BMCChassisStatusMonitor& operator=(const BMCChassisStatusMonitor&) = delete;
    BMCChassisStatusMonitor& operator=(BMCChassisStatusMonitor&&) = delete;
    virtual ~BMCChassisStatusMonitor() = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object.
     * @param number Chassis number within the system. Must be >= 1.
     * @param inventoryPath D-Bus inventory path of the chassis.
     * @param options Options that specify what types of monitoring are enabled.
     */
    BMCChassisStatusMonitor(sdbusplus::bus_t& bus, size_t number,
                            const std::string& inventoryPath,
                            const ChassisStatusMonitorOptions& options);

    /** @copydoc ChassisStatusMonitor::getNumber() */
    virtual size_t getNumber() const override
    {
        return number;
    }

    /** @copydoc ChassisStatusMonitor::getInventoryPath() */
    virtual const std::string& getInventoryPath() const override
    {
        return inventoryPath;
    }

    /** @copydoc ChassisStatusMonitor::getOptions() */
    virtual const ChassisStatusMonitorOptions& getOptions() const override
    {
        return options;
    }

    /** @copydoc ChassisStatusMonitor::isPresent() */
    virtual bool isPresent() override
    {
        if (!options.isPresentMonitored)
        {
            return true;
        }
        if (!isPresentValue)
        {
            throw std::runtime_error{
                "Present property value could not be obtained."};
        }
        return *isPresentValue;
    }

    /** @copydoc ChassisStatusMonitor::isAvailable() */
    virtual bool isAvailable() override
    {
        if (!options.isAvailableMonitored)
        {
            return true;
        }
        if (!isAvailableValue)
        {
            throw std::runtime_error{
                "Available property value could not be obtained."};
        }
        return *isAvailableValue;
    }

    /** @copydoc ChassisStatusMonitor::isEnabled() */
    virtual bool isEnabled() override
    {
        if (!options.isEnabledMonitored)
        {
            return true;
        }
        if (!isEnabledValue)
        {
            throw std::runtime_error{
                "Enabled property value could not be obtained."};
        }
        return *isEnabledValue;
    }

    /** @copydoc ChassisStatusMonitor::getPowerState() */
    virtual int getPowerState() override
    {
        if (!options.isPowerStateMonitored)
        {
            throw std::runtime_error{
                "Power state property value is not being monitored."};
        }
        if (!powerStateValue)
        {
            throw std::runtime_error{
                "Power state property value could not be obtained."};
        }
        return *powerStateValue;
    }

    /** @copydoc ChassisStatusMonitor::getPowerGood() */
    virtual int getPowerGood() override
    {
        if (!options.isPowerGoodMonitored)
        {
            throw std::runtime_error{
                "Power good property value is not being monitored."};
        }
        if (!powerGoodValue)
        {
            throw std::runtime_error{
                "Power good property value could not be obtained."};
        }
        return *powerGoodValue;
    }

    /** @copydoc ChassisStatusMonitor::isPoweredOn() */
    virtual bool isPoweredOn() override
    {
        return ((getPowerState() == 1) && (getPowerGood() == 1));
    }

    /** @copydoc ChassisStatusMonitor::isPoweredOff() */
    virtual bool isPoweredOff() override
    {
        return ((getPowerState() == 0) && (getPowerGood() == 0));
    }

    /** @copydoc ChassisStatusMonitor::getInputPowerStatus() */
    virtual PowerSystemInputs::Status getInputPowerStatus() override
    {
        if (!options.isInputPowerStatusMonitored)
        {
            return PowerSystemInputs::Status::Good;
        }
        if (!inputPowerStatusValue)
        {
            throw std::runtime_error{
                "Input power Status property value could not be obtained."};
        }
        return PowerSystemInputs::convertStatusFromString(
            *inputPowerStatusValue);
    }

    /** @copydoc ChassisStatusMonitor::isInputPowerGood() */
    virtual bool isInputPowerGood() override
    {
        return (getInputPowerStatus() == PowerSystemInputs::Status::Good);
    }

    /** @copydoc ChassisStatusMonitor::getPowerSuppliesStatus() */
    virtual PowerSystemInputs::Status getPowerSuppliesStatus() override
    {
        if (!options.isPowerSuppliesStatusMonitored)
        {
            return PowerSystemInputs::Status::Good;
        }
        if (!powerSuppliesStatusValue)
        {
            throw std::runtime_error{
                "Power supplies power Status property value could not be obtained."};
        }
        return PowerSystemInputs::convertStatusFromString(
            *powerSuppliesStatusValue);
    }

    /** @copydoc ChassisStatusMonitor::isPowerSuppliesPowerGood() */
    virtual bool isPowerSuppliesPowerGood() override
    {
        return (getPowerSuppliesStatus() == PowerSystemInputs::Status::Good);
    }

  protected:
    /**
     * Add a NameOwnerChanged match for the specified service.
     *
     * @param service D-Bus service
     */
    void addNameOwnerChangedMatch(const std::string& service)
    {
        matches.emplace_back(std::make_unique<sdbusplus::bus::match_t>(
            bus, sdbusplus::bus::match::rules::nameOwnerChanged(service),
            std::bind_front(&BMCChassisStatusMonitor::nameOwnerChangedCallback,
                            this)));
    }

    /**
     * Add an InterfacesAdded match for the specified D-Bus path.
     *
     * @param path D-Bus path
     */
    void addInterfacesAddedMatch(const std::string& path)
    {
        matches.emplace_back(std::make_unique<sdbusplus::bus::match_t>(
            bus, sdbusplus::bus::match::rules::interfacesAddedAtPath(path),
            std::bind_front(&BMCChassisStatusMonitor::interfacesAddedCallback,
                            this)));
    }

    /**
     * Add an PropertiesChanged match for the specified D-Bus path and
     * interface.
     *
     * @param path D-Bus path
     * @param interface D-Bus interface
     */
    void addPropertiesChangedMatch(const std::string& path,
                                   const std::string& interface)
    {
        matches.emplace_back(std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::propertiesChanged(path, interface),
            std::bind_front(&BMCChassisStatusMonitor::propertiesChangedCallback,
                            this)));
    }

    /**
     * Add D-Bus matches to get signals for NameOwnerChanged, InterfacesAdded,
     * and PropertiesChanged.
     */
    void addMatches();

    /**
     * Try to get the specified D-Bus property.
     *
     * If an error occurs, it is ignored and the specified optionalValue is not
     * modified.
     *
     * @param service D-Bus service
     * @param path D-Bus path
     * @param interface D-Bus interface
     * @param propertyName D-Bus property name
     * @param optionalValue Property value is stored here if obtained
     */
    template <typename T>
    void getProperty(const std::string& service, const std::string& path,
                     const std::string& interface,
                     const std::string& propertyName,
                     std::optional<T>& optionalValue);

    /**
     * Try to get properties from the inventory manager service.
     *
     * If an error occurs, it is ignored and the property values remain
     * undefined.
     */
    void getInventoryManagerProperties();

    /**
     * Try to get properties from the power sequencer service.
     *
     * If an error occurs, it is ignored and the property values remain
     * undefined.
     */
    void getPowerSequencerProperties();

    /**
     * Try to get properties from the chassis input power service.
     *
     * If an error occurs, it is ignored and the property values remain
     * undefined.
     */
    void getChassisInputPowerProperties();

    /**
     * Try to get properties from the power supply service.
     *
     * If an error occurs, it is ignored and the property values remain
     * undefined.
     */
    void getPowerSupplyProperties();

    /**
     * Try to get all properties that are being monitored.
     *
     * If an error occurs, it is ignored and the property values remain
     * undefined.
     */
    void getAllProperties()
    {
        getInventoryManagerProperties();
        getPowerSequencerProperties();
        getChassisInputPowerProperties();
        getPowerSupplyProperties();
    }

    /**
     * Stores the value of the specified property.
     *
     * Does nothing if the property name is not found in the properties map.
     *
     * @param properties Map of D-Bus property names and values
     * @param propertyName Property name to store
     * @param optionalValue Property value is stored in this parameter
     */
    template <typename T>
    void storeProperty(const DbusPropertyMap& properties,
                       const std::string& propertyName,
                       std::optional<T>& optionalValue);

    /**
     * Stores the values of all relevant interface properties found in the
     * properties map.
     *
     * Does nothing if no relevant properties found.
     *
     * @param properties Map of D-Bus property names and values
     * @param path D-Bus path
     * @param interface D-Bus interface
     */
    void storeProperties(const DbusPropertyMap& properties,
                         const std::string& path, const std::string& interface);

    /**
     * Callback function for NameOwnerChanged D-Bus signals.
     *
     * @param message D-Bus message
     */
    void nameOwnerChangedCallback(sdbusplus::message_t& message);

    /**
     * Callback function for InterfacesAdded D-Bus signals.
     *
     * @param message D-Bus message
     */
    void interfacesAddedCallback(sdbusplus::message_t& message);

    /**
     * Callback function for PropertiesChanged D-Bus signals.
     *
     * @param message D-Bus message
     */
    void propertiesChangedCallback(sdbusplus::message_t& message);

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t& bus;

    /**
     * Chassis number within the system.
     *
     * Chassis numbers start at 1 because chassis 0 represents the entire
     * system.
     */
    size_t number{1};

    /**
     * D-Bus inventory path of the chassis.
     */
    std::string inventoryPath{};

    /**
     * Options that define what types of monitoring are enabled.
     */
    ChassisStatusMonitorOptions options{};

    /**
     * D-Bus path where the state and pgood properties exist for this chassis.
     */
    std::string chassisPowerPath{};

    /**
     * D-Bus path where the input power Status property exists for this chassis.
     */
    std::string chassisInputPowerStatusPath{};

    /**
     * D-Bus path where the power supplies power Status property exists for this
     * chassis.
     */
    std::string powerSuppliesStatusPath{};

    /**
     * Present property value.
     *
     * Contains no value if the property value has not been obtained.
     */
    std::optional<bool> isPresentValue{};

    /**
     * Available property value.
     *
     * Contains no value if the property value has not been obtained.
     */
    std::optional<bool> isAvailableValue{};

    /**
     * Enabled property value.
     *
     * Contains no value if the property value has not been obtained.
     */
    std::optional<bool> isEnabledValue{};

    /**
     * Power state property value.
     *
     * Contains no value if the property value has not been obtained.
     */
    std::optional<int> powerStateValue{};

    /**
     * Power good (pgood) property value.
     *
     * Contains no value if the property value has not been obtained.
     */
    std::optional<int> powerGoodValue{};

    /**
     * Input power Status property value represented as a string.
     *
     * Contains no value if the property value has not been obtained.
     */
    std::optional<std::string> inputPowerStatusValue{};

    /**
     * Power supplies power Status property value represented as a string.
     *
     * Contains no value if the property value has not been obtained.
     */
    std::optional<std::string> powerSuppliesStatusValue{};

    /**
     * Match objects created to get NameOwnerChanged, InterfacesAdded, and
     * PropertiesChanged signals.
     */
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> matches{};
};

} // namespace phosphor::power::util
