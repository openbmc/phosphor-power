#include "../power_supply.hpp"
#include "mock.hpp"

#include <xyz/openbmc_project/Common/Device/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::psu;
using namespace phosphor::pmbus;

using ::testing::_;
using ::testing::Args;
using ::testing::Assign;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

static auto PSUInventoryPath = "/xyz/bmc/inv/sys/chassis/board/powersupply0";
static auto PSUGPIOLineName = "presence-ps0";

struct PMBusExpectations
{
    uint16_t statusWordValue{0x0000};
    uint8_t statusInputValue{0x00};
    uint8_t statusMFRValue{0x00};
    uint8_t statusCMLValue{0x00};
    uint8_t statusVOUTValue{0x00};
    uint8_t statusIOUTValue{0x00};
    uint8_t statusFans12Value{0x00};
    uint8_t statusTempValue{0x00};
};

// Helper function to setup expectations for various STATUS_* commands
void setPMBusExpectations(MockedPMBus& mockPMBus,
                          const PMBusExpectations& expectations)
{
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(expectations.statusWordValue));

    if (expectations.statusWordValue != 0)
    {
        // If fault bits are on in STATUS_WORD, there will also be a read of
        // STATUS_INPUT, STATUS_MFR, STATUS_CML, STATUS_VOUT (page 0), and
        // STATUS_TEMPERATURE.
        EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
            .Times(1)
            .WillOnce(Return(expectations.statusInputValue));
        EXPECT_CALL(mockPMBus, read(STATUS_MFR, _))
            .Times(1)
            .WillOnce(Return(expectations.statusMFRValue));
        EXPECT_CALL(mockPMBus, read(STATUS_CML, _))
            .Times(1)
            .WillOnce(Return(expectations.statusCMLValue));
        // Page will need to be set to 0 to read STATUS_VOUT.
        EXPECT_CALL(mockPMBus, insertPageNum(STATUS_VOUT, 0))
            .Times(1)
            .WillOnce(Return("status0_vout"));
        EXPECT_CALL(mockPMBus, read("status0_vout", _))
            .Times(1)
            .WillOnce(Return(expectations.statusVOUTValue));
        EXPECT_CALL(mockPMBus, read(STATUS_IOUT, _))
            .Times(1)
            .WillOnce(Return(expectations.statusIOUTValue));
        EXPECT_CALL(mockPMBus, read(STATUS_FANS_1_2, _))
            .Times(1)
            .WillOnce(Return(expectations.statusFans12Value));
        EXPECT_CALL(mockPMBus, read(STATUS_TEMPERATURE, _))
            .Times(1)
            .WillOnce(Return(expectations.statusTempValue));
    }
}

class PowerSupplyTests : public ::testing::Test
{
  public:
    PowerSupplyTests() :
        mockedUtil(reinterpret_cast<const MockedUtil&>(getUtils()))
    {
        ON_CALL(mockedUtil, getPresence(_, _)).WillByDefault(Return(false));
    }

    ~PowerSupplyTests() override
    {
        freeUtils();
    }

    const MockedUtil& mockedUtil;
};

// Helper function for when a power supply goes from missing to present.
void setMissingToPresentExpects(MockedPMBus& pmbus, const MockedUtil& util)
{
    // Call to analyze() will update to present, that will trigger updating
    // to the correct/latest HWMON directory, in case it changes.
    EXPECT_CALL(pmbus, findHwmonDir());
    // Presence change from missing to present will trigger write to
    // ON_OFF_CONFIG.
    EXPECT_CALL(pmbus, writeBinary(ON_OFF_CONFIG, _, _));
    // Presence change from missing to present will trigger in1_input read
    // in an attempt to get CLEAR_FAULTS called.
    // The voltage defaults to 0, the first call to analyze should update the
    // voltage to the current reading, triggering another clearing of faults
    // due to below minimum to within range voltage.
    // This READ_VIN for CLEAR_FAULTS does not check the returned value.
    EXPECT_CALL(pmbus, read(READ_VIN, _))
        .Times(2)
        .WillOnce(Return(1))
        .WillOnce(Return(2));
    // Missing/present call will update Presence in inventory.
    EXPECT_CALL(util, setPresence(_, _, true, _));
}

