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

// SMLink Status Register(PSU Register)
const static constexpr size_t StatusReg = 0x05;

// SMLink Status Register(Interrupt-control-bit Register)
const static constexpr size_t StatusReg_1 = 0x20;

// SMLink Status Register(Power-on error code Register)
const static constexpr size_t StatusReg_2 = 0x21;

using namespace std;
namespace witherspoon
{
namespace power
{
const auto DEVICE_NAME = "MihawkCPLD"s;

namespace fs = std::filesystem;
using namespace phosphor::logging;

using namespace sdbusplus::org::open_power::Witherspoon::Fault::Error;

namespace device_error = sdbusplus::xyz::openbmc_project::Common::Device::Error;

MihawkCPLD::MihawkCPLD(size_t instance, sdbusplus::bus::bus& bus) :
    Device(DEVICE_NAME, instance), bus(bus)
{
}

void MihawkCPLD::onFailure()
{
    bool psuError = checkPSUdevice();
    bool poweronError = checkPoweronFault();

    if (psuError)
    {
        // If PSU not present, report the error log & event.
        report<PSUPresentError>();
        std::cerr << "CPLD : PSU Present Error! \n";
    }

    // If the interrupt of power_on_error is switch on,
    // read CPLD_register error code to analyze and report the error log
    // & event.
    if (poweronError)
    {
        std::cerr << "CPLD : Power_ON error! \n";
        int errorcode;
        errorcode = readFromCPLDPSUErrorCode(busId, slaveAddr);
        if (errorcode == 0)
        {
            report<PowerOnErrorCPLDRegisterFail>();
            std::cerr << "Power_on error : Read CPLD-register fail! \n";
        }
        else if (errorcode == 1)
        {
            report<PowerOnErrorPSU0PGOODFail>();
            std::cerr << "Power_on error : psu0_pgood fail! \n";
        }
        else if (errorcode == 2)
        {
            report<PowerOnErrorPSU1PGOODFail>();
            std::cerr << "Power_on error : psu1_pgood fail! \n";
        }
        else if (errorcode == 3)
        {
            report<PowerOnError240VaFaultAFail>();
            std::cerr << "Power_on error : 240Va_Fault_A fail! \n";
        }
        else if (errorcode == 4)
        {
            report<PowerOnError240VaFaultBFail>();
            std::cerr << "Power_on error : 240Va_Fault_B fail! \n";
        }
        else if (errorcode == 5)
        {
            report<PowerOnError240VaFaultCFail>();
            std::cerr << "Power_on error : 240Va_Fault_C fail! \n";
        }
        else if (errorcode == 6)
        {
            report<PowerOnError240VaFaultDFail>();
            std::cerr << "Power_on error : 240Va_Fault_D fail! \n";
        }
        else if (errorcode == 7)
        {
            report<PowerOnError240VaFaultEFail>();
            std::cerr << "Power_on error : 240Va_Fault_E fail! \n";
        }
        else if (errorcode == 8)
        {
            report<PowerOnError240VaFaultFFail>();
            std::cerr << "Power_on error : 240Va_Fault_F fail! \n";
        }
        else if (errorcode == 9)
        {
            report<PowerOnError240VaFaultGFail>();
            std::cerr << "Power_on error : 240Va_Fault_G fail! \n";
        }
        else if (errorcode == 10)
        {
            report<PowerOnError240VaFaultHFail>();
            std::cerr << "Power_on error : 240Va_Fault_H fail! \n";
        }
        else if (errorcode == 11)
        {
            report<PowerOnError240VaFaultJFail>();
            std::cerr << "Power_on error : 240Va_Fault_J fail! \n";
        }
        else if (errorcode == 12)
        {
            report<PowerOnError240VaFaultKFail>();
            std::cerr << "Power_on error : 240Va_Fault_K fail! \n";
        }
        else if (errorcode == 13)
        {
            report<PowerOnError240VaFaultLFail>();
            std::cerr << "Power_on error : 240Va_Fault_L fail! \n";
        }
        else if (errorcode == 14)
        {
            report<PowerOnErrorP5VPGOODFail>();
            std::cerr << "Power_on error : P5V_pgood fail! \n";
        }
        else if (errorcode == 15)
        {
            report<PowerOnErrorP3V3PGOODFail>();
            std::cerr << "Power_on error : P3V3_pgood fail! \n";
        }
        else if (errorcode == 16)
        {
            report<PowerOnErrorP1V8PGOODFail>();
            std::cerr << "Power_on error : P1V8_pgood fail! \n";
        }
        else if (errorcode == 17)
        {
            report<PowerOnErrorP1V1PGOODFail>();
            std::cerr << "Power_on error : P1V1_pgood fail! \n";
        }
        else if (errorcode == 18)
        {
            report<PowerOnErrorP0V9PGOODFail>();
            std::cerr << "Power_on error : P0V9_pgood fail! \n";
        }
        else if (errorcode == 19)
        {
            report<PowerOnErrorP2V5APGOODFail>();
            std::cerr << "Power_on error : P2V5A_pgood fail! \n";
        }
        else if (errorcode == 20)
        {
            report<PowerOnErrorP2V5BPGOODFail>();
            std::cerr << "Power_on error : P2V5B_pgood fail! \n";
        }
        else if (errorcode == 21)
        {
            report<PowerOnErrorVdn0PGOODFail>();
            std::cerr << "Power_on error : Vdn0_pgood fail! \n";
        }
        else if (errorcode == 22)
        {
            report<PowerOnErrorVdn1PGOODFail>();
            std::cerr << "Power_on error : Vdn1_pgood fail! \n";
        }
        else if (errorcode == 23)
        {
            report<PowerOnErrorP1V5PGOODFail>();
            std::cerr << "Power_on error : P1V5_pgood fail! \n";
        }
        else if (errorcode == 24)
        {
            report<PowerOnErrorVio0PGOODFail>();
            std::cerr << "Power_on error : Vio0_pgood fail! \n";
        }
        else if (errorcode == 25)
        {
            report<PowerOnErrorVio1PGOODFail>();
            std::cerr << "Power_on error : Vio1_pgood fail! \n";
        }
        else if (errorcode == 26)
        {
            report<PowerOnErrorVdd0PGOODFail>();
            std::cerr << "Power_on error : Vdd0_pgood fail! \n";
        }
        else if (errorcode == 27)
        {
            report<PowerOnErrorVcs0PGOODFail>();
            std::cerr << "Power_on error : Vcs0_pgood fail! \n";
        }
        else if (errorcode == 28)
        {
            report<PowerOnErrorVdd1PGOODFail>();
            std::cerr << "Power_on error : Vdd1_pgood fail! \n";
        }
        else if (errorcode == 29)
        {
            report<PowerOnErrorVcs1PGOODFail>();
            std::cerr << "Power_on error : Vcs1_pgood fail! \n";
        }
        else if (errorcode == 30)
        {
            report<PowerOnErrorVddr0PGOODFail>();
            std::cerr << "Power_on error : Vddr0_pgood fail! \n";
        }
        else if (errorcode == 31)
        {
            report<PowerOnErrorVtt0PGOODFail>();
            std::cerr << "Power_on error : Vtt0_pgood fail! \n";
        }
        else if (errorcode == 32)
        {
            report<PowerOnErrorVddr1PGOODFail>();
            std::cerr << "Power_on error : Vddr1_pgood fail! \n";
        }
        else if (errorcode == 33)
        {
            report<PowerOnErrorVtt1PGOODFail>();
            std::cerr << "Power_on error : Vtt1_pgood fail! \n";
        }
        else if (errorcode == 34)
        {
            report<PowerOnErrorGPU0PGOODFail>();
            std::cerr << "Power_on error : GPU0_pgood fail! \n";
        }
        else if (errorcode == 35)
        {
            report<PowerOnErrorGPU1PGOODFail>();
            std::cerr << "Power_on error : GPU1_pgood fail! \n";
        }
        else if (errorcode == 170)
        {
            report<PowerOnErrorPSU0PSU1PGOODFail>();
            std::cerr << "Power_on error : Psu0&PSU1_pgood fail! \n";
        }
    }
}

void MihawkCPLD::analyze()
{
    bool psuError = checkPSUdevice();
    if (psuError)
    {
        // If PSU not present, report the error log & event.
        report<PSUPresentError>();
        std::cerr << "CPLD : PSU Present Error! \n";
    }
}

// Read CPLD_register error code and return the result to analyze.
int MihawkCPLD::readFromCPLDPSUErrorCode(int bus, int Addr)
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
    unsigned int statusReg_2 = StatusReg_2;

