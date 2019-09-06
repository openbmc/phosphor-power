#include "config.h"

#include "mihawk-cpld.hpp"

#include "utility.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <elog-errors.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <org/open_power/Witherspoon/Fault/error.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <string>
#include <xyz/openbmc_project/Common/Device/error.hpp>

extern "C" {
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
}

// i2c bus & i2c slave address of Mihawk's CPLD
#define busId 11
#define slaveAddr 0x40

// SMLink Status Register(Interrupt-control-bit Register)
const static constexpr size_t StatusReg_1 = 0x20;

// SMLink Status Register(Power-on error code Register)
const static constexpr size_t StatusReg_2 = 0x21;

// SMLink Status Register(PSU register code Register)
const static constexpr size_t StatusReg_3 = 0x05;

using namespace std;
namespace phosphor
{
namespace power
{
const auto DEVICE_NAME = "MIHAWKCPLD"s;

namespace fs = std::filesystem;
using namespace phosphor::logging;

using namespace sdbusplus::org::open_power::Witherspoon::Fault::Error;

MIHAWKCPLD::MIHAWKCPLD(size_t instance, sdbusplus::bus::bus& bus) :
    Device(DEVICE_NAME, instance), bus(bus)
{
}

void MIHAWKCPLD::onFailure()
{
    bool poweronError = checkPoweronFault();

    // If the interrupt of power_on_error is switch on,
    // read CPLD_register error code to analyze and report the error log event.
    if (poweronError)
    {
        std::cerr << "CPLD : Power_ON error! \n";
        int errorcode;
        errorcode = readFromCPLDPSUErrorCode(busId, slaveAddr);
        if (errorcode == powerOnErrorCode_0)
        {
            report<PowerOnErrorCode0>();
        }
        else if (errorcode == powerOnErrorCode_1)
        {
            report<PowerOnErrorCode1>();
        }
        else if (errorcode == powerOnErrorCode_2)
        {
            report<PowerOnErrorCode2>();
        }
        else if (errorcode == powerOnErrorCode_3)
        {
            report<PowerOnErrorCode3>();
        }
        else if (errorcode == powerOnErrorCode_4)
        {
            report<PowerOnErrorCode4>();
        }
        else if (errorcode == powerOnErrorCode_5)
        {
            report<PowerOnErrorCode5>();
        }
        else if (errorcode == powerOnErrorCode_6)
        {
            report<PowerOnErrorCode6>();
        }
        else if (errorcode == powerOnErrorCode_7)
        {
            report<PowerOnErrorCode7>();
        }
        else if (errorcode == powerOnErrorCode_8)
        {
            report<PowerOnErrorCode8>();
        }
        else if (errorcode == powerOnErrorCode_9)
        {
            report<PowerOnErrorCode9>();
        }
        else if (errorcode == powerOnErrorCode_10)
        {
            report<PowerOnErrorCode10>();
        }
        else if (errorcode == powerOnErrorCode_11)
        {
            report<PowerOnErrorCode11>();
        }
        else if (errorcode == powerOnErrorCode_12)
        {
            report<PowerOnErrorCode12>();
        }
        else if (errorcode == powerOnErrorCode_13)
        {
            report<PowerOnErrorCode13>();
        }
        else if (errorcode == powerOnErrorCode_14)
        {
            report<PowerOnErrorCode14>();
        }
        else if (errorcode == powerOnErrorCode_15)
        {
            report<PowerOnErrorCode15>();
        }
        else if (errorcode == powerOnErrorCode_16)
        {
            report<PowerOnErrorCode16>();
        }
        else if (errorcode == powerOnErrorCode_17)
        {
            report<PowerOnErrorCode17>();
        }
        else if (errorcode == powerOnErrorCode_18)
        {
            report<PowerOnErrorCode18>();
        }
        else if (errorcode == powerOnErrorCode_19)
        {
            report<PowerOnErrorCode19>();
        }
        else if (errorcode == powerOnErrorCode_20)
        {
            report<PowerOnErrorCode20>();
        }
        else if (errorcode == powerOnErrorCode_21)
        {
            report<PowerOnErrorCode21>();
        }
        else if (errorcode == powerOnErrorCode_22)
        {
            report<PowerOnErrorCode22>();
        }
        else if (errorcode == powerOnErrorCode_23)
        {
            report<PowerOnErrorCode23>();
        }
        else if (errorcode == powerOnErrorCode_24)
        {
            report<PowerOnErrorCode24>();
        }
        else if (errorcode == powerOnErrorCode_25)
        {
            report<PowerOnErrorCode25>();
        }
        else if (errorcode == powerOnErrorCode_26)
        {
            report<PowerOnErrorCode26>();
        }
        else if (errorcode == powerOnErrorCode_27)
        {
            report<PowerOnErrorCode27>();
        }
        else if (errorcode == powerOnErrorCode_28)
        {
            report<PowerOnErrorCode28>();
        }
        else if (errorcode == powerOnErrorCode_29)
        {
            report<PowerOnErrorCode29>();
        }
        else if (errorcode == powerOnErrorCode_30)
        {
            report<PowerOnErrorCode30>();
        }
        else if (errorcode == powerOnErrorCode_31)
        {
            report<PowerOnErrorCode31>();
        }
        else if (errorcode == powerOnErrorCode_32)
        {
            report<PowerOnErrorCode32>();
        }
        else if (errorcode == powerOnErrorCode_33)
        {
            report<PowerOnErrorCode33>();
        }
        else if (errorcode == powerOnErrorCode_34)
        {
            report<PowerOnErrorCode34>();
        }
        else if (errorcode == powerOnErrorCode_35)
        {
            report<PowerOnErrorCode35>();
        }
        else if (errorcode == powerOnErrorCode_36)
        {
            report<PowerOnErrorCode36>();
        }
        clearCPLDregister();
    }
}

void MIHAWKCPLD::analyze()
{
    //analysis psu power status when power_on fault.
    auto poweronError = checkPoweronFault();
    if (poweronError)
    {
        int errorcode;
        errorcode = checkPSUDCpgood();
        if (!((errorcode >> 1) & 1) && !((errorcode >> 3) & 1))
        {
            report<PsuErrorCode0>();
        }
        else if (!((errorcode >> 2) & 1) && !((errorcode >> 4) & 1))
        {
            report<PsuErrorCode1>();
        } 
    }
}

// Check for PoweronFault
bool MIHAWKCPLD::checkPoweronFault()
{
    bool result = 0;
    std::string i2cBus = "/dev/i2c-" + std::to_string(busId);

    // open i2c device(CPLD-PSU-register table)
    int fd = open(i2cBus.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0)
    {
        std::cerr << "Unable to open i2c device \n";
    }

    // set i2c slave address
    if (ioctl(fd, I2C_SLAVE_FORCE, slaveAddr) < 0)
    {
        std::cerr << "Unable to set device address \n";
        close(fd);
    }

    // check whether support i2c function
    unsigned long funcs = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        std::cerr << "Not support I2C_FUNCS \n";
        close(fd);
    }