TEST_F(PowerSupplyTests, Constructor)
{
    /**
     * @param[in] invpath - String for inventory path to use
     * @param[in] i2cbus - The bus number this power supply is on
     * @param[in] i2caddr - The 16-bit I2C address of the power supply
     * @param[in] gpioLineName - The string for the gpio-line-name to read for
     * presence.
     * @param[in] bindDelay - Time in milliseconds to delay binding the device
     * driver after seeing the presence line go active.
     */
    auto bus = sdbusplus::bus::new_default();

    // Try where inventory path is empty, constructor should fail.
    try
    {
        auto psu =
            std::make_unique<PowerSupply>(bus, "", 3, 0x68, PSUGPIOLineName);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid empty inventoryPath");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // TODO: Try invalid i2c address?

    // Try where gpioLineName is empty.
    try
    {
        auto psu =
            std::make_unique<PowerSupply>(bus, PSUInventoryPath, 3, 0x68, "");
        ADD_FAILURE()
            << "Should not have reached this line. Invalid gpioLineName.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid empty gpioLineName");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test with valid arguments
    // NOT using D-Bus inventory path for presence.
    try
    {
        auto psu = std::make_unique<PowerSupply>(bus, PSUInventoryPath, 3, 0x68,
                                                 PSUGPIOLineName);

        EXPECT_EQ(psu->isPresent(), false);
        EXPECT_EQ(psu->isFaulted(), false);
        EXPECT_EQ(psu->hasCommFault(), false);
        EXPECT_EQ(psu->hasInputFault(), false);
        EXPECT_EQ(psu->hasMFRFault(), false);
        EXPECT_EQ(psu->hasVINUVFault(), false);
        EXPECT_EQ(psu->hasVoutOVFault(), false);
        EXPECT_EQ(psu->hasIoutOCFault(), false);
        EXPECT_EQ(psu->hasVoutUVFault(), false);
        EXPECT_EQ(psu->hasFanFault(), false);
        EXPECT_EQ(psu->hasTempFault(), false);
        EXPECT_EQ(psu->hasPgoodFault(), false);
        EXPECT_EQ(psu->hasPSKillFault(), false);
        EXPECT_EQ(psu->hasPS12VcsFault(), false);
        EXPECT_EQ(psu->hasPSCS12VFault(), false);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test with valid arguments
    // TODO: Using D-Bus inventory path for presence.
    try
    {
        // FIXME: How do I get that presenceGPIO.read() in the startup to throw
        // an exception?

        // EXPECT_CALL(mockedUtil, getPresence(_,
        // StrEq(PSUInventoryPath)))
        //    .Times(1);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST_F(PowerSupplyTests, Analyze)
{
    auto bus = sdbusplus::bus::new_default();

    {
        // If I default to reading the GPIO, I will NOT expect a call to
        // getPresence().

        PowerSupply psu{bus, PSUInventoryPath, 4, 0x69, PSUGPIOLineName};
        MockedGPIOInterface* mockPresenceGPIO =
            static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
        EXPECT_CALL(*mockPresenceGPIO, read()).Times(1).WillOnce(Return(0));

        psu.analyze();
        // By default, nothing should change.
        EXPECT_EQ(psu.isPresent(), false);
        EXPECT_EQ(psu.isFaulted(), false);
        EXPECT_EQ(psu.hasInputFault(), false);
        EXPECT_EQ(psu.hasMFRFault(), false);
        EXPECT_EQ(psu.hasVINUVFault(), false);
        EXPECT_EQ(psu.hasCommFault(), false);
        EXPECT_EQ(psu.hasVoutOVFault(), false);
        EXPECT_EQ(psu.hasIoutOCFault(), false);
        EXPECT_EQ(psu.hasVoutUVFault(), false);
        EXPECT_EQ(psu.hasFanFault(), false);
        EXPECT_EQ(psu.hasTempFault(), false);
        EXPECT_EQ(psu.hasPgoodFault(), false);
        EXPECT_EQ(psu.hasPSKillFault(), false);
        EXPECT_EQ(psu.hasPS12VcsFault(), false);
        EXPECT_EQ(psu.hasPSCS12VFault(), false);
    }

    PowerSupply psu2{bus, PSUInventoryPath, 5, 0x6a, PSUGPIOLineName};
    // In order to get the various faults tested, the power supply needs to
    // be present in order to read from the PMBus device(s).
    MockedGPIOInterface* mockPresenceGPIO2 =
        static_cast<MockedGPIOInterface*>(psu2.getPresenceGPIO());
    // Always return 1 to indicate present.
    // Each analyze() call will trigger a read of the presence GPIO.
    EXPECT_CALL(*mockPresenceGPIO2, read()).WillRepeatedly(Return(1));
    EXPECT_EQ(psu2.isPresent(), false);

    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu2.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);

    // STATUS_WORD INPUT fault.
    {
        // Start with STATUS_WORD 0x0000. Powered on, no faults.
        // Set expectations for a no fault
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        // After reading STATUS_WORD, etc., there will be a READ_VIN check.
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("206000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), false);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);

        // Update expectations for STATUS_WORD input fault/warn
        // STATUS_INPUT fault bits ... on.
        expectations.statusWordValue = (status_word::INPUT_FAULT_WARN);
        expectations.statusInputValue = 0x38;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("207000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), true);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);
    }

    // STATUS_WORD INPUT/UV fault.
    {
        // First need it to return good status, then the fault
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("208000"));
        psu2.analyze();
        // Now set fault bits in STATUS_WORD
        expectations.statusWordValue =
            (status_word::INPUT_FAULT_WARN | status_word::VIN_UV_FAULT);
        // STATUS_INPUT fault bits ... on.
        expectations.statusInputValue = 0x38;
        setPMBusExpectations(mockPMBus, expectations);
        // Input/UV fault, so voltage should read back low.
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("19123"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), true);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), true);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);

        // Turning VIN_UV fault off causes clearing of faults, causing read of
        // in1_input as an attempt to get CLEAR_FAULTS called.
        expectations.statusWordValue = 0;
        setPMBusExpectations(mockPMBus, expectations);
        // The call to read the voltage
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("209000"));
        // The call for CLEAR_FAULTS command
        EXPECT_CALL(mockPMBus, read(READ_VIN, _)).Times(1).WillOnce(Return(3));
        psu2.analyze();
        // Should remain present, no longer be faulted, no input fault, no
        // VIN_UV fault. Nothing else should change.
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), false);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
    }

    // STATUS_WORD MFR fault.
    {
        // First need it to return good status, then the fault
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("210000"));
        psu2.analyze();
        // Now STATUS_WORD with MFR fault bit on.
        expectations.statusWordValue = (status_word::MFR_SPECIFIC_FAULT);
        // STATUS_MFR bits on.
        expectations.statusMFRValue = 0xFF;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("211000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasMFRFault(), true);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), true);
        EXPECT_EQ(psu2.hasPS12VcsFault(), true);
        EXPECT_EQ(psu2.hasPSCS12VFault(), true);
    }

    // Temperature fault.
    {
        // First STATUS_WORD with no bits set, then with temperature fault.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("212000"));
        psu2.analyze();
        // STATUS_WORD with temperature fault bit on.
        expectations.statusWordValue = (status_word::TEMPERATURE_FAULT_WARN);
        // STATUS_TEMPERATURE with fault bit(s) on.
        expectations.statusTempValue = 0x10;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("213000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), true);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);
    }

    // CML fault
    {
        // First STATUS_WORD wit no bits set, then with CML fault.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("214000"));
        psu2.analyze();
        // STATUS_WORD with CML fault bit on.
        expectations.statusWordValue = (status_word::CML_FAULT);
        // Turn on STATUS_CML fault bit(s)
        expectations.statusCMLValue = 0xFF;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("215000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), true);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);
    }

    // VOUT_OV_FAULT fault
    {
        // First STATUS_WORD with no bits set, then with VOUT/VOUT_OV fault.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("216000"));
        psu2.analyze();
        // STATUS_WORD with VOUT/VOUT_OV fault.
        expectations.statusWordValue =
            ((status_word::VOUT_FAULT) | (status_word::VOUT_OV_FAULT));
        // Turn on STATUS_VOUT fault bit(s)
        expectations.statusVOUTValue = 0xA0;
        // STATUS_TEMPERATURE don't care (default)
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("217000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), true);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);
    }

    // IOUT_OC_FAULT fault
    {
        // First STATUS_WORD with no bits set, then with IOUT_OC fault.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("218000"));
        psu2.analyze();
        // STATUS_WORD with IOUT_OC fault.
        expectations.statusWordValue = status_word::IOUT_OC_FAULT;
        // Turn on STATUS_IOUT fault bit(s)
        expectations.statusIOUTValue = 0x88;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("219000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), true);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);
    }

    // VOUT_UV_FAULT
    {
        // First STATUS_WORD with no bits set, then with VOUT fault.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("220000"));
        psu2.analyze();
        // Change STATUS_WORD to indicate VOUT fault.
        expectations.statusWordValue = (status_word::VOUT_FAULT);
        // Turn on STATUS_VOUT fault bit(s)
        expectations.statusVOUTValue = 0x30;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("221000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasVoutUVFault(), true);
        EXPECT_EQ(psu2.hasFanFault(), false);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);
    }

    // Fan fault
    {
        // First STATUS_WORD with no bits set, then with fan fault.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("222000"));
        psu2.analyze();
        expectations.statusWordValue = (status_word::FAN_FAULT);
        // STATUS_FANS_1_2 with fan 1 warning & fault bits on.
        expectations.statusFans12Value = 0xA0;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("223000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isPresent(), true);
        EXPECT_EQ(psu2.isFaulted(), true);
        EXPECT_EQ(psu2.hasInputFault(), false);
        EXPECT_EQ(psu2.hasMFRFault(), false);
        EXPECT_EQ(psu2.hasVINUVFault(), false);
        EXPECT_EQ(psu2.hasCommFault(), false);
        EXPECT_EQ(psu2.hasVoutOVFault(), false);
        EXPECT_EQ(psu2.hasIoutOCFault(), false);
        EXPECT_EQ(psu2.hasVoutUVFault(), false);
        EXPECT_EQ(psu2.hasFanFault(), true);
        EXPECT_EQ(psu2.hasTempFault(), false);
        EXPECT_EQ(psu2.hasPgoodFault(), false);
        EXPECT_EQ(psu2.hasPSKillFault(), false);
        EXPECT_EQ(psu2.hasPS12VcsFault(), false);
        EXPECT_EQ(psu2.hasPSCS12VFault(), false);
    }

    // PGOOD/OFF fault. Deglitched, needs to reach DEGLITCH_LIMIT.
    {
        // First STATUS_WORD with no bits set.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("123000"));
        psu2.analyze();
        EXPECT_EQ(psu2.isFaulted(), false);
        // POWER_GOOD# inactive, and OFF bit on.
        expectations.statusWordValue =
            ((status_word::POWER_GOOD_NEGATED) | (status_word::UNIT_IS_OFF));
        for (auto x = 1; x <= DEGLITCH_LIMIT; x++)
        {
            // STATUS_INPUT, STATUS_MFR, STATUS_CML, STATUS_VOUT, and
            // STATUS_TEMPERATURE: Don't care if bits set or not (defaults).
            setPMBusExpectations(mockPMBus, expectations);
            EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
                .Times(1)
                .WillOnce(Return("124000"));
            psu2.analyze();
            EXPECT_EQ(psu2.isPresent(), true);
            if (x < DEGLITCH_LIMIT)
            {
                EXPECT_EQ(psu2.isFaulted(), false);
            }
            else
            {
                EXPECT_EQ(psu2.isFaulted(), true);
            }
            EXPECT_EQ(psu2.hasInputFault(), false);
            EXPECT_EQ(psu2.hasMFRFault(), false);
            EXPECT_EQ(psu2.hasVINUVFault(), false);
            EXPECT_EQ(psu2.hasCommFault(), false);
            EXPECT_EQ(psu2.hasVoutOVFault(), false);
            EXPECT_EQ(psu2.hasVoutUVFault(), false);
            EXPECT_EQ(psu2.hasIoutOCFault(), false);
            EXPECT_EQ(psu2.hasFanFault(), false);
            EXPECT_EQ(psu2.hasTempFault(), false);
            if (x < DEGLITCH_LIMIT)
            {
                EXPECT_EQ(psu2.hasPgoodFault(), false);
            }
            else
            {
                EXPECT_EQ(psu2.hasPgoodFault(), true);
            }
        }
    }

    // TODO: ReadFailure
}

TEST_F(PowerSupplyTests, OnOffConfig)
{
    auto bus = sdbusplus::bus::new_default();
    uint8_t data = 0x15;

    // Test where PSU is NOT present
    try
    {
        // Assume GPIO presence, not inventory presence?
        PowerSupply psu{bus, PSUInventoryPath, 4, 0x69, PSUGPIOLineName};

        MockedGPIOInterface* mockPresenceGPIO =
            static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
        ON_CALL(*mockPresenceGPIO, read()).WillByDefault(Return(0));
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        // Constructor should set initial presence, default read returns 0.
        // If it is not present, I should not be trying to write to it.
        EXPECT_CALL(mockPMBus, writeBinary(_, _, _)).Times(0);
        psu.onOffConfig(data);
    }
    catch (...)
    {}

    // Test where PSU is present
    try
    {
        // Assume GPIO presence, not inventory presence?
        PowerSupply psu{bus, PSUInventoryPath, 5, 0x6a, PSUGPIOLineName};
        MockedGPIOInterface* mockPresenceGPIO =
            static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
        // There will potentially be multiple calls, we want it to continue
        // returning 1 for the GPIO read to keep the power supply present.
        EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        setMissingToPresentExpects(mockPMBus, mockedUtil);
        // If I am calling analyze(), I should probably give it good data.
        // STATUS_WORD 0x0000 is powered on, no faults.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("205000"));
        psu.analyze();
        // I definitely should be writting ON_OFF_CONFIG if I call the function
        EXPECT_CALL(mockPMBus, writeBinary(ON_OFF_CONFIG, ElementsAre(0x15),
                                           Type::HwmonDeviceDebug))
            .Times(1);
        psu.onOffConfig(data);
    }
    catch (...)
    {}
}

TEST_F(PowerSupplyTests, ClearFaults)
{
    auto bus = sdbusplus::bus::new_default();
    PowerSupply psu{bus, PSUInventoryPath, 13, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    // Each analyze() call will trigger a read of the presence GPIO.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("207000"));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);
    EXPECT_EQ(psu.hasCommFault(), false);
    EXPECT_EQ(psu.hasVoutOVFault(), false);
    EXPECT_EQ(psu.hasIoutOCFault(), false);
    EXPECT_EQ(psu.hasVoutUVFault(), false);
    EXPECT_EQ(psu.hasFanFault(), false);
    EXPECT_EQ(psu.hasTempFault(), false);
    EXPECT_EQ(psu.hasPgoodFault(), false);
    EXPECT_EQ(psu.hasPSKillFault(), false);
    EXPECT_EQ(psu.hasPS12VcsFault(), false);
    EXPECT_EQ(psu.hasPSCS12VFault(), false);

    // STATUS_WORD with fault bits galore!
    expectations.statusWordValue = 0xFFFF;
    // STATUS_INPUT with fault bits on.
    expectations.statusInputValue = 0xFF;
    // STATUS_MFR_SPEFIC with bits on.
    expectations.statusMFRValue = 0xFF;
    // STATUS_CML with bits on.
    expectations.statusCMLValue = 0xFF;
    // STATUS_VOUT with bits on.
    expectations.statusVOUTValue = 0xFF;
    // STATUS_IOUT with bits on.
    expectations.statusIOUTValue = 0xFF;
    // STATUS_FANS_1_2 with bits on.
    expectations.statusFans12Value = 0xFF;
    // STATUS_TEMPERATURE with bits on.
    expectations.statusTempValue = 0xFF;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("0"));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), true);
    EXPECT_EQ(psu.hasInputFault(), true);
    EXPECT_EQ(psu.hasMFRFault(), true);
    EXPECT_EQ(psu.hasVINUVFault(), true);
    EXPECT_EQ(psu.hasCommFault(), true);
    EXPECT_EQ(psu.hasVoutOVFault(), true);
    EXPECT_EQ(psu.hasIoutOCFault(), true);
    // Cannot have VOUT_OV_FAULT and VOUT_UV_FAULT.
    // Rely on HasVoutUVFault() to verify this sets and clears.
    EXPECT_EQ(psu.hasVoutUVFault(), false);
    EXPECT_EQ(psu.hasFanFault(), true);
    EXPECT_EQ(psu.hasTempFault(), true);
    // pgoodFault is deglitched up to DEGLITCH_LIMIT
    EXPECT_EQ(psu.hasPgoodFault(), false);
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("0"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), false);
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("0"));
    psu.analyze();
    // DEGLITCH_LIMIT reached for pgoodFault
    EXPECT_EQ(psu.hasPgoodFault(), true);
    EXPECT_EQ(psu.hasPSKillFault(), true);
    EXPECT_EQ(psu.hasPS12VcsFault(), true);
    EXPECT_EQ(psu.hasPSCS12VFault(), true);
    // This is the CLEAR_FAULTS read that does not check the return value.
    EXPECT_CALL(mockPMBus, read(READ_VIN, _)).Times(1).WillOnce(Return(3));
    psu.clearFaults();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);
    EXPECT_EQ(psu.hasCommFault(), false);
    EXPECT_EQ(psu.hasVoutOVFault(), false);
    EXPECT_EQ(psu.hasIoutOCFault(), false);
    EXPECT_EQ(psu.hasVoutUVFault(), false);
    EXPECT_EQ(psu.hasFanFault(), false);
    EXPECT_EQ(psu.hasTempFault(), false);
    EXPECT_EQ(psu.hasPgoodFault(), false);
    EXPECT_EQ(psu.hasPSKillFault(), false);
    EXPECT_EQ(psu.hasPS12VcsFault(), false);
    EXPECT_EQ(psu.hasPSCS12VFault(), false);

    // Faults clear on READ_VIN 0 -> !0
    // STATUS_WORD with fault bits galore!
    expectations.statusWordValue = 0xFFFF;
    // STATUS_INPUT with fault bits on.
    expectations.statusInputValue = 0xFF;
    // STATUS_MFR_SPEFIC with bits on.
    expectations.statusMFRValue = 0xFF;
    // STATUS_CML with bits on.
    expectations.statusCMLValue = 0xFF;
    // STATUS_VOUT with bits on.
    expectations.statusVOUTValue = 0xFF;
    // STATUS_IOUT with bits on.
    expectations.statusIOUTValue = 0xFF;
    // STATUS_FANS_1_2 with bits on.
    expectations.statusFans12Value = 0xFF;
    // STATUS_TEMPERATURE with bits on.
    expectations.statusTempValue = 0xFF;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, read(READ_VIN, _)).Times(1).WillOnce(Return(0));
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("0"));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), true);
    EXPECT_EQ(psu.hasInputFault(), true);
    EXPECT_EQ(psu.hasMFRFault(), true);
    EXPECT_EQ(psu.hasVINUVFault(), true);
    // True due to CML fault bits on
    EXPECT_EQ(psu.hasCommFault(), true);
    EXPECT_EQ(psu.hasVoutOVFault(), true);
    EXPECT_EQ(psu.hasIoutOCFault(), true);
    // Cannot have VOUT_OV_FAULT and VOUT_UV_FAULT.
    // Rely on HasVoutUVFault() to verify this sets and clears.
    EXPECT_EQ(psu.hasVoutUVFault(), false);
    EXPECT_EQ(psu.hasFanFault(), true);
    EXPECT_EQ(psu.hasTempFault(), true);
    // PGOOD fault is deglitched before hasPgoodFault() returns true.
    EXPECT_EQ(psu.hasPgoodFault(), false);
    EXPECT_EQ(psu.hasPSKillFault(), true);
    EXPECT_EQ(psu.hasPS12VcsFault(), true);
    EXPECT_EQ(psu.hasPSCS12VFault(), true);
    // STATUS_WORD with INPUT/VIN_UV fault bits off.
    expectations.statusWordValue = 0xDFF7;
    // STATUS_INPUT with VIN_UV_WARNING, VIN_UV_FAULT, and Unit Off For
    // Insufficient Input Voltage bits off.
    expectations.statusInputValue = 0xC7;
    setPMBusExpectations(mockPMBus, expectations);
    // READ_VIN back in range.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("206000"));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);
    EXPECT_EQ(psu.hasCommFault(), false);
    EXPECT_EQ(psu.hasVoutOVFault(), false);
    EXPECT_EQ(psu.hasIoutOCFault(), false);
    EXPECT_EQ(psu.hasVoutUVFault(), false);
    EXPECT_EQ(psu.hasFanFault(), false);
    EXPECT_EQ(psu.hasTempFault(), false);
    EXPECT_EQ(psu.hasPgoodFault(), false);
    EXPECT_EQ(psu.hasPSKillFault(), false);
    EXPECT_EQ(psu.hasPS12VcsFault(), false);
    EXPECT_EQ(psu.hasPSCS12VFault(), false);

    // TODO: Faults clear on missing/present?
}

