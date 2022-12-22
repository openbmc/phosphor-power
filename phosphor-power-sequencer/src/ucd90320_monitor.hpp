#pragma once

#include "ucd90x_monitor.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class UCD90320Monitor
 * This class implements fault analysis for the UCD90320
 * power sequencer device.
 */
class UCD90320Monitor : public UCD90xMonitor
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
     * @param bus D-Bus bus object
     * @param i2cBus The bus number of the power sequencer device
     * @param i2cAddress The I2C address of the power sequencer device
     */
    UCD90320Monitor(sdbusplus::bus_t& bus, std::uint8_t i2cBus,
                    std::uint16_t i2cAddress);

  protected:
    /** @copydoc UCD90xMonitor::formatGpioValues() */
    void formatGpioValues(
        const std::vector<int>& values, unsigned int numberLines,
        std::map<std::string, std::string>& additionalData) const override;
};

} // namespace phosphor::power::sequencer
