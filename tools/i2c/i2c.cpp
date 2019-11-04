#include "i2c.hpp"

namespace i2c
{

void I2CDevice::read(uint8_t addr, uint8_t size, std::vector<uint8_t>& data)
{
    // TODO
    (void)addr;
    (void)size;
    (void)data;
}

bool I2CDevice::write(uint8_t addr, uint8_t size, const uint8_t* data)
{
    // TODO
    (void)addr;
    (void)size;
    (void)data;
    return false;
}

std::unique_ptr<I2CInterface> I2CDevice::Create(uint8_t busId, uint8_t devAddr)
{
    std::unique_ptr<I2CDevice> dev(new I2CDevice(busId, devAddr));
    return dev;
}

std::unique_ptr<I2CInterface> Create(uint8_t busId, uint8_t devAddr)
{
    return I2CDevice::Create(busId, devAddr);
}

} // namespace i2c