TEST_F(PowerSupplyTests, UpdateInventory)
{
    auto bus = sdbusplus::bus::new_default();

    try
    {
        PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        // If it is not present, I should not be trying to read a string
        EXPECT_CALL(mockPMBus, readString(_, _)).Times(0);
        psu.updateInventory();
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    try
    {
        PowerSupply psu{bus, PSUInventoryPath, 13, 0x69, PSUGPIOLineName};
        MockedGPIOInterface* mockPresenceGPIO =
            static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
        // GPIO read return 1 to indicate present.
        EXPECT_CALL(*mockPresenceGPIO, read()).Times(1).WillOnce(Return(1));
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        setMissingToPresentExpects(mockPMBus, mockedUtil);
        // STATUS_WORD 0x0000 is powered on, no faults.
        PMBusExpectations expectations;
        setPMBusExpectations(mockPMBus, expectations);
        // Call to analyze will read voltage, trigger clear faults for 0 to
        // within range.
        EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
            .Times(1)
            .WillOnce(Return("123456"));
        psu.analyze();
        EXPECT_CALL(mockPMBus, readString(_, _)).WillRepeatedly(Return(""));
        psu.updateInventory();

#if IBM_VPD
        EXPECT_CALL(mockPMBus, readString(_, _))
            .WillOnce(Return("CCIN"))
            .WillOnce(Return("PN3456"))
            .WillOnce(Return("FN3456"))
            .WillOnce(Return("HEADER"))
            .WillOnce(Return("SN3456"))
            .WillOnce(Return("FW3456"));
#endif
        psu.updateInventory();
        // TODO: D-Bus mocking to verify values stored on D-Bus (???)
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST_F(PowerSupplyTests, IsPresent)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    EXPECT_EQ(psu.isPresent(), false);

    // Change GPIO read to return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).Times(1).WillOnce(Return(1));
    // Call to analyze() will update to present, that will trigger updating
    // to the correct/latest HWMON directory, in case it changes.
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // Call to analyze things will trigger read of STATUS_WORD and READ_VIN.
    // Default expectations will be on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Give it an input voltage in the 100-volt range.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("123456"));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
}

TEST_F(PowerSupplyTests, IsFaulted)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 11, 0x6f, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // Call to analyze things will trigger read of STATUS_WORD and READ_VIN.
    // Default expectations will be on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Give it an input voltage in the 100-volt range.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("124680"));
    psu.analyze();
    EXPECT_EQ(psu.isFaulted(), false);
    // STATUS_WORD with fault bits on.
    expectations.statusWordValue = 0xFFFF;
    // STATUS_INPUT with fault bits on.
    expectations.statusInputValue = 0xFF;
    // STATUS_MFR_SPECIFIC with faults bits on.
    expectations.statusMFRValue = 0xFF;
    // STATUS_CML with faults bits on.
    expectations.statusCMLValue = 0xFF;
    // STATUS_VOUT with fault bits on.
    expectations.statusVOUTValue = 0xFF;
    // STATUS_IOUT with fault bits on.
    expectations.statusIOUTValue = 0xFF;
    // STATUS_FANS_1_2 with bits on.
    expectations.statusFans12Value = 0xFF;
    // STATUS_TEMPERATURE with fault bits on.
    expectations.statusTempValue = 0xFF;
    setPMBusExpectations(mockPMBus, expectations);
    // Also get another read of READ_VIN.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("125790"));
    psu.analyze();
    EXPECT_EQ(psu.isFaulted(), true);
}

TEST_F(PowerSupplyTests, HasInputFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Analyze call will also need good READ_VIN value to check.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("201100"));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), false);
    // STATUS_WORD with input fault/warn on.
    expectations.statusWordValue = (status_word::INPUT_FAULT_WARN);
    // STATUS_INPUT with an input fault bit on.
    expectations.statusInputValue = 0x80;
    setPMBusExpectations(mockPMBus, expectations);
    // Analyze call will also need good READ_VIN value to check.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("201200"));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), true);
    // STATUS_WORD with no bits on.
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    // Analyze call will also need good READ_VIN value to check.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("201300"));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), false);
}

