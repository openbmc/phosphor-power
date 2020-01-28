#include <mock.hpp>

namespace phosphor
{
namespace pmbus
{

    std::unique_ptr<PMBusBase> createPMBus(std::uint8_t /*bus*/,
                                           const std::string& /*address*/)
{
    return std::make_unique<MockedPMBus>();
}

}
}
