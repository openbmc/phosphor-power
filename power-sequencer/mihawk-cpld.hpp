#pragma once

#include "device.hpp"
#include "pmbus.hpp"
#include "tools/i2c/i2c_interface.hpp"

#include <sdbusplus/bus.hpp>

#include <algorithm>
#include <filesystem>

namespace phosphor
{
namespace power
{

/**
 * @class MihawkCPLD
 *
 * This class implements fault analysis for Mihawk's CPLD
 * power sequencer device.
 *
 */
class MihawkCPLD : public Device
{
  public:
    MihawkCPLD() = delete;
    ~MihawkCPLD() = default;
    MihawkCPLD(const MihawkCPLD&) = delete;
    MihawkCPLD& operator=(const MihawkCPLD&) = delete;
    MihawkCPLD(MihawkCPLD&&) = default;
    MihawkCPLD& operator=(MihawkCPLD&&) = default;

    /**
     * Constructor
     *
     * @param[in] instance - the device instance number
     * @param[in] bus - D-Bus bus object
     */
    MihawkCPLD(size_t instance, sdbusplus::bus::bus& bus);

    /**
     * Analyzes the device for errors when the device is
     * known to be in an error state.  A log will be created.
     */
    void onFailure() override;

    /**
     * Checks the device for errors and only creates a log
     * if one is found.
     */
    void analyze() override;

    /**
     * Clears faults in the device
     */
    void clearFaults() override
    {
    }

  private:
    /**
     * If checkPoweronFault() or checkPowerreadyFault()
     * returns "true", use readFromCPLDErrorCode()
     * to read CPLD-error-code-register
     * to analyze the fail reason.
     *
     * @param[in] statusReg - I2C's statusReg, slaveAddr
     * offset.
     * ex.Mihawk's CPLD-register is on slaveAddr ox40 of
     * i2c-11, but poweron_errcode-register is on slaveAddr
     * offset 0x21, power_ready-errorcode-register is on
     * slaveAddr offset 0x22.
     *
     * @return int - the error-code value which is read on
     * CPLD-error-code-register.
     */
    int readFromCPLDErrorCode(int statusReg);

    /**
     * Checks for PoweronFault on Mihawk's
     * CPLD-power_on-error-interrupt-bit-register
     * whether is transfered to "1".
     *
     * @return bool - true if power_on fail.
     */
    bool checkPoweronFault();

    /**
     * Clear CPLD intrupt record after reading CPLD_register.
     */
    void clearCPLDregister();

    /**
     * Check for PowerreadyFault on Mihawk's
     * CPLD-power_ready-error-interrupt-bit-register
     * whether is transfered to "1".
     *
     * @return bool - true if power_ready fail.
     */
    bool checkPowerreadyFault();

    /**
     * Use I2CInterface to read & write CPLD_register.
     */
    std::unique_ptr<i2c::I2CInterface> i2c;

    /**
     * The D-Bus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * Open CPLD_register via i2c.
     */
    void openCPLDDevice();

    /**
     * The parameter which is checked CPLD's the same error
     * whether is created again.
     */
    bool errorcodeMask;

    enum class ErrorCode : int
    {
        /**
         * All of powerOnErrorcode are the definition of error-code
         * which are read on CPLD-error-code-register.
         */
        /**
         * The definition of error-code:
         * Read CPLD-error-code-register fail.
         */
        ErrorCode_0 = 0,

        /**
         * The definition of error-code:
         * PSU1_PGOOD fail.
         */
        ErrorCode_1 = 2,

        /**
         * The definition of error-code:
         * PSU0_PGOOD fail.
         */
        ErrorCode_2 = 1,

        /**
         * The definition of error-code:
         * 240Va_Fault_A fail.
         */
        ErrorCode_3 = 3,

        /**
         * The definition of error-code:
         * 240Va_Fault_B fail.
         */
        ErrorCode_4 = 4,

        /**
         * The definition of error-code:
         * 240Va_Fault_C fail.
         */
        ErrorCode_5 = 5,

