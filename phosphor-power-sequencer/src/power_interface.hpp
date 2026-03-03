/**
 * Copyright © 2021 IBM Corporation
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

#include "config.h"

#include <stddef.h> // for size_t
#include <systemd/sd-bus.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/vtable.hpp>

#include <string>

namespace phosphor::power::sequencer
{

/**
 * @class PowerInterface
 *
 * This class implements the org.openbmc.control.Power D-Bus interface.
 *
 * This interface is used to control the power state of a system or chassis.
 */
class PowerInterface
{
  public:
    PowerInterface() = delete;
    PowerInterface(const PowerInterface&) = delete;
    PowerInterface& operator=(const PowerInterface&) = delete;
    PowerInterface(PowerInterface&&) = delete;
    PowerInterface& operator=(PowerInterface&&) = delete;
    virtual ~PowerInterface() = default;

    /**
     * Constructor to put interface onto the specified D-Bus path.

     * @param bus D-Bus bus object
     * @param path D-Bus object path
     */
    PowerInterface(sdbusplus::bus_t& bus, const char* path);

    /**
     * Valid values for the 'state' property.
     */
    enum class PowerState : int
    {
        off = 0,
        on = 1
    };

    /**
     * Valid values for the 'pgood' property.
     */
    enum class PowerGood : int
    {
        off = 0,
        on = 1
    };

    /**
     * Converts the specified PowerState enumeration value to a string.
     *
     * @param powerState value in PowerState enumeration
     * @return string representation of value
     */
    static std::string toString(PowerState powerState)
    {
        if (powerState == PowerState::off)
        {
            return "off";
        }
        else
        {
            return "on";
        }
    }

    /**
     * Converts the specified PowerGood enumeration value to a string.
     *
     * @param powerGood value in PowerGood enumeration
     * @return string representation of value
     */
    static std::string toString(PowerGood powerGood)
    {
        if (powerGood == PowerGood::off)
        {
            return "off";
        }
        else
        {
            return "on";
        }
    }

    /**
     * Returns the chassis number.
     *
     * @return chassis number (0 for entire system)
     */
    size_t getChassisNumber() const
    {
        return chassisNumber;
    }

    /**
     * Sets the chassis number.
     *
     * @param number chassis number (0 for entire system)
     */
    void setChassisNumber(size_t number)
    {
        chassisNumber = number;
    }

    /**
     * Returns the value of the 'state' property.
     *
     * @return state property value
     */
    int getStateProperty() const
    {
        return state;
    }

    /**
     * Sets the value of the 'state' property.
     *
     * Note this does not power on or off the system or chassis. See
     * setPowerState().
     *
     * @param newState new value for state property
     * @param skipSignal indicates whether to skip sending D-Bus signal due to
     *                   the property change
     */
    void setStateProperty(int newState, bool skipSignal = false)
    {
        if (state != newState)
        {
            state = newState;
            if (!skipSignal)
            {
                emitPropertyChangedSignal("state");
            }
        }
    }

    /**
     * Returns the value of the 'pgood' property.
     *
     * @return pgood property value
     */
    int getPgoodProperty() const
    {
        return pgood;
    }

    /**
     * Sets the value of the 'pgood' property.
     *
     * @param newPgood new value for pgood property
     * @param skipSignals indicates whether to skip sending D-Bus signals due to
     *                   the property change
     */
    void setPgoodProperty(int newPgood, bool skipSignals = false)
    {
        if (pgood != newPgood)
        {
            pgood = newPgood;
            if (!skipSignals)
            {
                lg2::info(
                    "Chassis {CHASSIS_NUMBER}: D-Bus pgood property set to {PGOOD}",
                    "CHASSIS_NUMBER", chassisNumber, "PGOOD", newPgood);
                emitPropertyChangedSignal("pgood");
                if (pgood == static_cast<int>(PowerGood::off))
                {
                    emitPowerLostSignal();
                }
                else
                {
                    emitPowerGoodSignal();
                }
            }
        }
    }

    /**
     * Returns the value of the 'pgood_timeout' property.
     *
     * @return pgood_timeout property value in seconds
     */
    int getPgoodTimeoutProperty() const
    {
        return pgoodTimeout;
    }

