#include "config.h"

#include "mihawk-cpld.hpp"

#include "gpio.hpp"
#include "utility.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <elog-errors.hpp>
#include <org/open_power/Witherspoon/Fault/error.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <string>

// i2c bus & i2c slave address of Mihawk's CPLD_register
static constexpr uint8_t busId = 11;
static constexpr uint8_t slaveAddr = 0x40;

// SMLink Status Register(PSU status Register)
static constexpr size_t StatusReg_0 = 0x05;

// SMLink Status Register(Interrupt-control-bit Register)
static constexpr size_t StatusReg_1 = 0x20;

// SMLink Status Register(Power-on error code Register)
static constexpr size_t StatusReg_2 = 0x21;

// SMLink Status Register(Power-ready error code Register)
static constexpr size_t StatusReg_3 = 0x22;

using namespace std;

namespace phosphor
{
namespace power
{
const auto DEVICE_NAME = "MihawkCPLD"s;

namespace fs = std::filesystem;
using namespace phosphor::logging;

using namespace sdbusplus::org::open_power::Witherspoon::Fault::Error;

MihawkCPLD::MihawkCPLD(size_t instance, sdbusplus::bus_t& bus) :
    Device(DEVICE_NAME, instance), bus(bus)
{}

void MihawkCPLD::onFailure()
{
    bool poweronError = checkPoweronFault();

    // If the interrupt of power_on_error is switch on,
    // read CPLD_register error code to analyze
    // and report the error log event.
    if (poweronError)
    {
        ErrorCode code;
        code = static_cast<ErrorCode>(readFromCPLDErrorCode(StatusReg_2));

        switch (code)
        {
            case ErrorCode::_1:
                report<ErrorCode1>();
                break;
            case ErrorCode::_2:
                report<ErrorCode2>();
                break;
            case ErrorCode::_3:
                report<ErrorCode3>();
                break;
            case ErrorCode::_4:
                report<ErrorCode4>();
                break;
            case ErrorCode::_5:
                report<ErrorCode5>();
                break;
            case ErrorCode::_6:
                report<ErrorCode6>();
                break;
            case ErrorCode::_7:
                report<ErrorCode7>();
                break;
            case ErrorCode::_8:
                report<ErrorCode8>();
                break;
            case ErrorCode::_9:
                report<ErrorCode9>();
                break;
            case ErrorCode::_10:
                report<ErrorCode10>();
                break;
            case ErrorCode::_11:
                report<ErrorCode11>();
                break;
            case ErrorCode::_12:
                report<ErrorCode12>();
                break;
            case ErrorCode::_13:
                report<ErrorCode13>();
                break;
            case ErrorCode::_14:
                report<ErrorCode14>();
                break;
            case ErrorCode::_15:
                report<ErrorCode15>();
                break;
            case ErrorCode::_16:
                report<ErrorCode16>();
                break;
            case ErrorCode::_17:
                report<ErrorCode17>();
                break;
            case ErrorCode::_18:
                report<ErrorCode18>();
                break;
            case ErrorCode::_19:
                report<ErrorCode19>();
                break;
            case ErrorCode::_20:
                report<ErrorCode20>();
                break;
            case ErrorCode::_21:
                report<ErrorCode21>();
                break;
            case ErrorCode::_22:
                report<ErrorCode22>();
                break;
            case ErrorCode::_23:
                report<ErrorCode23>();
                break;
            case ErrorCode::_24:
                report<ErrorCode24>();
                break;
            case ErrorCode::_25:
                report<ErrorCode25>();
                break;
            case ErrorCode::_26:
                report<ErrorCode26>();
                break;
            case ErrorCode::_27:
                report<ErrorCode27>();
                break;
            case ErrorCode::_28:
                report<ErrorCode28>();
                break;
            case ErrorCode::_29:
                report<ErrorCode29>();
                break;
            case ErrorCode::_30:
                report<ErrorCode30>();
                break;
            case ErrorCode::_31:
                report<ErrorCode31>();
                break;
            case ErrorCode::_32:
                report<ErrorCode32>();
                break;
            case ErrorCode::_33:
                report<ErrorCode33>();
                break;
            case ErrorCode::_34:
                report<ErrorCode34>();
                break;
            case ErrorCode::_35:
                report<ErrorCode35>();
                break;
            case ErrorCode::_36:
                report<ErrorCode36>();
                break;
            default:
                // If the errorcode isn't 1~36,
                // it indicates that the CPLD register
                // has a reading issue,
                // so the errorcode0 error is reported.
                report<ErrorCode0>();
                break;
        }
        clearCPLDregister();
    }
}

void MihawkCPLD::analyze()
{
    bool powerreadyError = checkPowerreadyFault();

    // Use the function of GPIO class to check
    // GPIOF0(CPLD uses).
    using namespace phosphor::gpio;
    GPIO gpio{"/dev/gpiochip0", static_cast<gpioNum_t>(40), Direction::input};

    // Check GPIOFO pin whether is switched off.
    // if GPIOF0 has been switched off,
    // check CPLD's errorcode & report error.
    if (gpio.read() == Value::low)
    {
        // If the interrupt of power_ready_error is switch on,
        // read CPLD_register error code to analyze and
        // report the error event.
        if (powerreadyError)
        {
            ErrorCode code;
            code = static_cast<ErrorCode>(readFromCPLDErrorCode(StatusReg_3));

            if (!errorcodeMask)
            {
                // Check the errorcodeMask & errorcode whether
                // are the same value to avoid to report the
                // same error again.
                switch (code)
                {
                    case ErrorCode::_1:
                        report<ErrorCode1>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_2:
                        report<ErrorCode2>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_3:
                        report<ErrorCode3>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_4:
                        report<ErrorCode4>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_5:
                        report<ErrorCode5>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_6:
                        report<ErrorCode6>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_7:
                        report<ErrorCode7>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_8:
                        report<ErrorCode8>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_9:
                        report<ErrorCode9>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_10:
                        report<ErrorCode10>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_11:
                        report<ErrorCode11>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_12:
                        report<ErrorCode12>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_13:
                        report<ErrorCode13>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_14:
                        report<ErrorCode14>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_15:
                        report<ErrorCode15>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_16:
                        report<ErrorCode16>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_17:
                        report<ErrorCode17>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_18:
                        report<ErrorCode18>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_19:
                        report<ErrorCode19>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_20:
                        report<ErrorCode20>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_21:
                        report<ErrorCode21>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_22:
                        report<ErrorCode22>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_23:
                        report<ErrorCode23>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_24:
                        report<ErrorCode24>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_25:
                        report<ErrorCode25>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_26:
                        report<ErrorCode26>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_27:
                        report<ErrorCode27>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_28:
                        report<ErrorCode28>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_29:
                        report<ErrorCode29>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_30:
                        report<ErrorCode30>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_31:
                        report<ErrorCode31>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_32:
                        report<ErrorCode32>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_33:
                        report<ErrorCode33>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_34:
                        report<ErrorCode34>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_35:
                        report<ErrorCode35>();
                        errorcodeMask = 1;
                        break;
                    case ErrorCode::_36:
                        report<ErrorCode36>();
                        errorcodeMask = 1;
                        break;
                    default:
                        // If the errorcode is not 1~36,
                        // it indicates that the CPLD register
                        // has a reading issue, so the
                        // errorcode0 error is reported.
                        report<ErrorCode0>();
                        errorcodeMask = 1;
                        break;
                }
            }
            clearCPLDregister();
        }
    }

    if (gpio.read() == Value::high)
    {
        // If there isn't an error(GPIOF0
        // which CPLD uses is switched on),
        // we clear errorcodeMask.
        errorcodeMask = 0;
    }
}

// Check for PoweronFault
bool MihawkCPLD::checkPoweronFault()
{
    uint16_t statusValue_1;
    bool result;

    if (!i2c)
    {
        openCPLDDevice();
    }
    i2c->read(StatusReg_1, statusValue_1);

    if (statusValue_1 < 0)
    {
        std::cerr << "i2c->read() reads data failed \n";
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

    // Return the flag.
    return result;
}

// Read CPLD_register error code and return the result to analyze.
int MihawkCPLD::readFromCPLDErrorCode(int statusReg)
{
    uint16_t statusValue_2;

    if (!i2c)
    {
        openCPLDDevice();
    }
    i2c->read(statusReg, statusValue_2);

    if (statusValue_2 < 0 ||
        ((statusValue_2 > static_cast<int>(ErrorCode::_35)) &&
         (statusValue_2 != static_cast<int>(ErrorCode::_36))))
    {
        statusValue_2 = 0;
    }

    // Return the data via i2c->read().
    return statusValue_2;
}

// Check for PowerreadyFault
bool MihawkCPLD::checkPowerreadyFault()
{
    uint16_t statusValue_3;
    bool result;

    if (!i2c)
    {
        openCPLDDevice();
    }
    i2c->read(StatusReg_1, statusValue_3);

    if (statusValue_3 < 0)
    {
        std::cerr << "i2c->read() reads data failed \n";
        result = 0;
    }

    if ((statusValue_3 >> 6) & 1)
    {
        // If power_ready-interrupt-bit is read as 1,
        // switch on the flag.
        result = 1;
    }
    else
    {
        result = 0;
    }

    // Return the flag.
    return result;
}

// Clear CPLD_register after reading.
void MihawkCPLD::clearCPLDregister()
{
    uint16_t data = 0x01;
    uint16_t checkpsu;

    if (!i2c)
    {
        openCPLDDevice();
    }

    // check psu pgood status.
    i2c->read(StatusReg_0, checkpsu);

    // check one of both psus pgood status before
    // clear CPLD_register.
    if (((checkpsu >> 1) & 1) || ((checkpsu >> 2) & 1))
    {
        // Write 0x01 to StatusReg_1 for clearing
        // CPLD_register.
        i2c->write(StatusReg_1, data);
    }
}

// Open i2c device(CPLD_register)
void MihawkCPLD::openCPLDDevice()
{
    i2c = i2c::create(busId, slaveAddr);
}

} // namespace power
} // namespace phosphor