TEST_F(PowerSupplyTests, HasMFRFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // First return STATUS_WORD with no bits on.
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Analyze call will also need good READ_VIN value to check.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("202100"));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), false);
    // Next return STATUS_WORD with MFR fault bit on.
    expectations.statusWordValue = (status_word::MFR_SPECIFIC_FAULT);
    // STATUS_MFR_SPEFIC with bit(s) on.
    expectations.statusMFRValue = 0xFF;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("202200"));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), true);
    // Back to no bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("202300"));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), false);
}

TEST_F(PowerSupplyTests, HasVINUVFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);

    // Presence change from missing to present will trigger in1_input read in
    // an attempt to get CLEAR_FAULTS called. Return value ignored.
    // Zero to non-zero voltage, for missing/present change, triggers clear
    // faults call again. Return value ignored.
    // Fault (low voltage) to not faulted (voltage in range) triggers clear
    // faults call a third time.

    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Analyze call will also need good READ_VIN value to check.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("201100"));
    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), false);
    // Turn fault on.
    expectations.statusWordValue = (status_word::VIN_UV_FAULT);
    // Curious disagreement between PMBus Spec. Part II Figure 16 and 33. Go by
    // Figure 16, and assume bits on in STATUS_INPUT.
    expectations.statusInputValue = 0x18;
    setPMBusExpectations(mockPMBus, expectations);
    // If there is a VIN_UV fault, fake reading voltage of less than 20V
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("19876"));
    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), true);
    // Back to no fault bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    // Updates now result in clearing faults if read voltage goes from below the
    // minimum, to within a valid range.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("201300"));
    // Went from below minimum to within range, expect CLEAR_FAULTS.
    EXPECT_CALL(mockPMBus, read(READ_VIN, _)).Times(1).WillOnce(Return(3));
    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), false);
}

