#include "i2c.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
}

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

namespace i2c
{

void I2CDevice::open()
{
    fd = ::open(busStr.c_str(), O_RDWR);
    if (fd == -1)
    {
        log<level::ERR>("Failed to open device",
                        entry("DEV=%s", busStr.c_str()));
        elog<InternalFailure>();
    }
}

void I2CDevice::close()
{
    ::close(fd);
}

bool I2CDevice::read(uint8_t addr, uint8_t size, int32_t& res, bool pec)
{
    int i2cSize = I2C_SMBUS_BYTE_DATA;
    switch (size)
    {
        case 0:
            return false;
        case 1:
            i2cSize = I2C_SMBUS_BYTE_DATA;
            break;
        case 2:
            i2cSize = I2C_SMBUS_WORD_DATA;
            break;
        default:
            log<level::ERR>("Invalid i2c read size", entry("ADDR=%x", addr),
                            entry("SIZE=%u", size));
            return false;
    }

    if (!checkFuncs(i2cSize, pec))
    {
        return false;
    }
    if (ioctl(fd, I2C_SLAVE_FORCE, devAddr) < 0)
    {
        log<level::ERR>("Failed to set address", entry("ADDR=%x", addr));
        return false;
    }
    if (pec && !setPEC(true))
    {
        return false;
    }
    if (i2cSize == I2C_SMBUS_WORD_DATA)
    {
        res = i2c_smbus_read_word_data(fd, addr);
    }
    else if (i2cSize == I2C_SMBUS_BYTE_DATA)
    {
        res = i2c_smbus_read_byte_data(fd, addr);
    }
    if (res == -1)
    {
        log<level::ERR>("Failed to read i2c", entry("ERR=%s", strerror(errno)));
        return false;
    }
    return true;
}

bool I2CDevice::write(uint8_t addr, uint8_t size, const uint8_t* data, bool pec)
{
    // TODO
    (void)addr;
    (void)size;
    (void)data;
    (void)pec;
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

bool I2CDevice::checkFuncs(int size, bool pec)
{
    unsigned long funcs;

    /* Check adapter functionality */
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        log<level::ERR>("Failed to get funcs", entry("DEV=%s", busStr.c_str()),
                        entry("ERR=%s", strerror(errno)));
        return false;
    }

    switch (size)
    {
        case I2C_SMBUS_BYTE:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE))
            {
                log<level::ERR>("Missing SMBUS_READ_BYTE");
                return false;
            }
            break;
        case I2C_SMBUS_BYTE_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
            {
                log<level::ERR>("Missing SMBUS_READ_BYTE_DATA");
                return false;
            }
            break;

        case I2C_SMBUS_WORD_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA))
            {
                log<level::ERR>("Missing SMBUS_READ_WORD_DATA");
                return false;
            }
            break;
        case I2C_SMBUS_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_BLOCK_DATA))
            {
                log<level::ERR>("Missing SMBUS_WRITE_BLOCK_DATA");
                return false;
            }
            break;
        case I2C_SMBUS_I2C_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_I2C_BLOCK))
            {
                log<level::ERR>("Missing SMBUS_WRITE_I2C_BLOCK");
                return false;
            }
            break;
    }

    if (pec && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C)))
    {
        log<level::ERR>("PEC not supported");
    }
    return true;
}

bool I2CDevice::setPEC(bool enable)
{
    int data = enable ? 1 : 0;
    if (ioctl(fd, I2C_PEC, data) < 0)
    {
        log<level::ERR>("Failed to set/clear PEC",
                        entry("ERR=%s", strerror(errno)),
                        entry("ENABLE=%d", data));
        return false;
    }
    return true;
}

} // namespace i2c
