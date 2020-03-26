#pragma once

#include "pmbus.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <sdbusplus/bus/match.hpp>

#include <stdexcept>

namespace phosphor::power::psu
{

// PMBus device driver "file name" to read for CCIN value.
constexpr auto CCIN = "ccin";
constexpr auto PART_NUMBER = "part_number";
constexpr auto SERIAL_HEADER = "header";
constexpr auto SERIAL_NUMBER = "serial_number";
constexpr auto FW_VERSION = "fw_version";

// The D-Bus property name to update with the CCIN value.
constexpr auto MODEL_PROP = "Model";
constexpr auto PN_PROP = "PartNumber";
constexpr auto SN_PROP = "SerialNumber";
constexpr auto VERSION_PROP = "Version";

// ipzVPD Keyword sizes
static constexpr auto FL_KW_SIZE = 20;

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
     * @param[in] invpath - String for inventory path to use
     * @param[in] i2cbus - The bus number this power supply is on
     * @param[in] i2caddr - The 16-bit I2C address of the power supply
     */
    PowerSupply(sdbusplus::bus::bus& bus, const std::string& invpath,
                std::uint8_t i2cbus, const std::string& i2caddr) :
        bus(bus),
        inventoryPath(invpath),
        pmbusIntf(phosphor::pmbus::createPMBus(i2cbus, i2caddr))
    {
        if (inventoryPath.empty())
        {
            throw std::invalid_argument{"Invalid empty inventoryPath"};
        }

        // Setup the functions to call when the D-Bus inventory path for the
        // Present property changes.
        presentMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::propertiesChanged(inventoryPath,
                                                            INVENTORY_IFACE),
            [this](auto& msg) { this->inventoryChanged(msg); });
        presentAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::path_namespace(inventoryPath),
            [this](auto& msg) { this->inventoryChanged(msg); });
        // Get the current state of the Present property.
        updatePresence();
    }

    phosphor::pmbus::PMBusBase& getPMBus()
    {
        return *pmbusIntf;
    }

    /**
     * Power supply specific function to analyze for faults/errors.
     *
     * Various PMBus status bits will be checked for fault conditions.
     * If a certain fault bits are on, the appropriate error will be
     * committed.
     */
    void analyze();

    /**
     * Write PMBus CLEAR_FAULTS
     *
     * This function will be called in various situations in order to clear
     * any fault status bits that may have been set, in order to start over
     * with a clean state. Presence changes and power state changes will
     * want to clear any faults logged.
     */
    void clearFaults();

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
    void updateInventory();

    /**
     * @brief Accessor function to indicate present status
     */
    bool isPresent() const
    {
        return present;
    }

    /**
     * @brief Returns true if a fault was found.
     */
    bool isFaulted() const
    {
        return faultFound;
    }

    /**
     * @brief Returns true if INPUT fault occurred.
     */
    bool hasInputFault() const
    {
        return inputFault;
    }

    /**
     * @brief Returns true if MFRSPECIFIC occurred.
     */
    bool hasMFRFault() const
    {
        return mfrFault;
    }

    /**
     * @brief Returns true if VIN_UV_FAULT occurred.
     */
    bool hasVINUVFault() const
    {
        return vinUVFault;
    }

  private:
    /** @brief systemd bus member */
    sdbusplus::bus::bus& bus;

    /** @brief True if a fault has already been found and not cleared */
    bool faultFound = false;

    /** @brief True if bit 5 of STATUS_WORD high byte is on. */
    bool inputFault = false;

    /** @brief True if bit 4 of STATUS_WORD high byte is on. */
    bool mfrFault = false;

    /** @brief True if bit 3 of STATUS_WORD low byte is on. */
    bool vinUVFault = false;

    /**
     * @brief D-Bus path to use for this power supply's inventory status.
     **/
    std::string inventoryPath;

    /** @brief True if the power supply is present. */
    bool present = false;

    /** @brief D-Bus match variable used to subscribe to Present property
     * changes.
     **/
    std::unique_ptr<sdbusplus::bus::match_t> presentMatch;

    /** @brief D-Bus match variable used to subscribe for Present property
     * interface added.
     */
    std::unique_ptr<sdbusplus::bus::match_t> presentAddedMatch;

    /**
     * @brief Pointer to the PMBus interface
     *
     * Used to read or write to/from PMBus power supply devices.
     */
    std::unique_ptr<phosphor::pmbus::PMBusBase> pmbusIntf;

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
