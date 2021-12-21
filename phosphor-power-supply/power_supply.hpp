#pragma once

#include "pmbus.hpp"
#include "types.hpp"
#include "util.hpp"
#include "utility.hpp"

#include <gpiod.hpp>
#include <sdbusplus/bus/match.hpp>

#include <filesystem>
#include <stdexcept>

namespace phosphor::power::psu
{

#if IBM_VPD
// PMBus device driver "file name" to read for CCIN value.
constexpr auto CCIN = "ccin";
constexpr auto PART_NUMBER = "part_number";
constexpr auto FRU_NUMBER = "fru";
constexpr auto SERIAL_HEADER = "header";
constexpr auto SERIAL_NUMBER = "serial_number";
constexpr auto FW_VERSION = "fw_version";

// The D-Bus property name to update with the CCIN value.
constexpr auto MODEL_PROP = "Model";
constexpr auto PN_PROP = "PartNumber";
constexpr auto SPARE_PN_PROP = "SparePartNumber";
constexpr auto SN_PROP = "SerialNumber";
constexpr auto VERSION_PROP = "Version";

// ipzVPD Keyword sizes
static constexpr auto FL_KW_SIZE = 20;
#endif

constexpr auto LOG_LIMIT = 3;
constexpr auto DEGLITCH_LIMIT = 3;

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
     * @param[in] gpioLineName - The gpio-line-name to read for presence. See
     * https://github.com/openbmc/docs/blob/master/designs/device-tree-gpio-naming.md
     */
    PowerSupply(sdbusplus::bus::bus& bus, const std::string& invpath,
                std::uint8_t i2cbus, const std::uint16_t i2caddr,
                const std::string& gpioLineName);

    phosphor::pmbus::PMBusBase& getPMBus()
    {
        return *pmbusIntf;
    }

    GPIOInterfaceBase* getPresenceGPIO()
    {
        return presenceGPIO.get();
    }

