#include "i2c.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

namespace i2c
{

void I2CDevice::open()
{
    fd = ::open(busStr.c_str(), O_RDWR);
    if (fd == -1)
    {
        throw I2CException(busStr, devAddr, errno);
    }
}

void I2CDevice::close()
{
    ::close(fd);
}

void I2CDevice::read(uint8_t addr, uint8_t& data)
{
    // TODO
    (void)addr;
    (void)data;
}

void I2CDevice::read(uint8_t addr, uint16_t& data)
{
    // TODO
    (void)addr;
    (void)data;
}

void I2CDevice::read(uint8_t addr, uint8_t size, uint8_t* data)
{
    // TODO
    (void)addr;
    (void)size;
    (void)data;
}

void I2CDevice::write(uint8_t addr, uint8_t data)
{
    // TODO
    (void)addr;
    (void)data;
}

void I2CDevice::write(uint8_t addr, uint16_t data)
{
    // TODO
    (void)addr;
    (void)data;
}

void I2CDevice::write(uint8_t addr, uint8_t size, const uint8_t* data)
{
    // TODO
    (void)addr;
    (void)size;
    (void)data;
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
