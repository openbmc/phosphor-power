#pragma once

#include "pmbus.hpp"
#include "util_base.hpp"

#include <gmock/gmock.h>

namespace phosphor
{
namespace pmbus
{
class MockedPMBus : public PMBusBase
{

    public:
        virtual ~MockedPMBus() = default;

        MOCK_METHOD(uint64_t, read, (const std::string& name, Type type), (override));
};
} // namespace pmbus

namespace power
{
namespace psu
{
class MockedUtil : public UtilBase
{
  public:
    virtual ~MockedUtil() = default;

    MOCK_CONST_METHOD2(getPresence, bool(sdbusplus::bus::bus& bus,
                                         const std::string& invpath));
};

static std::unique_ptr<MockedUtil> util;
inline const UtilBase& getUtils()
{
    if (!util)
    {
        util = std::make_unique<MockedUtil>();
    }
    return *util;
}

inline void freeUtils()
{
    util.reset();
}

} // namespace psu
} // namespace power

} // namespace phosphor