TEST_F(PowerSupplyTests, HasVoutOVFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x69, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    // Initial value would be 0, so this read updates it to non-zero.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("202100"));
    psu.analyze();
    EXPECT_EQ(psu.hasVoutOVFault(), false);
    // Turn fault on.
    expectations.statusWordValue = (status_word::VOUT_OV_FAULT);
    // STATUS_VOUT fault bit(s)
    expectations.statusVOUTValue = 0x80;
    // STATUS_TEMPERATURE default.
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("202200"));
    psu.analyze();
    EXPECT_EQ(psu.hasVoutOVFault(), true);
    // Back to no fault bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("202300"));
    psu.analyze();
    EXPECT_EQ(psu.hasVoutOVFault(), false);
}

TEST_F(PowerSupplyTests, HasIoutOCFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x6d, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    // Initial value would be 0, so this read updates it to non-zero.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("203100"));
    psu.analyze();
    EXPECT_EQ(psu.hasIoutOCFault(), false);
    // Turn fault on.
    expectations.statusWordValue = status_word::IOUT_OC_FAULT;
    // STATUS_IOUT fault bit(s)
    expectations.statusIOUTValue = 0x88;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("203200"));
    psu.analyze();
    EXPECT_EQ(psu.hasIoutOCFault(), true);
    // Back to no fault bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("203300"));
    psu.analyze();
    EXPECT_EQ(psu.hasIoutOCFault(), false);
}

