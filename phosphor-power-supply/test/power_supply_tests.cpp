#include "../power_supply.hpp"
#include "mock.hpp"

#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::psu;
using namespace phosphor::pmbus;

using ::testing::_;
using ::testing::DoDefault;
using ::testing::Return;
using ::testing::Throw;

using sdbusplus::xyz::openbmc_project::Common::Device::Error::ReadFailure;

static auto PSUInventoryPath =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0";
static auto PSUGPIOLineName = "presence-ps0";
static auto ChassisName = "chassis0";

// MFR_POUT_MAX value that makes the PSU skip creating the peak input power
// sensor (IBM_CFFPS_1400W has problems with input_history), so tests do not
// create D-Bus sensor objects.
static auto NoSensorMaxPower = "30725";

class PowerSupplyTests : public ::testing::Test
{
  public:
    PowerSupplyTests() :
        bus(sdbusplus::bus::new_default()),
        mockedUtil(static_cast<const MockedUtil&>(getUtils()))
    {
        ON_CALL(mockedUtil, getPresence(_, _)).WillByDefault(Return(false));
    }

    ~PowerSupplyTests() override
    {
        freeUtils();
    }

    /** @brief Creates a PSU that uses the (mocked) presence GPIO. */
    std::unique_ptr<PowerSupply> makePSU(bool multiChassis = false)
    {
        return std::make_unique<PowerSupply>(
            bus, PSUInventoryPath, 3, 0x68, "ibm-cffps", PSUGPIOLineName,
            [this]() { return powerOn; }, ChassisName, multiChassis);
    }

    static MockedGPIOInterface& gpio(PowerSupply& psu)
    {
        return *static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    }

    static MockedPMBus& pmbus(PowerSupply& psu)
    {
        return static_cast<MockedPMBus&>(psu.getPMBus());
    }

    /**
     * @brief Runs one analyze() poll with the given PMBus register values.
     *
     * @param[in] statusWord - Value returned for STATUS_WORD
     * @param[in] vin - READ_VIN value in millivolts. A value below 20 V is
     *                  needed for a VIN_UV fault to stay latched, because
     *                  analyze() clears VIN_UV when VIN is back in range.
     * @param[in] statusInput - Value returned for STATUS_INPUT
     * @param[in] statusMFR - Value returned for STATUS_MFR
     */
    static void poll(PowerSupply& psu, uint16_t statusWord,
                     const std::string& vin = "206000", uint8_t statusInput = 0,
                     uint8_t statusMFR = 0)
    {
        auto& pm = pmbus(psu);
        ON_CALL(pm, read(STATUS_WORD, _, _)).WillByDefault(Return(statusWord));
        ON_CALL(pm, read(STATUS_INPUT, _, _))
            .WillByDefault(Return(statusInput));
        ON_CALL(pm, read(STATUS_MFR, _, _)).WillByDefault(Return(statusMFR));
        ON_CALL(pm, readString(READ_VIN, _)).WillByDefault(Return(vin));
        psu.analyze();
    }

    /**
     * @brief Makes the PSU go from missing to present and runs one clean poll.
     *
     * The first poll after the PSU appears reads VIN for the first time, which
     * clears VIN_UV, so tests that check fault latching do this first.
     */
    void makePresent(PowerSupply& psu)
    {
        ON_CALL(gpio(psu), read()).WillByDefault(Return(1));
        ON_CALL(pmbus(psu), readString(MFR_POUT_MAX, _))
            .WillByDefault(Return(NoSensorMaxPower));
        // Note: going from missing to present waits 1 second before binding
        // the device driver.
        poll(psu, 0x0000);
        ASSERT_TRUE(psu.isPresent());
        ASSERT_FALSE(psu.isFaulted());
    }

    sdbusplus::bus_t bus;
    const MockedUtil& mockedUtil;
    bool powerOn{true};
};

TEST_F(PowerSupplyTests, Constructor)
{
    // Empty inventory path
    try
    {
        PowerSupply psu(
            bus, "", 3, 0x68, "ibm-cffps", PSUGPIOLineName,
            []() { return true; }, ChassisName, false);
        ADD_FAILURE() << "Should have thrown for empty inventory path";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid empty inventoryPath");
    }

    // Empty GPIO line name
    try
    {
        PowerSupply psu(
            bus, PSUInventoryPath, 3, 0x68, "ibm-cffps", "",
            []() { return true; }, ChassisName, false);
        ADD_FAILURE() << "Should have thrown for empty gpioLineName";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid empty gpioLineName");
    }

    // Valid arguments, single and multi chassis. Nothing present, no faults.
    for (bool multiChassis : {false, true})
    {
        auto psu = makePSU(multiChassis);
        EXPECT_FALSE(psu->isPresent());
        EXPECT_FALSE(psu->isFaulted());
        EXPECT_FALSE(psu->hasCommFault());
        EXPECT_FALSE(psu->hasACFault());
        EXPECT_FALSE(psu->hasInputFault());
        EXPECT_FALSE(psu->hasVINUVFault());
        EXPECT_FALSE(psu->hasFanFault());
        EXPECT_FALSE(psu->hasTempFault());
        EXPECT_FALSE(psu->hasPgoodFault());
        EXPECT_FALSE(psu->hasMFRFault());
    }
}

