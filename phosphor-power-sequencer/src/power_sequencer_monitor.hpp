#pragma once

namespace phosphor::power::sequencer
{

/**
 * @class PowerSequencerMonitor
 * Define a base class for monitoring a power sequencer device.
 */
class PowerSequencerMonitor
{
  public:
    PowerSequencerMonitor() = default;
    PowerSequencerMonitor(const PowerSequencerMonitor&) = delete;
    PowerSequencerMonitor& operator=(const PowerSequencerMonitor&) = delete;
    PowerSequencerMonitor(PowerSequencerMonitor&&) = delete;
    PowerSequencerMonitor& operator=(PowerSequencerMonitor&&) = delete;
    virtual ~PowerSequencerMonitor() = default;
};

} // namespace phosphor::power::sequencer
