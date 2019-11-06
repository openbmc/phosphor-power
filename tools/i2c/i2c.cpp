#include "i2c.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

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