TEST_F(PowerSupplyTests, HasVoutUVFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x6a, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    // Initial value would be 0, so this read updates it to non-zero.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("204100"));
    psu.analyze();
    EXPECT_EQ(psu.hasVoutUVFault(), false);
    // Turn fault on.
    expectations.statusWordValue = (status_word::VOUT_FAULT);
    // STATUS_VOUT fault bit(s)
    expectations.statusVOUTValue = 0x30;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("204200"));
    psu.analyze();
    EXPECT_EQ(psu.hasVoutUVFault(), true);
    // Back to no fault bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("204300"));
    psu.analyze();
    EXPECT_EQ(psu.hasVoutUVFault(), false);
}

TEST_F(PowerSupplyTests, HasFanFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x6d, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    // Initial value would be 0, so this read updates it to non-zero.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("205100"));
    psu.analyze();
    EXPECT_EQ(psu.hasFanFault(), false);
    // Turn fault on.
    expectations.statusWordValue = (status_word::FAN_FAULT);
    // STATUS_FANS_1_2 fault bit on (Fan 1 Fault)
    expectations.statusFans12Value = 0x80;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("205200"));
    psu.analyze();
    EXPECT_EQ(psu.hasFanFault(), true);
    // Back to no fault bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("205300"));
    psu.analyze();
    EXPECT_EQ(psu.hasFanFault(), false);
}