TEST_F(PowerSupplyTests, ShortNameIsLastPathNode)
{
    auto psu = makePSU();
    EXPECT_EQ(psu->getShortName(), "powersupply0");
}

TEST_F(PowerSupplyTests, AnalyzeNotPresent)
{
    auto psu = makePSU();
    EXPECT_CALL(gpio(*psu), read()).WillOnce(Return(0));
    // Not present: STATUS_WORD must not be read.
    EXPECT_CALL(pmbus(*psu), read(STATUS_WORD, _, _)).Times(0);

    psu->analyze();

    EXPECT_FALSE(psu->isPresent());
    EXPECT_FALSE(psu->isFaulted());
}

TEST_F(PowerSupplyTests, PresenceChange)
{
    auto psu = makePSU();
    auto& pm = pmbus(*psu);
    ON_CALL(pm, readString(MFR_POUT_MAX, _))
        .WillByDefault(Return(NoSensorMaxPower));

    // Set all expectations before calling production code.
    // Missing -> present: GPIO reads 1, hwmon dir lookup, ON_OFF_CONFIG write,
    // inventory update (present=true), and history sync requested.
    // Present -> missing: GPIO reads 0, inventory update (present=false).
    EXPECT_CALL(gpio(*psu), read()).WillOnce(Return(1)).WillOnce(Return(0));
    EXPECT_CALL(pm, findHwmonDir());
    EXPECT_CALL(pm, writeBinary(ON_OFF_CONFIG, _, _));
    EXPECT_CALL(mockedUtil, setPresence(_, _, true, _));
    EXPECT_CALL(mockedUtil, setPresence(_, _, false, _));

    poll(*psu, 0x0000);
    EXPECT_TRUE(psu->isPresent());
    EXPECT_TRUE(psu->isSyncHistoryRequired());

    psu->clearSyncHistoryRequired();
    EXPECT_FALSE(psu->isSyncHistoryRequired());

    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->isPresent());
    EXPECT_FALSE(psu->isSyncHistoryRequired());
}

TEST_F(PowerSupplyTests, InputFaultDeglitch)
{
    auto psu = makePSU();
    makePresent(*psu);

    // The fault only counts once it has been seen DEGLITCH_LIMIT times.
    for (auto x = 1; x <= DEGLITCH_LIMIT; x++)
    {
        poll(*psu, status_word::INPUT_FAULT_WARN, "206000", 0x04);
        EXPECT_EQ(psu->hasInputFault(), x >= DEGLITCH_LIMIT);
        EXPECT_EQ(psu->isFaulted(), x >= DEGLITCH_LIMIT);
        EXPECT_FALSE(psu->hasVINUVFault());
    }

    // Fault going away clears it.
    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->hasInputFault());
    EXPECT_FALSE(psu->isFaulted());
}

TEST_F(PowerSupplyTests, ShortInputFaultIsDeglitched)
{
    auto psu = makePSU();
    makePresent(*psu);

    // One fewer poll than the limit, then clean: never reported as a fault.
    for (auto x = 1; x < DEGLITCH_LIMIT; x++)
    {
        poll(*psu, status_word::INPUT_FAULT_WARN, "206000", 0x04);
        EXPECT_FALSE(psu->hasInputFault());
    }
    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->hasInputFault());
    EXPECT_FALSE(psu->isFaulted());
}

TEST_F(PowerSupplyTests, VinUVFault)
{
    auto psu = makePSU();
    makePresent(*psu);

    // Low VIN keeps VIN_UV latched, otherwise analyze() clears it again.
    for (auto x = 1; x <= DEGLITCH_LIMIT; x++)
    {
        poll(*psu, status_word::INPUT_FAULT_WARN | status_word::VIN_UV_FAULT,
             "19123", 0x18);
        EXPECT_EQ(psu->hasVINUVFault(), x >= DEGLITCH_LIMIT);
        EXPECT_EQ(psu->hasInputFault(), x >= DEGLITCH_LIMIT);
        EXPECT_EQ(psu->isFaulted(), x >= DEGLITCH_LIMIT);
        EXPECT_TRUE(psu->hasACFault());
    }

    // Input comes back: faults clear.
    poll(*psu, 0x0000, "209000");
    EXPECT_FALSE(psu->hasVINUVFault());
    EXPECT_FALSE(psu->hasInputFault());
    EXPECT_FALSE(psu->isFaulted());
}

