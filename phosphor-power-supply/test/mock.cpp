#include "mock.hpp"

#include <gmock/gmock.h>

// These replace the real createPMBus(), createGPIO() and getUtils() so that a
// real PowerSupply object can be built and driven without hardware or D-Bus
// services. The mocks are wrapped in NiceMock so calls a test does not care
// about (for example the many STATUS_* reads) do not generate warnings.
namespace phosphor
{
namespace pmbus
{

std::unique_ptr<PMBusBase> createPMBus(std::uint16_t /*bus*/,
                                       const std::string& /*address*/)
{
    return std::make_unique<::testing::NiceMock<MockedPMBus>>();
}
} // namespace pmbus

namespace power
{
namespace psu
{
static std::unique_ptr<::testing::NiceMock<MockedUtil>> util;

const UtilBase& getUtils()
{
    if (!util)
    {
        util = std::make_unique<::testing::NiceMock<MockedUtil>>();
    }
    return *util;
}

void freeUtils()
{
    util.reset();
}

std::unique_ptr<GPIOInterfaceBase> createGPIO(const std::string& /*namedGpio*/)
{
    return std::make_unique<::testing::NiceMock<MockedGPIOInterface>>();
}

} // namespace psu
} // namespace power
} // namespace phosphor
