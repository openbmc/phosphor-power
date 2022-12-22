#pragma once

#include <sdbusplus/bus.hpp>

#include <map>
#include <string>

namespace phosphor::power::sequencer
{

/**
 * @class PowerSequencerMonitor
 * Define a base class for monitoring a power sequencer device.
 */
class PowerSequencerMonitor
{
  public:
    PowerSequencerMonitor() = delete;
    PowerSequencerMonitor(const PowerSequencerMonitor&) = delete;
    PowerSequencerMonitor& operator=(const PowerSequencerMonitor&) = delete;
    PowerSequencerMonitor(PowerSequencerMonitor&&) = delete;
    PowerSequencerMonitor& operator=(PowerSequencerMonitor&&) = delete;
    virtual ~PowerSequencerMonitor() = default;

    /**
     * Create a base device object for power sequence monitoring.
     * @param bus D-Bus bus object
     */
    explicit PowerSequencerMonitor(sdbusplus::bus_t& bus);

    /**
     * Logs an error using the D-Bus Create method.
     * @param message Message property of the error log entry
     * @param additionalData AdditionalData property of the error log entry
     */
    void logError(const std::string& message,
                  std::map<std::string, std::string>& additionalData);

    /**
     * Analyzes the device for errors when the device is
     * known to be in an error state.  A log will be created.
     * @param timeout if the failure state was determined by timing out
     * @param powerSupplyError The power supply error to log. A default
     * std:string, i.e. empty string (""), is passed when there is no power
     * supply error to log.
     */
    virtual void onFailure(bool timeout, const std::string& powerSupplyError);

  protected:
    /**
     * The D-Bus bus object
     */
    sdbusplus::bus_t& bus;

    /**
     * Create a BMC Dump
     */
    void createBmcDump();
};

} // namespace phosphor::power::sequencer