TEST_F(PowerSupplyTests, QuickAcFaultIsLatched)
{
    auto psu = makePSU();
    makePresent(*psu);
    EXPECT_FALSE(psu->hasACFault());

    // A single poll sees VIN_UV: too short to be an input fault, but the AC
    // loss must be remembered.
    poll(*psu, status_word::VIN_UV_FAULT, "19123");
    EXPECT_FALSE(psu->hasVINUVFault());
    EXPECT_FALSE(psu->isFaulted());
    EXPECT_TRUE(psu->hasACFault());

    // The AC fault stays latched for AC_FAULT_LIMIT clean polls...
    for (auto x = 1; x < AC_FAULT_LIMIT; x++)
    {
        poll(*psu, 0x0000);
        EXPECT_TRUE(psu->hasACFault()) << "clean poll " << x;
    }

    // ...then expires.
    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->hasACFault());
}

TEST_F(PowerSupplyTests, AcFaultRelatchedByNewFault)
{
    auto psu = makePSU();
    makePresent(*psu);

    poll(*psu, status_word::VIN_UV_FAULT, "19123");
    poll(*psu, 0x0000);
    poll(*psu, 0x0000);
    EXPECT_TRUE(psu->hasACFault());

    // Another AC loss restarts the window.
    poll(*psu, status_word::VIN_UV_FAULT, "19123");
    for (auto x = 1; x < AC_FAULT_LIMIT; x++)
    {
        poll(*psu, 0x0000);
        EXPECT_TRUE(psu->hasACFault()) << "clean poll " << x;
    }
    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->hasACFault());
}

TEST_F(PowerSupplyTests, FanFault)
{
    auto psu = makePSU();
    makePresent(*psu);

    for (auto x = 1; x <= DEGLITCH_LIMIT; x++)
    {
        poll(*psu, status_word::FAN_FAULT);
        EXPECT_EQ(psu->hasFanFault(), x >= DEGLITCH_LIMIT);
        EXPECT_EQ(psu->isFaulted(), x >= DEGLITCH_LIMIT);
        EXPECT_FALSE(psu->hasInputFault());
    }

    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->hasFanFault());
    EXPECT_FALSE(psu->isFaulted());
}

TEST_F(PowerSupplyTests, PgoodFault)
{
    auto psu = makePSU();
    makePresent(*psu);

    // PGOOD uses a longer deglitch limit than other faults.
    for (auto x = 1; x <= PGOOD_DEGLITCH_LIMIT; x++)
    {
        poll(*psu, status_word::POWER_GOOD_NEGATED);
        EXPECT_EQ(psu->hasPgoodFault(), x >= PGOOD_DEGLITCH_LIMIT);
        EXPECT_EQ(psu->isFaulted(), x >= PGOOD_DEGLITCH_LIMIT);
    }

    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->hasPgoodFault());
}

TEST_F(PowerSupplyTests, UnitOffIsPgoodFault)
{
    auto psu = makePSU();
    makePresent(*psu);

    for (auto x = 1; x <= PGOOD_DEGLITCH_LIMIT; x++)
    {
        poll(*psu, status_word::UNIT_IS_OFF);
    }
    EXPECT_TRUE(psu->hasPgoodFault());
}

// Note: reaching LOG_LIMIT failed reads commits a ReadFailure error log, which
// needs the logging service. The failure count is updated before the commit, so
// an exception from the commit (no logging service on the build machine) is
// ignored here; the comm fault state is what is being checked.
TEST_F(PowerSupplyTests, CommFault)
{
    auto psu = makePSU();
    makePresent(*psu);
    auto& pm = pmbus(*psu);

    ON_CALL(pm, read(STATUS_WORD, _, _)).WillByDefault(Throw(ReadFailure()));

    for (auto x = 1; x <= LOG_LIMIT; x++)
    {
        try
        {
            psu->analyze();
        }
        catch (const std::exception&)
        {}
        EXPECT_EQ(psu->hasCommFault(), x >= LOG_LIMIT);
        EXPECT_EQ(psu->isFaulted(), x >= LOG_LIMIT);
    }

    // A good read resets the failure count.
    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->hasCommFault());
    EXPECT_FALSE(psu->isFaulted());
}

