#include "i2c.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

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
    const char* extInfo;
    fd = ::open(busStr.c_str(), O_RDWR);
    if (fd != -1)
    {
        if (ioctl(fd, I2C_SLAVE, devAddr) != -1)
        {
            return;
        }
        else
        {
            extInfo = "Failed to set I2C_SLAVE";
        }
    }
    else
    {
        extInfo = "Failed to open device";
    }
    // Throw IIC error when it fails to open the device or fails to set the
    // slave address
    throw I2CException(busStr, devAddr, errno, extInfo);
}

void I2CDevice::close()
{
    ::close(fd);
}

void I2CDevice::read(uint8_t addr, uint8_t& data)
{
    int i2cSize = (addr == 0) ? I2C_SMBUS_BYTE : I2C_SMBUS_BYTE_DATA;
    int ret;

    if (!checkReadFuncs(i2cSize))
    {
        throw I2CException(busStr, devAddr, 0, "Unsupported function");
    }

    if (i2cSize == I2C_SMBUS_BYTE)
    {
        ret = i2c_smbus_read_byte(fd);
    }
    else
    {
        ret = i2c_smbus_read_byte_data(fd, addr);
    }
    if (ret == -1)
    {
        throw I2CException(busStr, devAddr, errno, "Failed to read");
    }
    data = static_cast<uint8_t>(ret);
}

void I2CDevice::read(uint8_t addr, uint16_t& data)
{
    int i2cSize = I2C_SMBUS_WORD_DATA;
    int ret;

    if (!checkReadFuncs(i2cSize))
    {
        throw I2CException(busStr, devAddr, 0, "Unsupported function");
    }

    ret = i2c_smbus_read_word_data(fd, addr);
    if (ret == -1)
    {
        throw I2CException(busStr, devAddr, errno, "Failed to read");
    }
    data = static_cast<uint16_t>(ret);
}

void I2CDevice::read(uint8_t addr, uint8_t size, uint8_t* data)
{
    int i2cSize;
    int ret;
    switch (size)
    {
        case 0:
            throw I2CException(busStr, devAddr, 0, "Invalid read size 0");
        case 1:
            i2cSize = I2C_SMBUS_BYTE_DATA;
            break;
        default:
            // Block read
            i2cSize = I2C_SMBUS_BLOCK_DATA;
            break;
    }

    if (!checkReadFuncs(i2cSize))
    {
        throw I2CException(busStr, devAddr, 0, "Unsupported function");
    }

    if (i2cSize == I2C_SMBUS_BYTE_DATA)
    {
        ret = i2c_smbus_read_byte_data(fd, addr);
        data[0] = static_cast<uint8_t>(ret);
    }
    else
    {
        // TODO
        ret = -1;
    }
    if (ret == -1)
    {
        throw I2CException(busStr, devAddr, errno, "Failed to read");
    }
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

bool I2CDevice::checkReadFuncs(int size)
{
    bool ret = true;
    unsigned long funcs;

    /* Check adapter functionality */
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        perror("Failed to get funcs");
        return false;
    }

    switch (size)
    {
        case I2C_SMBUS_BYTE:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE))
            {
                fprintf(stderr, "Missing SMBUS_READ_BYTE");
                ret = false;
            }
            break;
        case I2C_SMBUS_BYTE_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
            {
                fprintf(stderr, "Missing SMBUS_READ_BYTE_DATA");
                ret = false;
            }
            break;

        case I2C_SMBUS_WORD_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA))
            {
                fprintf(stderr, "Missing SMBUS_READ_WORD_DATA");
                ret = false;
            }
            break;
        case I2C_SMBUS_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA))
            {
                fprintf(stderr, "Missing SMBUS_READ_WORD_DATA");
                ret = false;
            }
            break;
        default:
            ret = false;
    }
    return ret;
}
} // namespace i2c
