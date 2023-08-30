#pragma once

#include "types.hpp"

#include <sdbusplus/bus/match.hpp>

#include <bitset>
#include <chrono>

namespace phosphor::power::psu
{

/**
 * @class UtilBase
 * A base class to allow for mocking certain utility functions.
 */
class UtilBase
{
  public:
    virtual ~UtilBase() = default;

    virtual bool getPresence(sdbusplus::bus_t& bus,
                             const std::string& invpath) const = 0;

    virtual void setPresence(sdbusplus::bus_t& bus, const std::string& invpath,
                             bool present, const std::string& name) const = 0;

    virtual void setAvailable(sdbusplus::bus_t& bus, const std::string& invpath,
                              bool available) const = 0;

    virtual void handleChassisHealthRollup(sdbusplus::bus_t& bus,
                                           const std::string& invpath,
                                           bool addRollup) const = 0;

    virtual std::string getChassis(sdbusplus::bus_t& /*bus*/,
                                   const std::string& /*invpath*/) const = 0;
};

const UtilBase& getUtils();

inline bool getPresence(sdbusplus::bus_t& bus, const std::string& invpath)
{
    return getUtils().getPresence(bus, invpath);
}

inline void setPresence(sdbusplus::bus_t& bus, const std::string& invpath,
                        bool present, const std::string& name)
{
    return getUtils().setPresence(bus, invpath, present, name);
}

inline void setAvailable(sdbusplus::bus_t& bus, const std::string& invpath,
                         bool available)
{
    getUtils().setAvailable(bus, invpath, available);
}

inline void handleChassisHealthRollup(sdbusplus::bus_t& bus,
                                      const std::string& invpath,
                                      bool addRollup)
{
    getUtils().handleChassisHealthRollup(bus, invpath, addRollup);
}

inline std::string getChassis(sdbusplus::bus_t& bus, const std::string& invpath)
{
    return getUtils().getChassis(bus, invpath);
}

class GPIOInterfaceBase
{
  public:
    virtual ~GPIOInterfaceBase() = default;

    virtual int read() = 0;
    virtual void write(int value, std::bitset<32> flags) = 0;
    virtual void toggleLowHigh(const std::chrono::milliseconds& delay) = 0;
    virtual std::string getName() const = 0;
};

} // namespace phosphor::power::psu
