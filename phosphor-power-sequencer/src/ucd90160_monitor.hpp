#pragma once

#include "ucd90x_monitor.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdint>

namespace phosphor::power::sequencer
{

/**
 * @class UCD90160Monitor
 * This class implements fault analysis for the UCD90160
 * power sequencer device.
 */
class UCD90160Monitor : public UCD90xMonitor
{
  public:
    UCD90160Monitor() = delete;
    UCD90160Monitor(const UCD90160Monitor&) = delete;
    UCD90160Monitor& operator=(const UCD90160Monitor&) = delete;
    UCD90160Monitor(UCD90160Monitor&&) = delete;
    UCD90160Monitor& operator=(UCD90160Monitor&&) = delete;
    virtual ~UCD90160Monitor() = default;

    /**
     * Create a device object for UCD90160 monitoring.
     * @param bus D-Bus bus object
     * @param i2cBus The bus number of the power sequencer device
     * @param i2cAddress The I2C address of the power sequencer device
     */
    UCD90160Monitor(sdbusplus::bus_t& bus, std::uint8_t i2cBus,
                    std::uint16_t i2cAddress);
};

} // namespace phosphor::power::sequencer
