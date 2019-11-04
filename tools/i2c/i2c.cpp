#include "i2c.hpp"

namespace i2c
{

bool I2CDevice::read(uint8_t addr, uint8_t size, int32_t& data)
{
    // TODO
    (void)addr;
    (void)size;
    (void)data;
    return false;
}

bool I2CDevice::write(uint8_t addr, uint8_t size, const uint8_t* data)
{
    // TODO
    (void)addr;
    (void)size;
    (void)data;
    return false;
}

std::unique_ptr<I2CInterface> I2CDevice::create(uint8_t busId, uint8_t devAddr)
{
    std::unique_ptr<I2CDevice> dev(new I2CDevice(busId, devAddr));
    return dev;
}

std::unique_ptr<I2CInterface> create(uint8_t busId, uint8_t devAddr)
{
    return I2CDevice::create(busId, devAddr);
}

} // namespace i2c