TEST_F(PowerSupplyTests, ShortCommFailureIsNotFault)
{
    auto psu = makePSU();
    makePresent(*psu);
    auto& pm = pmbus(*psu);

    ON_CALL(pm, read(STATUS_WORD, _, _)).WillByDefault(Throw(ReadFailure()));
    for (auto x = 1; x < LOG_LIMIT; x++)
    {
        psu->analyze();
        EXPECT_FALSE(psu->hasCommFault());
    }

    poll(*psu, 0x0000);
    EXPECT_FALSE(psu->hasCommFault());
}

TEST_F(PowerSupplyTests, AvailabilityFollowsInputFault)
{
    auto psu = makePSU();

    // Set all expectations before calling production code.
    // Sequence of availability updates:
    // 1. makePresent(): PSU becomes present -> available=true
    // 2. Fault reported after deglitch limit -> available=false
    // 3. Fault clears -> available=true
    ::testing::InSequence seq;
    EXPECT_CALL(mockedUtil, setAvailable(_, _, true)).Times(1);
    EXPECT_CALL(mockedUtil, handleChassisHealthRollup(_, _, false)).Times(1);
    EXPECT_CALL(mockedUtil, setAvailable(_, _, false)).Times(1);
    EXPECT_CALL(mockedUtil, handleChassisHealthRollup(_, _, true)).Times(1);
    EXPECT_CALL(mockedUtil, setAvailable(_, _, true)).Times(1);
    EXPECT_CALL(mockedUtil, handleChassisHealthRollup(_, _, false)).Times(1);

    makePresent(*psu);

    for (auto x = 1; x <= DEGLITCH_LIMIT + 1; x++)
    {
        poll(*psu, status_word::INPUT_FAULT_WARN, "206000", 0x04);
    }

    poll(*psu, 0x0000);
    poll(*psu, 0x0000);
}

TEST_F(PowerSupplyTests, ClearFaults)
{
    auto psu = makePSU();
    auto& pm = pmbus(*psu);

    // The hwmon driver sends CLEAR_FAULTS when in1_input is read, and
    // in1_lcrit_alarm clears VIN_UV/OFF.
    // During makePresent():
    //   1. updatePresenceGPIO() calls clearFaults() -> in1_lcrit_alarm,
    //      in1_input
    //   2. analyze() sees input voltage back in range -> in1_lcrit_alarm
    // During psu->clearFaults():
    //   3. clearFaults() -> in1_lcrit_alarm, in1_input
    EXPECT_CALL(pm, read(_, _, _)).WillRepeatedly(DoDefault());
    EXPECT_CALL(pm, read(READ_VIN, _, _)).Times(2);
    EXPECT_CALL(pm, read("in1_lcrit_alarm", _, _)).Times(3);

    makePresent(*psu);

    for (auto x = 1; x <= DEGLITCH_LIMIT; x++)
    {
        poll(*psu, status_word::FAN_FAULT);
    }
    ASSERT_TRUE(psu->hasFanFault());

    psu->clearFaults();

    EXPECT_FALSE(psu->hasFanFault());
    EXPECT_FALSE(psu->isFaulted());
    EXPECT_FALSE(psu->isFaultLogged());
}

TEST_F(PowerSupplyTests, ClearFaultsNotPresent)
{
    auto psu = makePSU();
    ON_CALL(gpio(*psu), read()).WillByDefault(Return(0));

    // Nothing to clear when the PSU is missing.
    auto& pm = pmbus(*psu);
    EXPECT_CALL(pm, read(_, _, _)).WillRepeatedly(DoDefault());
    EXPECT_CALL(pm, read(READ_VIN, _, _)).Times(0);
    EXPECT_CALL(pm, read("in1_lcrit_alarm", _, _)).Times(0);
    psu->clearFaults();
}

// linearToInteger is a pure function of the PMBus LINEAR11 format: a 5-bit
// two's complement exponent in bits 15:11 and an 11-bit two's complement
// mantissa in bits 10:0.
TEST(LinearToInteger, Values)
{
    EXPECT_DOUBLE_EQ(PowerSupply::linearToInteger(0x0000), 0.0);
    // exponent 0, mantissa 100
    EXPECT_DOUBLE_EQ(PowerSupply::linearToInteger(100), 100.0);
    // exponent +1, mantissa 10 -> 20
    EXPECT_DOUBLE_EQ(PowerSupply::linearToInteger((1u << 11) | 10u), 20.0);
    // exponent -1, mantissa 100 -> 50
    EXPECT_DOUBLE_EQ(PowerSupply::linearToInteger((0x1Fu << 11) | 100u), 50.0);
    // exponent 0, mantissa -1
    EXPECT_DOUBLE_EQ(PowerSupply::linearToInteger(0x07FF), -1.0);
}