TEST_F(PowerSupplyTests, HasTempFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x6a, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    // Initial value would be 0, so this read updates it to non-zero.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("206100"));
    psu.analyze();
    EXPECT_EQ(psu.hasTempFault(), false);
    // Turn fault on.
    expectations.statusWordValue = (status_word::TEMPERATURE_FAULT_WARN);
    // STATUS_TEMPERATURE fault bit on (OT Fault)
    expectations.statusTempValue = 0x80;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("206200"));
    psu.analyze();
    EXPECT_EQ(psu.hasTempFault(), true);
    // Back to no fault bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("206300"));
    psu.analyze();
    EXPECT_EQ(psu.hasTempFault(), false);
}

TEST_F(PowerSupplyTests, HasPgoodFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x6b, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    // Initial value would be 0, so this read updates it to non-zero.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("207100"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), false);
    // Setup another expectation of no faults.
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("207200"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), false);
    // Setup another expectation of no faults.
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("207300"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), false);
    // Turn PGOOD# off (fault on).
    expectations.statusWordValue = (status_word::POWER_GOOD_NEGATED);
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("207400"));
    psu.analyze();
    // Expect false until reaches DEGLITCH_LIMIT
    EXPECT_EQ(psu.hasPgoodFault(), false);
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("207500"));
    psu.analyze();
    // Expect false until reaches DEGLITCH_LIMIT
    EXPECT_EQ(psu.hasPgoodFault(), false);
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("207600"));
    psu.analyze();
    // DEGLITCH_LIMIT reached, expect true.
    EXPECT_EQ(psu.hasPgoodFault(), true);
    // Back to no fault bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("207700"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), false);

    // Turn OFF bit on
    expectations.statusWordValue = (status_word::UNIT_IS_OFF);
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208100"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), false);
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208200"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), false);
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208300"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), true);
    // Back to no fault bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208400"));
    psu.analyze();
    EXPECT_EQ(psu.hasPgoodFault(), false);
}