        /**
         * The definition of error-code:
         * 240Va_Fault_D fail.
         */
        ErrorCode_6 = 6,

        /**
         * The definition of error-code:
         * 240Va_Fault_E fail.
         */
        ErrorCode_7 = 7,

        /**
         * The definition of error-code:
         * 240Va_Fault_F fail.
         */
        ErrorCode_8 = 8,

        /**
         * The definition of error-code:
         * 240Va_Fault_G fail.
         */
        ErrorCode_9 = 9,

        /**
         * The definition of error-code:
         * 240Va_Fault_H fail.
         */
        ErrorCode_10 = 10,

        /**
         * The definition of error-code:
         * 240Va_Fault_J fail.
         */
        ErrorCode_11 = 11,

        /**
         * The definition of error-code:
         * 240Va_Fault_K fail.
         */
        ErrorCode_12 = 12,

        /**
         * The definition of error-code:
         * 240Va_Fault_L fail.
         */
        ErrorCode_13 = 13,

        /**
         * The definition of error-code:
         * P5V_PGOOD fail.
         */
        ErrorCode_14 = 14,

        /**
         * The definition of error-code:
         * P3V3_PGOOD fail.
         */
        ErrorCode_15 = 15,

        /**
         * The definition of error-code:
         * P1V8_PGOOD fail.
         */
        ErrorCode_16 = 16,

        /**
         * The definition of error-code:
         * P1V1_PGOOD fail.
         */
        ErrorCode_17 = 17,

        /**
         * The definition of error-code:
         * P0V9_PGOOD fail.
         */
        ErrorCode_18 = 18,

        /**
         * The definition of error-code:
         * P2V5A_PGOOD fail.
         */
        ErrorCode_19 = 19,

        /**
         * The definition of error-code:
         * P2V5B_PGOOD fail.
         */
        ErrorCode_20 = 20,

        /**
         * The definition of error-code:
         * Vdn0_PGOOD fail.
         */
        ErrorCode_21 = 21,

        /**
         * The definition of error-code:
         * Vdn1_PGOOD fail.
         */
        ErrorCode_22 = 22,

        /**
         * The definition of error-code:
         * P1V5_PGOOD fail.
         */
        ErrorCode_23 = 23,

        /**
         * The definition of error-code:
         * Vio0_PGOOD fail.
         */
        ErrorCode_24 = 24,

        /**
         * The definition of error-code:
         * Vio1_PGOOD fail.
         */
        ErrorCode_25 = 25,

        /**
         * The definition of error-code:
         * Vdd0_PGOOD fail.
         */
        ErrorCode_26 = 26,

        /**
         * The definition of error-code:
         * Vcs0_PGOOD fail.
         */
        ErrorCode_27 = 27,

        /**
         * The definition of error-code:
         * Vdd1_PGOOD fail.
         */
        ErrorCode_28 = 28,

        /**
         * The definition of error-code:
         * Vcs1_PGOOD fail.
         */
        ErrorCode_29 = 29,

        /**
         * The definition of error-code:
         * Vddr0_PGOOD fail.
         */
        ErrorCode_30 = 30,

        /**
         * The definition of error-code:
         * Vtt0_PGOOD fail.
         */
        ErrorCode_31 = 31,

        /**
         * The definition of error-code:
         * Vddr1_PGOOD fail.
         */
        ErrorCode_32 = 32,

        /**
         * The definition of error-code:
         * Vtt1_PGOOD fail.
         */
        ErrorCode_33 = 33,

        /**
         * The definition of error-code:
         * GPU0_PGOOD fail.
         */
        ErrorCode_34 = 34,

        /**
         * The definition of error-code:
         * GPU1_PGOOD fail.
         */
        ErrorCode_35 = 35,

        /**
         * The definition of error-code:
         * PSU0PSU1_PGOOD fail.
         */
        ErrorCode_36 = 170
    };
};

} // namespace power
} // namespace phosphor
