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

    MOCK_METHOD(uint64_t, read, (const std::string& name, Type type),
                (override));
    MOCK_METHOD(std::string, readString, (const std::string& name, Type type),
                (override));
    MOCK_METHOD(void, writeBinary,
                (const std::string& name, std::vector<uint8_t> data, Type type),
                (override));
    MOCK_METHOD(void, findHwmonDir, (), (override));
    MOCK_METHOD(const fs::path&, path, (), (const, override));
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

    MOCK_METHOD(bool, getPresence,
                (sdbusplus::bus::bus & bus, const std::string& invpath),
                (const, override));
};

static std::unique_ptr<MockedUtil> util;

inline void freeUtils()
{
    util.reset();
}

} // namespace psu
} // namespace power

} // namespace phosphor