    /**
     * Sets the value of the 'pgood_timeout' property.
     *
     * Note this does not change the power good timeout value used in
     * the system or chassis. See setPowerGoodTimeout().
     *
     * @param newTimeout new value for pgood_timeout property in seconds
     * @param skipSignal indicates whether to skip sending D-Bus signal due to
     *                   the property change
     */
    void setPgoodTimeoutProperty(int newTimeout, bool skipSignal = false)
    {
        if (pgoodTimeout != newTimeout)
        {
            pgoodTimeout = newTimeout;
            if (!skipSignal)
            {
                emitPropertyChangedSignal("pgood_timeout");
            }
        }
    }

    /**
     * Sets the requested power state for the system or chassis.
     *
     * Throws an exception if an error occurs.
     *
     * @param newState new power state
     */
    virtual void setPowerState(int newState) = 0;

    /**
     * Sets the power good timeout for the system or chassis.
     *
     * @param newTimeout new power good timeout value in seconds
     */
    virtual void setPowerGoodTimeout(int newTimeout) = 0;

    /**
     * Sets the power supply error for the system or chassis.
     *
     * The value should be a message argument for a phosphor-logging D-Bus
     * Create method, such as
     * "xyz.openbmc_project.Power.PowerSupply.Error.PSKillFault".
     *
     * Replaces any previously set power supply error.
     *
     * Any previously set power supply error is cleared during a power on
     * attempt.
     *
     * @param error Power supply error.
     */
    virtual void setPowerSupplyError(const std::string& error) = 0;

  protected:
    /**
     * D-Bus callback for getting the pgood property
     */
    static int callbackGetPgood(sd_bus* bus, const char* path,
                                const char* interface, const char* property,
                                sd_bus_message* msg, void* context,
                                sd_bus_error* ret_error);

    /**
     * D-Bus callback for getting the pgood_timeout property
     */
    static int callbackGetPgoodTimeout(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* msg, void* context,
        sd_bus_error* error);

    /**
     * D-Bus callback for the getPowerState method
     */
    static int callbackGetPowerState(sd_bus_message* msg, void* context,
                                     sd_bus_error* error);

    /**
     * D-Bus callback for getting the state property
     */
    static int callbackGetState(sd_bus* bus, const char* path,
                                const char* interface, const char* property,
                                sd_bus_message* msg, void* context,
                                sd_bus_error* error);

    /**
     * D-Bus callback for setting the pgood_timeout property
     */
    static int callbackSetPgoodTimeout(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* msg, void* context,
        sd_bus_error* error);

    /**
     * D-Bus callback for the setPowerSupplyError method
     */
    static int callbackSetPowerSupplyError(sd_bus_message* msg, void* context,
                                           sd_bus_error* error);

    /**
     * D-Bus callback for the setPowerState method
     */
    static int callbackSetPowerState(sd_bus_message* msg, void* context,
                                     sd_bus_error* error);

    /**
     * Emit the PowerGood signal.
     */
    void emitPowerGoodSignal();

    /**
     * Emit the PowerLost signal.
     */
    void emitPowerLostSignal();

    /**
     * Emit the PropertyChanged signal.

     * @param property the property that changed
     */
    void emitPropertyChangedSignal(const char* property);

    /**
     * Systemd vtable structure that contains all the
     * methods, signals, and properties of this interface with their
     * respective systemd attributes.
     */
    static const sdbusplus::vtable::vtable_t vtable[];

    /**
     * Holder for the instance of this interface to be on D-Bus.
     */
    sdbusplus::server::interface::interface serverInterface;

    /**
     * Chassis number. The number 0 is used for the entire system.
     */
    size_t chassisNumber{0};

    /**
     * Last requested power state for the system or chassis.
     */
    int state{0};

    /**
     * Power good value for the system or chassis.
     */
    int pgood{0};

    /**
     * Power good timeout in seconds.
     */
    int pgoodTimeout{PGOOD_TIMEOUT};
};

} // namespace phosphor::power::sequencer
