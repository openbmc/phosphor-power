#pragma once

#include "pmbus.hpp"
#include "util_base.hpp"

#include <gpiod.hpp>

#include <bitset>
#include <filesystem>

#include <gmock/gmock.h>

namespace phosphor
{
namespace pmbus
{

namespace fs = std::filesystem;

class MockedPMBus : public PMBusBase
{
  public:
    virtual ~MockedPMBus() = default;

    MOCK_METHOD(uint64_t, read,
                (const std::string& name, Type type, bool errTrace),
                (override));
    MOCK_METHOD(std::string, readString, (const std::string& name, Type type),
                (override));
    MOCK_METHOD(std::vector<uint8_t>, readBinary,
                (const std::string& name, Type type, size_t length),
                (override));
    MOCK_METHOD(void, writeBinary,
                (const std::string& name, std::vector<uint8_t> data, Type type),
                (override));
    MOCK_METHOD(void, findHwmonDir, (), (override));
    MOCK_METHOD(const fs::path&, path, (), (const, override));
    MOCK_METHOD(std::string, insertPageNum,
                (const std::string& templateName, size_t page), (override));
    MOCK_METHOD(fs::path, getPath, (Type type), (override));
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
                (sdbusplus::bus_t & bus, const std::string& invpath),
                (const, override));
    MOCK_METHOD(void, setPresence,
                (sdbusplus::bus_t & bus, const std::string& invpath,
                 bool present, const std::string& name),
                (const, override));
    MOCK_METHOD(void, setAvailable,
                (sdbusplus::bus_t & bus, const std::string& invpath,
                 bool available),
                (const, override));
    MOCK_METHOD(void, handleChassisHealthRollup,
                (sdbusplus::bus_t & bus, const std::string& invpath,
                 bool addRollup),
                (const, override));

    std::string getChassis(sdbusplus::bus_t& /*bus*/,
                           const std::string& /*invpath*/) const override
    {
        return "/xyz/openbmc_project/inventory/system/chassis";
    }
};

class MockedGPIOInterface : public GPIOInterfaceBase
{
  public:
    MOCK_METHOD(int, read, (), (override));
    MOCK_METHOD(void, write, (int value, std::bitset<32> flags), (override));
    MOCK_METHOD(void, toggleLowHigh, (const std::chrono::milliseconds& delay),
                (override));
    MOCK_METHOD(std::string, getName, (), (const, override));
};

const UtilBase& getUtils();

void freeUtils();

} // namespace psu
} // namespace power

} // namespace phosphor
