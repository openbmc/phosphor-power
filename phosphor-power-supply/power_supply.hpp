#pragma once
#include "device.hpp"

namespace phosphor
{
namespace power
{
namespace psu
{

/**
 * @class PowerSupply
 * Represents a PMBus power supply device.
 */
class PowerSupply
{
  public:
    PowerSupply();
    PowerSupply(const PowerSupply&) = delete;
    PowerSupply(PowerSupply&&) = default;
    PowerSupply& operator=(const PowerSupply&) = default;
    PowerSupply& operator=(PowerSupply&&) = default;
    ~PowerSupply() = default;

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

  private:
    /** @brief True if a fault has already been found and not cleared */
    bool faultFound = false;
};

} // namespace psu
} // namespace power
} // namespace phosphor
