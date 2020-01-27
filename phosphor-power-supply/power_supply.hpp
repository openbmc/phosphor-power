#pragma once

#include "types.hpp"

#include <sdbusplus/bus/match.hpp>

namespace phosphor::power::psu
{
/**
 * @class PowerSupply
 * Represents a PMBus power supply device.
 */
class PowerSupply
{
  public:
    PowerSupply() = delete;
    PowerSupply(const PowerSupply&) = delete;
    PowerSupply(PowerSupply&&) = delete;
    PowerSupply& operator=(const PowerSupply&) = delete;
    PowerSupply& operator=(PowerSupply&&) = delete;
    ~PowerSupply() = default;

    /**
     * @param[in] invpath - string for inventory path to use
     */
    PowerSupply(sdbusplus::bus::bus& bus, const std::string& invpath,
                std::uint8_t i2cbus, std::string i2caddr) :
        bus(bus),
        inventoryPath(invpath), i2cbus(i2cbus), i2caddr(i2caddr)
    {
        // Setup the function to call when the D-Bus inventory path for the
        // Present property changes.
        presentMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::propertiesChanged(inventoryPath,
                                                            INVENTORY_IFACE),
            [this](auto& msg) { this->inventoryChanged(msg); });
        // Get the current state of the Present property.
        updatePresence();
    }

    /**
     * Power supply specific function to analyze for faults/errors.
     *
     * Various PMBus status bits will be checked for fault conditions.
     * If a certain fault bits are on, the appropriate error will be
     * committed.
     */
    void analyze()
    {
    }

    /**
     * Write PMBus CLEAR_FAULTS
     *
     * This function will be called in various situations in order to clear
     * any fault status bits that may have been set, in order to start over
     * with a clean state. Presence changes and power state changes will
     * want to clear any faults logged.
     */
    void clearFaults()
    {
    }

    /**
     * @brief Adds properties to the inventory.
     *
     * Reads the values from the device and writes them to the
     * associated power supply D-Bus inventory object.
     *
     * This needs to be done on startup, and each time the presence
     * state changes.
     *
     * Properties added:
     * - Serial Number
     * - Part Number
     * - CCIN (Customer Card Identification Number) - added as the Model
     * - Firmware version
     */
    void updateInventory()
    {
    }

    /**
     * @brief Accessor function to indicate present status
     */
    bool isPresent() const
    {
        return present;
    }

  private:
    /** @brief systemd bus member */
    sdbusplus::bus::bus& bus;

    /** @brief True if a fault has already been found and not cleared */
    bool faultFound = false;

    /**
     * @brief D-Bus path to use for this power supply's inventory status.
     **/
    std::string inventoryPath;

    /**
     * @brief I2C bus that this power supply is on.
     */
    std::uint8_t i2cbus;

    /**
     * @brief I2C address of this power supply.
     *
     * The PMBus device driver will put this in a path with 16-bit address,
     * represented as a file path string.
     */
    std::string i2caddr;

    /** @brief True if the power supply is present. */
    bool present = false;

    /** @brief D-Bus match variable used to subscribe to Present property
     * changes.
     **/
    std::unique_ptr<sdbusplus::bus::match_t> presentMatch;
    std::unique_ptr<sdbusplus::bus::match_t> presentAddedMatch;

    /**
     *  @brief Updates the presence status by querying D-Bus
     *
     * The D-Bus inventory properties for this power supply will be read to
     * determine if the power supply is present or not and update this
     * object's present member variable to reflect current status.
     **/
    void updatePresence();

    /**
     * @brief Callback for inventory property changes
     *
     * Process change of Present property for power supply.
     *
     * @param[in]  msg - Data associated with Present change signal
     **/
    void inventoryChanged(sdbusplus::message::message& msg);
};

} // namespace phosphor::power::psu