    // check whether support i2c-read function
    if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
    {
        std::cerr << "Not support I2C_FUNC_SMBUS_READ_BYTE_DATA \n";
        close(fd);
    }

    int statusValue_1;

    statusValue_1 = i2c_smbus_read_byte_data(fd, StatusReg_1);
    close(fd);

    if (statusValue_1 < 0)
    {
        std::cerr << "i2c_smbus_read_byte_data failed \n";
        result = 0;
    }

    if ((statusValue_1 >> 5) & 1)
    {
        // If power_on-interrupt-bit is read as 1,
        // switch on the flag.
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

// Read CPLD_register error code and return the result to analyze.
int MIHAWKCPLD::readFromCPLDPSUErrorCode(int bus, int Addr)
{
    std::string i2cBus = "/dev/i2c-" + std::to_string(bus);

    // open i2c device(CPLD-PSU-register table)
    int fd = open(i2cBus.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0)
    {
        std::cerr << "Unable to open i2c device \n";
    }

    // set i2c slave address
    if (ioctl(fd, I2C_SLAVE_FORCE, Addr) < 0)
    {
        std::cerr << "Unable to set device address \n";
        close(fd);
    }

    // check whether support i2c function
    unsigned long funcs = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        std::cerr << "Not support I2C_FUNCS \n";
        close(fd);
    }

    // check whether support i2c-read function
    if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
    {
        std::cerr << "Not support I2C_FUNC_SMBUS_READ_BYTE_DATA \n";
        close(fd);
    }

    int statusValue_2;
    statusValue_2 = i2c_smbus_read_byte_data(fd, StatusReg_2);
    close(fd);

    if (statusValue_2 < 0)
    {
        statusValue_2 = 0;
    }

    // return the i2c-read data
    return statusValue_2;
}

// Check PSU_DC_PGOOD state form PSU register via CPLD
int MIHAWKCPLD::checkPSUDCpgood()
{
    std::string i2cBus = "/dev/i2c-" + std::to_string(busId);

    // open i2c device(CPLD-PSU-register table)
    int fd = open(i2cBus.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0)
    {
        std::cerr << "Unable to open i2c device \n";
    }

    // set i2c slave address
    if (ioctl(fd, I2C_SLAVE_FORCE, slaveAddr) < 0)
    {
        std::cerr << "Unable to set device address \n";
        close(fd);
    }

    // check whether support i2c function
    unsigned long funcs = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        std::cerr << "Not support I2C_FUNCS \n";
        close(fd);
    }

    // check whether support i2c-read function
    if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
    {
        std::cerr << "Not support I2C_FUNC_SMBUS_READ_BYTE_DATA \n";
        close(fd);
    }

    int statusValue_3;
    statusValue_3 = i2c_smbus_read_byte_data(fd, StatusReg_3);
    close(fd);

    // return the i2c-read data
    return statusValue_3;
}

// Clear CPLD_register after reading.
void MIHAWKCPLD::clearCPLDregister()
{
    std::string i2cBus = "/dev/i2c-" + std::to_string(busId);

    // open i2c device(CPLD-PSU-register table)
    int fd = open(i2cBus.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0)
    {
        std::cerr << "Unable to open i2c device \n";
    }

    // set i2c slave address
    if (ioctl(fd, I2C_SLAVE_FORCE, slaveAddr) < 0)
    {
        std::cerr << "Unable to set device address \n";
        close(fd);
    }

    // check whether support i2c function
    unsigned long funcs = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        std::cerr << "Not support I2C_FUNCS \n";
        close(fd);
    }

    // check whether support i2c-write function
    if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
    {
        std::cerr << "Not support I2C_FUNC_SMBUS_WRITE_BYTE_DATA \n";
        close(fd);
    }

    // clear CPLD_register by writing 0x01 to it.
    if ((i2c_smbus_write_byte_data(fd, StatusReg_1, 0x01)) < 0)
    {
        std::cerr << "i2c_smbus_write_byte_data failed \n";
    }
    close(fd);
}

} // namespace power
} // namespace phosphor
