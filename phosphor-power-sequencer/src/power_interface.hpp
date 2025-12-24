#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/vtable.hpp>

#include <string>

namespace phosphor::power::sequencer
{

/**
 * @class PowerControl
 * This class provides the org.openbmc.control.Power D-Bus interface.
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
     * Constructor to put object onto bus at a dbus path.
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
     * Emit the power good signal
     */
    void emitPowerGoodSignal();

    /**
     * Emit the power lost signal
     */
    void emitPowerLostSignal();

    /**
     * Emit the property changed signal
     * @param property the property that changed
     */
    void emitPropertyChangedSignal(const char* property);

    /**
     * Returns the power good of the chassis
     * @return power good
     */
    virtual int getPgood() const = 0;

    /**
     * Returns the power good timeout
     * @return power good timeout
     */
    virtual int getPgoodTimeout() const = 0;

    /**
     * Returns the value of the last requested power state
     * @return power state. A power on request is value 1. Power off is 0.
     */
    virtual int getState() const = 0;

    /**
     * Sets the power good timeout
     * @param timeout power good timeout
     */
    virtual void setPgoodTimeout(int timeout) = 0;

    /**
     * Initiates a chassis power state change
     * @param state power state. Request power on with a value of 1. Request
     * power off with a value of 0. Other values will be rejected.
     */
    virtual void setState(int state) = 0;

    /**
     * Sets the power supply error.
     * @param error power supply error. The value should be a message
     * argument for a phosphor-logging Create call, e.g.
     * "xyz.openbmc_project.Power.PowerSupply.Error.PSKillFault"
     */
    virtual void setPowerSupplyError(const std::string& error) = 0;

  private:
    /**
     * Holder for the instance of this interface to be on dbus
     */
    sdbusplus::server::interface::interface serverInterface;

    /**
     * Systemd vtable structure that contains all the
     * methods, signals, and properties of this interface with their
     * respective systemd attributes
     */
    static const sdbusplus::vtable::vtable_t vtable[];

    /**
     * Systemd bus callback for getting the pgood property
     */
    static int callbackGetPgood(sd_bus* bus, const char* path,
                                const char* interface, const char* property,
                                sd_bus_message* msg, void* context,
                                sd_bus_error* ret_error);

    /**
     * Systemd bus callback for getting the pgood_timeout property
     */
    static int callbackGetPgoodTimeout(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* msg, void* context,
        sd_bus_error* error);

    /**
     * Systemd bus callback for the getPowerState method
     */
    static int callbackGetPowerState(sd_bus_message* msg, void* context,
                                     sd_bus_error* error);

    /**
     * Systemd bus callback for getting the state property
     */
    static int callbackGetState(sd_bus* bus, const char* path,
                                const char* interface, const char* property,
                                sd_bus_message* msg, void* context,
                                sd_bus_error* error);

    /**
     * Systemd bus callback for setting the pgood_timeout property
     */
    static int callbackSetPgoodTimeout(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* msg, void* context,
        sd_bus_error* error);

    /**
     * Systemd bus callback for the setPowerSupplyError method
     */
    static int callbackSetPowerSupplyError(sd_bus_message* msg, void* context,
                                           sd_bus_error* error);

    /**
     * Systemd bus callback for the setPowerState method
     */
    static int callbackSetPowerState(sd_bus_message* msg, void* context,
                                     sd_bus_error* error);
};

} // namespace phosphor::power::sequencer