    std::string getPresenceGPIOName() const
    {
        if (presenceGPIO != nullptr)
        {
            return presenceGPIO->getName();
        }
        else
        {
            return std::string();
        }
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
     * Write PMBus ON_OFF_CONFIG
     *
     * This function will be called to cause the PMBus device driver to send the
     * ON_OFF_CONFIG command. Takes one byte of data.
     *
     * @param[in] data - The ON_OFF_CONFIG data byte mask.
     */
    void onOffConfig(uint8_t data);

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
     * @brief Returns the last read value from STATUS_WORD.
     */
    uint64_t getStatusWord() const
    {
        return statusWord;
    }

    /**
     * @brief Returns the last read value from STATUS_INPUT.
     */
    uint64_t getStatusInput() const
    {
        return statusInput;
    }

    /**
     * @brief Returns the last read value from STATUS_MFR.
     */
    uint64_t getMFRFault() const
    {
        return statusMFR;
    }

    /**
     * @brief Returns the last read value from STATUS_CML.
     */
    uint64_t getStatusCML() const
    {
        return statusCML;
    }

    /**
     * @brief Returns the last read value from STATUS_VOUT.
     */
    uint64_t getStatusVout() const
    {
        return statusVout;
    }

    /**
     * @brief Returns the last value read from STATUS_IOUT.
     */
    uint64_t getStatusIout() const
    {
        return statusIout;
    }

    /**
     * @brief Returns the last value read from STATUS_FANS_1_2.
     */
    uint64_t getStatusFans12() const
    {
        return statusFans12;
    }

    /**
     * @brief Returns the last value read from STATUS_TEMPERATURE.
     */
    uint64_t getStatusTemperature() const
    {
        return statusTemperature;
    }

    /**
     * @brief Returns true if a fault was found.
     */
    bool isFaulted() const
    {
        return (hasCommFault() || vinUVFault || inputFault || voutOVFault ||
                ioutOCFault || voutUVFault || fanFault || tempFault ||
                (pgoodFault >= DEGLITCH_LIMIT) || mfrFault);
    }

    /**
     * @brief Return whether a fault has been logged for this power supply
     */
    bool isFaultLogged() const
    {
        return faultLogged;
    }

    /**
     * @brief Called when a fault for this power supply has been logged.
     */
    void setFaultLogged()
    {
        faultLogged = true;
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

    /**
     * @brief Returns true if VOUT_OV_FAULT occurred.
     */
    bool hasVoutOVFault() const
    {
        return voutOVFault;
    }

    /**
     * @brief Returns true if IOUT_OC fault occurred (bit 4 STATUS_BYTE).
     */
    bool hasIoutOCFault() const
    {
        return ioutOCFault;
    }

    /**
     * @brief Returns true if VOUT_UV_FAULT occurred.
     */
    bool hasVoutUVFault() const
    {
        return voutUVFault;
    }

    /**
     *@brief Returns true if fan fault occurred.
     */
    bool hasFanFault() const
    {
        return fanFault;
    }

    /**
     * @brief Returns true if TEMPERATURE fault occurred.
     */
    bool hasTempFault() const
    {
        return tempFault;
    }

    /**
     * @brief Returns true if there is a PGood fault (PGOOD# inactive, or OFF
     * bit on).
     */
    bool hasPgoodFault() const
    {
        return (pgoodFault >= DEGLITCH_LIMIT);
    }

    /**
     * @brief Return true if there is a PS_Kill fault.
     */
    bool hasPSKillFault() const
    {
        return psKillFault;
    }

    /**
     * @brief Returns true if there is a 12Vcs (standy power) fault.
     */
    bool hasPS12VcsFault() const
    {
        return ps12VcsFault;
    }

    /**
     * @brief Returns true if there is a 12V current-share fault.
     */
    bool hasPSCS12VFault() const
    {
        return psCS12VFault;
    }

    /**
     * @brief Returns the device path
     *
     * This can be used for error call outs.
     * Example: /sys/bus/i2c/devices/3-0068
     */
    const std::string getDevicePath() const
    {
        return pmbusIntf->path();
    }

    /**
     * @brief Returns this power supplies inventory path.
     *
     * This can be used for error call outs.
     * Example:
     * /xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply1
     */
    const std::string& getInventoryPath() const
    {
        return inventoryPath;
    }

    /**
     * @brief Returns the firmware revision version read from the power supply
     */
    const std::string& getFWVersion() const
    {
        return fwVersion;
    }

    /**
     * @brief Returns the model name of the power supply
     */
    const std::string& getModelName() const
    {
        return modelName;
    }

    /** @brief Returns true if the number of failed reads exceeds limit
     * TODO: or CML bit on.
     */
    bool hasCommFault() const
    {
        return ((readFail >= LOG_LIMIT) || (cmlFault));
    }

    /**
     * @brief Reads the pmbus input voltage and returns that actual voltage
     *        reading and the calculated input voltage based on thresholds.
     * @param[out] actualInputVoltage - The actual voltage reading, in Volts.
     * @param[out] inputVoltage - A rounded up/down value of the actual input
     *             voltage based on thresholds, in Volts.
     */
    void getInputVoltage(double& actualInputVoltage, int& inputVoltage) const;

  private:
    /** @brief systemd bus member */
    sdbusplus::bus::bus& bus;

    /** @brief Will be updated to the latest/lastvalue read from STATUS_WORD.*/
    uint64_t statusWord = 0;

    /** @brief Will be updated to the latest/lastvalue read from STATUS_INPUT.*/
    uint64_t statusInput = 0;

    /** @brief Will be updated to the latest/lastvalue read from STATUS_MFR.*/
    uint64_t statusMFR = 0;

    /** @brief Will be updated to the latest/last value read from STATUS_CML.*/
    uint64_t statusCML = 0;

    /** @brief Will be updated to the latest/last value read from STATUS_VOUT.*/
    uint64_t statusVout = 0;

    /** @brief Will be updated to the latest/last value read from STATUS_IOUT.*/
    uint64_t statusIout = 0;

    /** @brief Will be updated to the latest/last value read from
     * STATUS_FANS_1_2. */
    uint64_t statusFans12 = 0;

    /** @brief Will be updated to the latest/last value read from
     * STATUS_TEMPERATURE.*/
    uint64_t statusTemperature = 0;

    /** @brief True if an error for a fault has already been logged. */
    bool faultLogged = false;

    /** @brief True if bit 1 of STATUS_WORD low byte is on. */
    bool cmlFault = false;

    /** @brief True if bit 5 of STATUS_WORD high byte is on. */
    bool inputFault = false;

    /** @brief True if bit 4 of STATUS_WORD high byte is on. */
    bool mfrFault = false;

    /** @brief True if bit 3 of STATUS_WORD low byte is on. */
    bool vinUVFault = false;

    /** @brief True if bit 5 of STATUS_WORD low byte is on. */
    bool voutOVFault = false;

    /** @brief True if bit 4 of STATUS_WORD low byte is on. */
    bool ioutOCFault = false;

    /** @brief True if bit 7 of STATUS_WORD high byte is on and bit 5 (VOUT_OV)
     * of low byte is off. */
    bool voutUVFault = false;

    /** @brief True if FANS fault/warn bit on in STATUS_WORD. */
    bool fanFault = false;

    /** @brief True if bit 2 of STATUS_WORD low byte is on. */
    bool tempFault = false;

    /**
     * @brief Incremented if bit 11 or 6 of STATUS_WORD is on. PGOOD# is
     * inactive, or the unit is off.
     *
     * Considered faulted if reaches DEGLITCH_LIMIT.
     */
    int pgoodFault = 0;

    /**
     * @brief Power Supply Kill fault.
     */
    bool psKillFault = false;

    /**
     * @brief Power Supply 12Vcs fault (standby power).
     */
    bool ps12VcsFault = false;

    /**
     * @brief Power Supply Current-Share fault in 12V domain.
     */
    bool psCS12VFault = false;

    /** @brief Count of the number of read failures. */
    size_t readFail = 0;

    /**
     * @breif Examine STATUS_WORD for CML (communication, memory, logic fault).
     */
    void analyzeCMLFault();

    void analyzeInputFault();

    /**
     * @brief Examine STATUS_WORD for temperature fault.
     */
    void analyzeTemperatureFault();

    /**
     * @brief Examine STATUS_WORD for pgood or unit off faults.
     */
    void analyzePgoodFault();

    /**
     * @brief Determine possible manufacturer-specific faults from bits in
     * STATUS_MFR.
     *
     * The bits in the STATUS_MFR_SPECIFIC command response have "Manufacturer
     * Defined" meanings. Determine which faults, if any, are present based on
     * the power supply (device driver) type.
     */
    void determineMFRFault();

    void analyzeMFRFault();

    /**
     * @brief D-Bus path to use for this power supply's inventory status.
     **/
    std::string inventoryPath;

    /**
     * @brief The libgpiod object for monitoring PSU presence
     */
    std::unique_ptr<GPIOInterfaceBase> presenceGPIO = nullptr;

    /** @brief True if the power supply is present. */
    bool present = false;

    /** @brief Power supply model name. */
    std::string modelName;

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
    std::unique_ptr<phosphor::pmbus::PMBusBase> pmbusIntf = nullptr;

    /** @brief Stored copy of the firmware version/revision string */
    std::string fwVersion;

    /**
     * @brief The file system path used for binding the device driver.
     */
    const std::filesystem::path bindPath;

    /* @brief The string to pass in for binding the device driver. */
    std::string bindDevice;

    /**
     * @brief Binds or unbinds the power supply device driver
     *
     * Called when a presence change is detected to either bind the device
     * driver for the power supply when it is installed, or unbind the device
     * driver when the power supply is removed.
     *
     * Writes <device> to <path>/bind (or unbind)
     *
     * @param present - when true, will bind the device driver
     *                  when false, will unbind the device driver
     */
    void bindOrUnbindDriver(bool present);

    /**
     *  @brief Updates the presence status by querying D-Bus
     *
     * The D-Bus inventory properties for this power supply will be read to
     * determine if the power supply is present or not and update this
     * object's present member variable to reflect current status.
     **/
    void updatePresence();

    /**
     * @brief Updates the power supply presence by reading the GPIO line.
     */
    void updatePresenceGPIO();

    /**
     * @brief Callback for inventory property changes
     *
     * Process change of Present property for power supply.
     *
     * This is used if we are watching the D-Bus properties instead of reading
     * the GPIO presence line ourselves.
     *
     * @param[in]  msg - Data associated with Present change signal
     **/
    void inventoryChanged(sdbusplus::message::message& msg);

    /**
     * @brief Callback for inventory property added.
     *
     * Process add of the interface with the Present property for power supply.
     *
     * This is used if we are watching the D-Bus properties instead of reading
     * the GPIO presence line ourselves.
     *
     * @param[in]  msg - Data associated with Present add signal
     **/
    void inventoryAdded(sdbusplus::message::message& msg);
};

} // namespace phosphor::power::psu