TEST_F(PowerSupplyTests, HasPSKillFault)
{
    auto bus = sdbusplus::bus::new_default();
    PowerSupply psu{bus, PSUInventoryPath, 4, 0x6d, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    // Initial value would be 0, so this read updates it to non-zero.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208100"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSKillFault(), false);
    // Next return STATUS_WORD with MFR fault bit on.
    expectations.statusWordValue = (status_word::MFR_SPECIFIC_FAULT);
    // STATUS_MFR_SPEFIC with bit(s) on.
    expectations.statusMFRValue = 0xFF;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208200"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSKillFault(), true);
    // Back to no bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208300"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSKillFault(), false);
    // Next return STATUS_WORD with MFR fault bit on.
    expectations.statusWordValue = (status_word::MFR_SPECIFIC_FAULT);
    // STATUS_MFR_SPEFIC with bit 4 on.
    expectations.statusMFRValue = 0x10;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208400"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSKillFault(), true);
    // Back to no bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("208500"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSKillFault(), false);
}

TEST_F(PowerSupplyTests, HasPS12VcsFault)
{
    auto bus = sdbusplus::bus::new_default();
    PowerSupply psu{bus, PSUInventoryPath, 5, 0x6e, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    // Call to analyze will trigger read of "in1_input" to check voltage.
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209100"));
    psu.analyze();
    EXPECT_EQ(psu.hasPS12VcsFault(), false);
    // Next return STATUS_WORD with MFR fault bit on.
    expectations.statusWordValue = (status_word::MFR_SPECIFIC_FAULT);
    // STATUS_MFR_SPEFIC with bit(s) on.
    expectations.statusMFRValue = 0xFF;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209200"));
    psu.analyze();
    EXPECT_EQ(psu.hasPS12VcsFault(), true);
    // Back to no bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209300"));
    psu.analyze();
    EXPECT_EQ(psu.hasPS12VcsFault(), false);
    // Next return STATUS_WORD with MFR fault bit on.
    expectations.statusWordValue = (status_word::MFR_SPECIFIC_FAULT);
    // STATUS_MFR_SPEFIC with bit 6 on.
    expectations.statusMFRValue = 0x40;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209400"));
    psu.analyze();
    EXPECT_EQ(psu.hasPS12VcsFault(), true);
    // Back to no bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209500"));
    psu.analyze();
    EXPECT_EQ(psu.hasPS12VcsFault(), false);
}

TEST_F(PowerSupplyTests, HasPSCS12VFault)
{
    auto bus = sdbusplus::bus::new_default();
    PowerSupply psu{bus, PSUInventoryPath, 6, 0x6f, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    setMissingToPresentExpects(mockPMBus, mockedUtil);
    // STATUS_WORD 0x0000 is powered on, no faults.
    PMBusExpectations expectations;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209100"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSCS12VFault(), false);
    // Next return STATUS_WORD with MFR fault bit on.
    expectations.statusWordValue = (status_word::MFR_SPECIFIC_FAULT);
    // STATUS_MFR_SPEFIC with bit(s) on.
    expectations.statusMFRValue = 0xFF;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209200"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSCS12VFault(), true);
    // Back to no bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209300"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSCS12VFault(), false);
    // Next return STATUS_WORD with MFR fault bit on.
    expectations.statusWordValue = (status_word::MFR_SPECIFIC_FAULT);
    // STATUS_MFR_SPEFIC with bit 7 on.
    expectations.statusMFRValue = 0x80;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209400"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSCS12VFault(), true);
    // Back to no bits on in STATUS_WORD
    expectations.statusWordValue = 0;
    setPMBusExpectations(mockPMBus, expectations);
    EXPECT_CALL(mockPMBus, readString(READ_VIN, _))
        .Times(1)
        .WillOnce(Return("209500"));
    psu.analyze();
    EXPECT_EQ(psu.hasPSCS12VFault(), false);
}
