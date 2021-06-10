#pragma once

#include "types.hpp"

#include <sdbusplus/bus/match.hpp>

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

    virtual bool getPresence(sdbusplus::bus::bus& bus,
                             const std::string& invpath) const = 0;

    virtual void setPresence(sdbusplus::bus::bus& bus,
                             const std::string& invpath, bool present,
                             const std::string& name) const = 0;
};

const UtilBase& getUtils();

inline bool getPresence(sdbusplus::bus::bus& bus, const std::string& invpath)
{
    return getUtils().getPresence(bus, invpath);
}

inline void setPresence(sdbusplus::bus::bus& bus, const std::string& invpath,
                        bool present, const std::string& name)
{
    return getUtils().setPresence(bus, invpath, present, name);
}

class GPIOInterface
{
  public:
    virtual ~GPIOInterface() = default;

    virtual int read() = 0;
};

} // namespace phosphor::power::psu
