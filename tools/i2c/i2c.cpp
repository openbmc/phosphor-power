#include "i2c.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>

extern "C" {
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
}

namespace i2c
{

void I2CDevice::open()
{
    fd = ::open(busStr.c_str(), O_RDWR);
    if (fd == -1)
    {
        throw I2CException("Failed to open", busStr, devAddr, errno);
    }

    if (ioctl(fd, I2C_SLAVE, devAddr) < 0)
    {
        throw I2CException("Failed to set I2C_SLAVE", busStr, devAddr, errno);
    }
}

void I2CDevice::close()
{
    ::close(fd);
}

void I2CDevice::checkReadFuncs(int type)
{
    unsigned long funcs;

    /* Check adapter functionality */
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        throw I2CException("Failed to get funcs", busStr, devAddr, errno);
    }

    switch (type)
    {
        case I2C_SMBUS_BYTE:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE))
            {
                throw I2CException("Missing SMBUS_READ_BYTE", busStr, devAddr);
            }
            break;
        case I2C_SMBUS_BYTE_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
            {
                throw I2CException("Missing SMBUS_READ_BYTE_DATA", busStr,
                                   devAddr);
            }
            break;

        case I2C_SMBUS_WORD_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA))
            {
                throw I2CException("Missing SMBUS_READ_WORD_DATA", busStr,
                                   devAddr);
            }
            break;
        case I2C_SMBUS_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA))
            {
                throw I2CException("Missing SMBUS_READ_BLOCK_DATA", busStr,
                                   devAddr);
            }
            break;
        default:
            fprintf(stderr, "Unexpected read size type: %d\n", type);
            assert(false);
            break;
    }
}

void I2CDevice::read(uint8_t& data)
{
    checkReadFuncs(I2C_SMBUS_BYTE);

    int ret = i2c_smbus_read_byte(fd);
    if (ret < 0)
    {
        throw I2CException("Failed to read byte", busStr, devAddr, errno);
    }
    data = static_cast<uint8_t>(ret);
}

void I2CDevice::read(uint8_t addr, uint8_t& data)
{
    checkReadFuncs(I2C_SMBUS_BYTE_DATA);

    int ret = i2c_smbus_read_byte_data(fd, addr);
    if (ret < 0)
    {
        throw I2CException("Failed to read byte data", busStr, devAddr, errno);
    }
    data = static_cast<uint8_t>(ret);
}

void I2CDevice::read(uint8_t addr, uint16_t& data)
{
    checkReadFuncs(I2C_SMBUS_WORD_DATA);
    int ret = i2c_smbus_read_word_data(fd, addr);
    if (ret < 0)
    {
        throw I2CException("Failed to read word data", busStr, devAddr, errno);
    }
    data = static_cast<uint16_t>(ret);
}

void I2CDevice::read(uint8_t addr, uint8_t& size, uint8_t* data)
{
    checkReadFuncs(I2C_SMBUS_BLOCK_DATA);

    int ret = i2c_smbus_read_block_data(fd, addr, data);
    if (ret < 0)
    {
        throw I2CException("Failed to read block data", busStr, devAddr, errno);
    }
    size = static_cast<uint8_t>(ret);
}

void I2CDevice::write(uint8_t data)
{
    // TODO
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
