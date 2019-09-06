#pragma once

#include "device.hpp"
#include "pmbus.hpp"

#include <algorithm>
#include <filesystem>
#include <sdbusplus/bus.hpp>

namespace witherspoon
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
     * Checks for PSU0 & PSU1 whether are "present".
     *
     * @return bool - true if PSU0 & PSU1 aren't "present".
     */
    bool checkPSUdevice();

    /**
     * The D-Bus bus object
     */
    sdbusplus::bus::bus& bus;
};

} // namespace power
} // namespace witherspoon
