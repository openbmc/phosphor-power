#pragma once

#include <sdbusplus/exception.hpp>

namespace sdbusplus
{
namespace org
{
namespace open_power
{
namespace Witherspoon
{
namespace Fault
{
namespace Error
{

struct PowerSupplyInputFault final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSupplyInputFault";
    static constexpr auto errDesc =
            "The power supply has indicated an input or under voltage condition. Check the power supply cabling and/or input power source.";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSupplyInputFault: The power supply has indicated an input or under voltage condition. Check the power supply cabling and/or input power source.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerSupplyShouldBeOn final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSupplyShouldBeOn";
    static constexpr auto errDesc =
            "The power supply indicated that it is not on when it should be.";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSupplyShouldBeOn: The power supply indicated that it is not on when it should be.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerSupplyOutputOvercurrent final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSupplyOutputOvercurrent";
    static constexpr auto errDesc =
            "The power supply detected an output overcurrent fault condition.";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSupplyOutputOvercurrent: The power supply detected an output overcurrent fault condition.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerSupplyOutputOvervoltage final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSupplyOutputOvervoltage";
    static constexpr auto errDesc =
            "The power supply detected an output overvoltage fault condition.";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSupplyOutputOvervoltage: The power supply detected an output overvoltage fault condition.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerSupplyFanFault final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSupplyFanFault";
    static constexpr auto errDesc =
            "The power supply detected bad fan operation.";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSupplyFanFault: The power supply detected bad fan operation.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerSupplyTemperatureFault final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSupplyTemperatureFault";
    static constexpr auto errDesc =
            "The power supply has had an over temperature condition.";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSupplyTemperatureFault: The power supply has had an over temperature condition.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct Shutdown final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.Shutdown";
    static constexpr auto errDesc =
            "A power off was issued because a power fault was detected";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.Shutdown: A power off was issued because a power fault was detected";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerOnFailure final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerOnFailure";
    static constexpr auto errDesc =
            "System power failed to turn on";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerOnFailure: System power failed to turn on";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode0 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode0";
    static constexpr auto errDesc =
            "Read CPLD-register fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode0: Read CPLD-register fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode1 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode1";
    static constexpr auto errDesc =
            "CPLD's error reason is PSU0_PGOOD fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode1: CPLD's error reason is PSU0_PGOOD fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode2 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode2";
    static constexpr auto errDesc =
            "CPLD's error reason is PSU1_PGOOD fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode2: CPLD's error reason is PSU1_PGOOD fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode3 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode3";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_A fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode3: CPLD's error reason is 240Va_Fault_A fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode4 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode4";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_B fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode4: CPLD's error reason is 240Va_Fault_B fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode5 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode5";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_C fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode5: CPLD's error reason is 240Va_Fault_C fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode6 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode6";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_D fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode6: CPLD's error reason is 240Va_Fault_D fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode7 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode7";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_E fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode7: CPLD's error reason is 240Va_Fault_E fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode8 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode8";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_F fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode8: CPLD's error reason is 240Va_Fault_F fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode9 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode9";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_G fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode9: CPLD's error reason is 240Va_Fault_G fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode10 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode10";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_H fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode10: CPLD's error reason is 240Va_Fault_H fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode11 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode11";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_J fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode11: CPLD's error reason is 240Va_Fault_J fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode12 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode12";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_K fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode12: CPLD's error reason is 240Va_Fault_K fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode13 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode13";
    static constexpr auto errDesc =
            "CPLD's error reason is 240Va_Fault_L fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode13: CPLD's error reason is 240Va_Fault_L fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode14 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode14";
    static constexpr auto errDesc =
            "CPLD's error reason is P5V_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode14: CPLD's error reason is P5V_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode15 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode15";
    static constexpr auto errDesc =
            "CPLD's error reason is P3V3_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode15: CPLD's error reason is P3V3_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode16 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode16";
    static constexpr auto errDesc =
            "CPLD's error reason is P1V8_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode16: CPLD's error reason is P1V8_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode17 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode17";
    static constexpr auto errDesc =
            "CPLD's error reason is P1V1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode17: CPLD's error reason is P1V1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode18 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode18";
    static constexpr auto errDesc =
            "CPLD's error reason is P0V9_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode18: CPLD's error reason is P0V9_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode19 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode19";
    static constexpr auto errDesc =
            "CPLD's error reason is P2V5A_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode19: CPLD's error reason is P2V5A_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode20 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode20";
    static constexpr auto errDesc =
            "CPLD's error reason is P2V5B_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode20: CPLD's error reason is P2V5B_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode21 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode21";
    static constexpr auto errDesc =
            "CPLD's error reason is Vdn0_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode21: CPLD's error reason is Vdn0_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode22 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode22";
    static constexpr auto errDesc =
            "CPLD's error reason is Vdn1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode22: CPLD's error reason is Vdn1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode23 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode23";
    static constexpr auto errDesc =
            "CPLD's error reason is P1V5_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode23: CPLD's error reason is P1V5_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode24 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode24";
    static constexpr auto errDesc =
            "CPLD's error reason is Vio0_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode24: CPLD's error reason is Vio0_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode25 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode25";
    static constexpr auto errDesc =
            "CPLD's error reason is Vio1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode25: CPLD's error reason is Vio1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode26 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode26";
    static constexpr auto errDesc =
            "CPLD's error reason is Vdd0_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode26: CPLD's error reason is Vdd0_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode27 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode27";
    static constexpr auto errDesc =
            "CPLD's error reason is Vcs0_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode27: CPLD's error reason is Vcs0_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode28 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode28";
    static constexpr auto errDesc =
            "CPLD's error reason is Vdd1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode28: CPLD's error reason is Vdd1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode29 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode29";
    static constexpr auto errDesc =
            "CPLD's error reason is Vcs1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode29: CPLD's error reason is Vcs1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode30 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode30";
    static constexpr auto errDesc =
            "CPLD's error reason is Vddr0_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode30: CPLD's error reason is Vddr0_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode31 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode31";
    static constexpr auto errDesc =
            "CPLD's error reason is Vtt0_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode31: CPLD's error reason is Vtt0_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode32 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode32";
    static constexpr auto errDesc =
            "CPLD's error reason is Vddr1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode32: CPLD's error reason is Vddr1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode33 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode33";
    static constexpr auto errDesc =
            "CPLD's error reason is Vtt1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode33: CPLD's error reason is Vtt1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode34 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode34";
    static constexpr auto errDesc =
            "CPLD's error reason is GPU0_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode34: CPLD's error reason is GPU0_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode35 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode35";
    static constexpr auto errDesc =
            "CPLD's error reason is GPU1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode35: CPLD's error reason is GPU1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct ErrorCode36 final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.ErrorCode36";
    static constexpr auto errDesc =
            "CPLD's error reason is PSU0&PSU1_pgood fail";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.ErrorCode36: CPLD's error reason is PSU0&PSU1_pgood fail";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerSequencerVoltageFault final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSequencerVoltageFault";
    static constexpr auto errDesc =
            "The power sequencer chip detected a voltage fault";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSequencerVoltageFault: The power sequencer chip detected a voltage fault";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerSequencerPGOODFault final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSequencerPGOODFault";
    static constexpr auto errDesc =
            "The power sequencer chip detected a PGOOD fault";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSequencerPGOODFault: The power sequencer chip detected a PGOOD fault";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct PowerSequencerFault final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.PowerSequencerFault";
    static constexpr auto errDesc =
            "The power sequencer chip detected a fault";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.PowerSequencerFault: The power sequencer chip detected a fault";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct GPUPowerFault final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.GPUPowerFault";
    static constexpr auto errDesc =
            "A GPU suffered a power fault";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.GPUPowerFault: A GPU suffered a power fault";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct GPUOverTemp final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.GPUOverTemp";
    static constexpr auto errDesc =
            "A GPU suffered an over-temperature fault";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.GPUOverTemp: A GPU suffered an over-temperature fault";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct MemoryPowerFault final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Witherspoon.Fault.Error.MemoryPowerFault";
    static constexpr auto errDesc =
            "A memory device suffered a power fault";
    static constexpr auto errWhat =
            "org.open_power.Witherspoon.Fault.Error.MemoryPowerFault: A memory device suffered a power fault";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

} // namespace Error
} // namespace Fault
} // namespace Witherspoon
} // namespace open_power
} // namespace org
} // namespace sdbusplus

