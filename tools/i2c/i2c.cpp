#include "i2c.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>

extern "C"
{
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
}

namespace i2c
{

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
        case I2C_SMBUS_I2C_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_I2C_BLOCK))
            {
                throw I2CException("Missing I2C_FUNC_SMBUS_READ_I2C_BLOCK",
                                   busStr, devAddr);
            }
            break;
        default:
            fprintf(stderr, "Unexpected read size type: %d\n", type);
            assert(false);
            break;
    }
}

void I2CDevice::checkWriteFuncs(int type)
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
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE))
            {
                throw I2CException("Missing SMBUS_WRITE_BYTE", busStr, devAddr);
            }
            break;
        case I2C_SMBUS_BYTE_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
            {
                throw I2CException("Missing SMBUS_WRITE_BYTE_DATA", busStr,
                                   devAddr);
            }
            break;

        case I2C_SMBUS_WORD_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA))
            {
                throw I2CException("Missing SMBUS_WRITE_WORD_DATA", busStr,
                                   devAddr);
            }
            break;
        case I2C_SMBUS_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_BLOCK_DATA))
            {
                throw I2CException("Missing SMBUS_WRITE_BLOCK_DATA", busStr,
                                   devAddr);
            }
            break;
        case I2C_SMBUS_I2C_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_I2C_BLOCK))
            {
                throw I2CException("Missing I2C_FUNC_SMBUS_WRITE_I2C_BLOCK",
                                   busStr, devAddr);
            }
            break;
        default:
            fprintf(stderr, "Unexpected read size type: %d\n", type);
            assert(false);
    }
}

void I2CDevice::open()
{
    if (isOpen())
    {
        throw I2CException("Device already open", busStr, devAddr);
    }

    fd = ::open(busStr.c_str(), O_RDWR);
    if (fd == -1)
    {
        throw I2CException("Failed to open", busStr, devAddr, errno);
    }

    if (ioctl(fd, I2C_SLAVE, devAddr) < 0)
    {
        // Close device since setting slave address failed
        closeWithoutException();

        throw I2CException("Failed to set I2C_SLAVE", busStr, devAddr, errno);
    }
}

void I2CDevice::close()
{
    checkIsOpen();
    if (::close(fd) == -1)
    {
        throw I2CException("Failed to close", busStr, devAddr, errno);
    }
    fd = INVALID_FD;
}

void I2CDevice::read(uint8_t& data)
{
    checkIsOpen();
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
    checkIsOpen();
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
    checkIsOpen();
    checkReadFuncs(I2C_SMBUS_WORD_DATA);
    int ret = i2c_smbus_read_word_data(fd, addr);
    if (ret < 0)
    {
        throw I2CException("Failed to read word data", busStr, devAddr, errno);
    }
    data = static_cast<uint16_t>(ret);
}

void I2CDevice::read(uint8_t addr, uint8_t& size, uint8_t* data, Mode mode)
{
    checkIsOpen();
    int ret;
    switch (mode)
    {
        case Mode::SMBUS:
            checkReadFuncs(I2C_SMBUS_BLOCK_DATA);
            ret = i2c_smbus_read_block_data(fd, addr, data);
            break;
        case Mode::I2C:
            checkReadFuncs(I2C_SMBUS_I2C_BLOCK_DATA);
            ret = i2c_smbus_read_i2c_block_data(fd, addr, size, data);
            if (ret != size)
            {
                throw I2CException("Failed to read i2c block data", busStr,
                                   devAddr, errno);
            }
            break;
    }
    if (ret < 0)
    {
        throw I2CException("Failed to read block data", busStr, devAddr, errno);
    }
    size = static_cast<uint8_t>(ret);
}

void I2CDevice::write(uint8_t data)
{
    checkIsOpen();
    checkWriteFuncs(I2C_SMBUS_BYTE);

    if (i2c_smbus_write_byte(fd, data) < 0)
    {
        throw I2CException("Failed to write byte", busStr, devAddr, errno);
    }
}

void I2CDevice::write(uint8_t addr, uint8_t data)
{
    checkIsOpen();
    checkWriteFuncs(I2C_SMBUS_BYTE_DATA);

    if (i2c_smbus_write_byte_data(fd, addr, data) < 0)
    {
        throw I2CException("Failed to write byte data", busStr, devAddr, errno);
    }
}

void I2CDevice::write(uint8_t addr, uint16_t data)
{
    checkIsOpen();
    checkWriteFuncs(I2C_SMBUS_WORD_DATA);

    if (i2c_smbus_write_word_data(fd, addr, data) < 0)
    {
        throw I2CException("Failed to write word data", busStr, devAddr, errno);
    }
}

void I2CDevice::write(uint8_t addr, uint8_t size, const uint8_t* data,
                      Mode mode)
{
    checkIsOpen();
    int ret;
    switch (mode)
    {
        case Mode::SMBUS:
            checkWriteFuncs(I2C_SMBUS_BLOCK_DATA);
            ret = i2c_smbus_write_block_data(fd, addr, size, data);
            break;
        case Mode::I2C:
            checkWriteFuncs(I2C_SMBUS_I2C_BLOCK_DATA);
            ret = i2c_smbus_write_i2c_block_data(fd, addr, size, data);
            break;
    }
    if (ret < 0)
    {
        throw I2CException("Failed to write block data", busStr, devAddr,
                           errno);
    }
}

std::unique_ptr<I2CInterface> I2CDevice::create(uint8_t busId, uint8_t devAddr,
                                                InitialState initialState)
{
    std::unique_ptr<I2CDevice> dev(new I2CDevice(busId, devAddr, initialState));
    return dev;
}

std::unique_ptr<I2CInterface> create(uint8_t busId, uint8_t devAddr,
                                     I2CInterface::InitialState initialState)
{
    return I2CDevice::create(busId, devAddr, initialState);
}

} // namespace i2c
