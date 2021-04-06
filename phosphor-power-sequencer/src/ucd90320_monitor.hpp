#pragma once

#include "pmbus.hpp"
#include "power_sequencer_monitor.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor::power::sequencer
{

/**
 * @class UCD90320Monitor
 * This class implements fault analysis for the UCD90320
 * power sequencer device.
 */
class UCD90320Monitor : public PowerSequencerMonitor
{
  public:
    UCD90320Monitor() = delete;
    UCD90320Monitor(const UCD90320Monitor&) = delete;
    UCD90320Monitor& operator=(const UCD90320Monitor&) = delete;
    UCD90320Monitor(UCD90320Monitor&&) = delete;
    UCD90320Monitor& operator=(UCD90320Monitor&&) = delete;
    virtual ~UCD90320Monitor() = default;

    /**
     * Create a device object for UCD90320 monitoring.
     * @param[in] bus D-Bus bus object
     * @param[in] i2cBus The bus number of the power sequencer device
     * @param[in] i2cAddress The I2C address of the power sequencer device
     */
    UCD90320Monitor(sdbusplus::bus::bus& bus, std::uint8_t i2cBus,
                    const std::uint16_t i2cAddress);

  private:
    /**
     * The D-Bus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * The read/write interface to this hardware
     */
    pmbus::PMBus interface;
};

} // namespace phosphor::power::sequencer
