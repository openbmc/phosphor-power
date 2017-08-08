#pragma once
#include "device.hpp"
#include "pmbus.hpp"

namespace witherspoon
{
namespace power
{
namespace psu
{

/**
 * @class PowerSupply
 * Represents a PMBus power supply device.
 */
class PowerSupply : public Device
{
    public:
        PowerSupply() = delete;
        PowerSupply(const PowerSupply&) = delete;
        PowerSupply(PowerSupply&&) = default;
        PowerSupply& operator=(const PowerSupply&) = default;
        PowerSupply& operator=(PowerSupply&&) = default;
        ~PowerSupply() = default;

        /**
         * Constructor
         *
         * @param[in] name - the device name
         * @param[in] inst - the device instance
         * @param[in] objpath - the path to monitor
         * @param[in] invpath - the inventory path to use
         * @param[in] bus - D-Bus bus object
         */
        PowerSupply(const std::string& name, size_t inst,
                    const std::string& objpath,
                    const std::string& invpath,
                    sdbusplus::bus::bus& bus)
            : Device(name, inst), monitorPath(objpath), inventoryPath(invpath),
            bus(bus), pmbusIntf(objpath)
        {
        }

        /**
         * Power supply specific function to analyze for faults/errors.
         *
         * Various PMBus status bits will be checked for fault conditions.
         * If a certain fault bits are on, the appropriate error will be
         * committed.
         */
        void analyze() override;

        /**
         * Write PMBus CLEAR_FAULTS
         *
         * This function will be called in various situations in order to clear
         * any fault status bits that may have been set, in order to start over
         * with a clean state. Presence changes and power state changes will
         * want to clear any faults logged.
         */
        void clearFaults() override;

    private:
        /**
         * The path to use for reading various PMBus bits/words.
         */
        std::string monitorPath;

        /**
         * The D-Bus path to use for this power supply's inventory status.
         */
        std::string inventoryPath;

        /** @brief Connection for sdbusplus bus */
        sdbusplus::bus::bus& bus;

        /**
         * @brief Pointer to the PMBus interface
         *
         * Used to read out of or write to the /sysfs tree(s) containing files
         * that a device driver monitors the PMBus interface to the power
         * supplies.
         */
        witherspoon::pmbus::PMBus pmbusIntf;

        /**
         * @brief Has a PMBus read failure already been logged?
         */
        bool readFailLogged = false;

        /**
         * @brief Set to true when a VIN UV fault has been detected
         *
         * This is the VIN_UV_FAULT bit in the low byte from the STATUS_WORD
         * command response.
         */
        bool vinUVFault = false;

        /**
         * @brief Set to true when an input fault or warning is detected
         *
         * This is the "INPUT FAULT OR WARNING" bit in the high byte from the
         * STATUS_WORD command response.
         */
        bool inputFault = false;
};

}
}
}
