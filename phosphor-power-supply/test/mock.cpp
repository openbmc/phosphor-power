#include "mock.hpp"

namespace phosphor
{
namespace pmbus
{

std::unique_ptr<PMBusBase> createPMBus(std::uint8_t /*bus*/,
                                       const std::string& /*address*/)
{
    return std::make_unique<MockedPMBus>();
}
} // namespace pmbus

namespace power
{
namespace psu
{
static std::unique_ptr<MockedUtil> util;

const UtilBase& getUtils()
{
    if (!util)
    {
        util = std::make_unique<MockedUtil>();
    }
    return *util;
}

void freeUtils()
{
    util.reset();
}

std::unique_ptr<GPIOInterfaceBase> createGPIO(const std::string& /*namedGpio*/)
{
    return std::make_unique<MockedGPIOInterface>();
}

} // namespace psu
} // namespace power
} // namespace phosphor
