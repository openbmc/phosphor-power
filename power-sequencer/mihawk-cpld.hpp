#pragma once

#include "device.hpp"
#include "pmbus.hpp"

#include <algorithm>
#include <filesystem>
#include <sdbusplus/bus.hpp>

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
class MIHAWKCPLD : public Device
{
  public:
    MIHAWKCPLD() = delete;
    ~MIHAWKCPLD() = default;
    MIHAWKCPLD(const MIHAWKCPLD&) = delete;
    MIHAWKCPLD& operator=(const MIHAWKCPLD&) = delete;
    MIHAWKCPLD(MIHAWKCPLD&&) = default;
    MIHAWKCPLD& operator=(MIHAWKCPLD&&) = default;

    /**
     * Constructor
     *
     * @param[in] instance - the device instance number
     * @param[in] bus - D-Bus bus object
     */
    MIHAWKCPLD(size_t instance, sdbusplus::bus::bus& bus);

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
     * If checkPoweronFault() returns "true",
     * use ReadFromCPLDPSUErrorCode() to read CPLD-error-code-register
     * to analyze the fail reason.
     *
     * @param[in] bus - I2C's bus, ex.i2c-11.
     * Mihawk's CPLD-register is on i2c-11.
     *
     * @param[in] Addr - the i2c-11 address of Mihawk's CPLD-register.
     *
     * @return int - the error-code value which is read on
     * CPLD-error-code-register.
     */
    int readFromCPLDPSUErrorCode(int bus, int Addr);

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
     * Check PSU power status via runtime-monitor after power_on_fault.
     */
    int checkPSUDCpgood();

    /**
     * The D-Bus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * All of powerOnErrorcode are the definition of error-code
     * which are read on CPLD-error-code-register.
     */
    /**
     * The definition of error-code:
     *
     */
    const int powerOnErrorCode_0 = 0;

    /**
     * The definition of error-code:
     * PSU1_PGOOD fail.
     */
    const int powerOnErrorCode_1 = 1;

    /**
     * The definition of error-code:
     * PSU0_PGOOD fail.
     */
    const int powerOnErrorCode_2 = 2;

    /**
     * The definition of error-code:
     * 240Va_Fault_A fail.
     */
    const int powerOnErrorCode_3 = 3;

    /**
     * The definition of error-code:
     * 240Va_Fault_B fail.
     */
    const int powerOnErrorCode_4 = 4;

    /**
     * The definition of error-code:
     * 240Va_Fault_C fail.
     */
    const int powerOnErrorCode_5 = 5;

    /**
     * The definition of error-code:
     * 240Va_Fault_D fail.
     */
    const int powerOnErrorCode_6 = 6;

    /**
     * The definition of error-code:
     * 240Va_Fault_E fail.
     */
    const int powerOnErrorCode_7 = 7;

    /**
     * The definition of error-code:
     * 240Va_Fault_F fail.
     */
    const int powerOnErrorCode_8 = 8;

    /**
     * The definition of error-code:
     * 240Va_Fault_G fail.
     */
    const int powerOnErrorCode_9 = 9;

    /**
     * The definition of error-code:
     * 240Va_Fault_H fail.
     */
    const int powerOnErrorCode_10 = 10;

    /**
     * The definition of error-code:
     * 240Va_Fault_J fail.
     */
    const int powerOnErrorCode_11 = 11;

    /**
     * The definition of error-code:
     * 240Va_Fault_K fail.
     */
    const int powerOnErrorCode_12 = 12;

    /**
     * The definition of error-code:
     * 240Va_Fault_L fail.
     */
    const int powerOnErrorCode_13 = 13;

    /**
     * The definition of error-code:
     * P5V_PGOOD fail.
     */
    const int powerOnErrorCode_14 = 14;

    /**
     * The definition of error-code:
     * P3V3_PGOOD fail.
     */
    const int powerOnErrorCode_15 = 15;

    /**
     * The definition of error-code:
     * P1V8_PGOOD fail.
     */
    const int powerOnErrorCode_16 = 16;

    /**
     * The definition of error-code:
     * P1V1_PGOOD fail.
     */
    const int powerOnErrorCode_17 = 17;

    /**
     * The definition of error-code:
     * P0V9_PGOOD fail.
     */
    const int powerOnErrorCode_18 = 18;

    /**
     * The definition of error-code:
     * P2V5A_PGOOD fail.
     */
    const int powerOnErrorCode_19 = 19;

    /**
     * The definition of error-code:
     * P2V5B_PGOOD fail.
     */
    const int powerOnErrorCode_20 = 20;

    /**
     * The definition of error-code:
     * Vdn0_PGOOD fail.
     */
    const int powerOnErrorCode_21 = 21;

    /**
     * The definition of error-code:
     * Vdn1_PGOOD fail.
     */
    const int powerOnErrorCode_22 = 22;

    /**
     * The definition of error-code:
     * P1V5_PGOOD fail.
     */
    const int powerOnErrorCode_23 = 23;

    /**
     * The definition of error-code:
     * Vio0_PGOOD fail.
     */
    const int powerOnErrorCode_24 = 24;

    /**
     * The definition of error-code:
     * Vio1_PGOOD fail.
     */
    const int powerOnErrorCode_25 = 25;

    /**
     * The definition of error-code:
     * Vdd0_PGOOD fail.
     */
    const int powerOnErrorCode_26 = 26;

    /**
     * The definition of error-code:
     * Vcs0_PGOOD fail.
     */
    const int powerOnErrorCode_27 = 27;

    /**
     * The definition of error-code:
     * Vdd1_PGOOD fail.
     */
    const int powerOnErrorCode_28 = 28;

    /**
     * The definition of error-code:
     * Vcs1_PGOOD fail.
     */
    const int powerOnErrorCode_29 = 29;

    /**
     * The definition of error-code:
     * Vddr0_PGOOD fail.
     */
    const int powerOnErrorCode_30 = 30;

    /**
     * The definition of error-code:
     * Vtt0_PGOOD fail.
     */
    const int powerOnErrorCode_31 = 31;

    /**
     * The definition of error-code:
     * Vddr1_PGOOD fail.
     */
    const int powerOnErrorCode_32 = 32;

    /**
     * The definition of error-code:
     * Vtt1_PGOOD fail.
     */
    const int powerOnErrorCode_33 = 33;

    /**
     * The definition of error-code:
     * GPU0_PGOOD fail.
     */
    const int powerOnErrorCode_34 = 34;

    /**
     * The definition of error-code:
     * GPU1_PGOOD fail.
     */
    const int powerOnErrorCode_35 = 35;

    /**
     * The definition of error-code:
     * PSU0PSU1_PGOOD fail.
     */
    const int powerOnErrorCode_36 = 170;
};

} // namespace power
} // namespace phosphor