    statusValue_2 = i2c_smbus_read_byte_data(fd, statusReg_2);
    close(fd);

    if (statusValue_2 < 0)
    {
        statusValue_2 = 0;
    }

    // return the i2c-read data
    return statusValue_2;
}

// Check PSU_present_pin.
bool MihawkCPLD::checkPSUdevice()
{
    bool result = 0;
    int bin[32], num;
    int i = 0, length = 0;
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

    int statusValue;
    unsigned int statusReg = StatusReg;

    statusValue = i2c_smbus_read_byte_data(fd, statusReg);
    close(fd);

    if (statusValue < 0)
    {
        std::cerr << "i2c_smbus_read_byte_data failed \n";
    }

    // Transfer decimal_value to binary_value
    num = statusValue;
    do
    {
        bin[i] = num % 2;
        num = num / 2;
        i++;
        length++;
    } while (num != 1);
    bin[length] = num;
    length++;

    if (bin[3] == 1 && bin[4] == 1)
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

// Check for PoweronFault
bool MihawkCPLD::checkPoweronFault()
{
    bool result_1 = 0;
    int bin1[32], num1;
    int i = 0, length = 0;
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
    unsigned int statusReg_1 = StatusReg_1;

    statusValue_1 = i2c_smbus_read_byte_data(fd, statusReg_1);
    close(fd);

    if (statusValue_1 < 0)
    {
        std::cerr << "i2c_smbus_read_byte_data failed \n";
    }

    // Transfer decimal_value to binary_value
    num1 = statusValue_1;
    do
    {
        bin1[i] = num1 % 2;
        num1 = num1 / 2;
        i++;
        length++;
    } while (num1 != 1);
    bin1[length] = num1;
    length++;

    if (bin1[5] == 1)
    {
        // If power_on-interrupt-bit is read as 1,
        // switch on the flag.
        result_1 = 1;
    }
    else
    {
        result_1 = 0;
    }

    return result_1;
}

} // namespace power
} // namespace witherspoon
