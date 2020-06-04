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

TEST_F(PowerSupplyTests, Constructor)
{
    /**
     * @param[in] invpath - String for inventory path to use
     * @param[in] i2cbus - The bus number this power supply is on
     * @param[in] i2caddr - The 16-bit I2C address of the power supply
     */
    auto bus = sdbusplus::bus::new_default();
    EXPECT_CALL(mockedUtil, getPresence(_, StrEq(PSUInventoryPath))).Times(1);
    auto psu = std::make_unique<PowerSupply>(bus, PSUInventoryPath, 3, "0068");

    EXPECT_EQ(psu->isPresent(), false);
    EXPECT_EQ(psu->isFaulted(), false);
    EXPECT_EQ(psu->hasInputFault(), false);
    EXPECT_EQ(psu->hasMFRFault(), false);
    EXPECT_EQ(psu->hasVINUVFault(), false);
}

TEST_F(PowerSupplyTests, Analyze)
{
    auto bus = sdbusplus::bus::new_default();

    EXPECT_CALL(mockedUtil, getPresence(_, StrEq(PSUInventoryPath))).Times(1);
    PowerSupply psu{bus, PSUInventoryPath, 4, "0069"};
    psu.analyze();
    // By default, nothing should change.
    EXPECT_EQ(psu.isPresent(), false);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);

    // In order to get the various faults tested, the power supply needs to be
    // present in order to read from the PMBus device(s).
    EXPECT_CALL(mockedUtil, getPresence(_, StrEq(PSUInventoryPath)))
        .Times(1)
        .WillOnce(Return(true)); // present
    PowerSupply psu2{bus, PSUInventoryPath, 5, "006a"};
    EXPECT_EQ(psu2.isPresent(), true);

    // STATUS_WORD 0x0000 is powered on, no faults.
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu2.getPMBus());
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0x0000));
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), false);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // STATUS_WORD input fault/warn
    EXPECT_CALL(mockPMBus, read(_, _))
        .Times(1)
        .WillOnce(Return(status_word::INPUT_FAULT_WARN));
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), true);
    EXPECT_EQ(psu2.hasInputFault(), true);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // STATUS_WORD INPUT/UV fault.
    // First need it to return good status, then the fault
    EXPECT_CALL(mockPMBus, read(_, _))
        .WillOnce(Return(0x0000))
        .WillOnce(Return(status_word::VIN_UV_FAULT));
    psu2.analyze();
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), true);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), true);

    // STATUS_WORD MFR fault.
    EXPECT_CALL(mockPMBus, read(_, _))
        .WillOnce(Return(0x0000))
        .WillOnce(Return(status_word::MFR_SPECIFIC_FAULT));
    psu2.analyze();
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), true);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), true);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // Ignore Temperature fault.
    EXPECT_CALL(mockPMBus, read(_, _))
        .WillOnce(Return(0x0000))
        .WillOnce(Return(status_word::TEMPERATURE_FAULT_WARN));
    psu2.analyze();
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), false);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // Ignore fan fault
    EXPECT_CALL(mockPMBus, read(_, _))
        .WillOnce(Return(0x0000))
        .WillOnce(Return(status_word::FAN_FAULT));
    psu2.analyze();
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), false);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // TODO: ReadFailure
}

TEST_F(PowerSupplyTests, OnOffConfig)
{
    auto bus = sdbusplus::bus::new_default();
    uint8_t data = 0x15;

    try
    {
        EXPECT_CALL(mockedUtil, getPresence(_, StrEq(PSUInventoryPath)))
            .Times(1);
        PowerSupply psu{bus, PSUInventoryPath, 4, "0069"};
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        // If it is not present, I should not be trying to write to it.
        EXPECT_CALL(mockPMBus, writeBinary(_, _, _)).Times(0);
        psu.onOffConfig(data);
    }
    catch (...)
    {
    }

    try
    {
        // TODO: presence should matter?
        EXPECT_CALL(mockedUtil, getPresence(_, StrEq(PSUInventoryPath)))
            .Times(1)
            .WillOnce(Return(true)); // present
        PowerSupply psu2{bus, PSUInventoryPath, 5, "006a"};
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu2.getPMBus());
        // TODO: ???should I check the filename?
        EXPECT_CALL(mockPMBus, writeBinary(_, ElementsAre(0x15),
                                           phosphor::pmbus::Type::Debug))
            .Times(1);
        psu2.onOffConfig(data);
    }
    catch (...)
    {
    }
}

TEST_F(PowerSupplyTests, ClearFaults)
{
    auto bus = sdbusplus::bus::new_default();
    EXPECT_CALL(mockedUtil, getPresence(_, StrEq(PSUInventoryPath)))
        .Times(1)
        .WillOnce(Return(true)); // present
    PowerSupply psu{bus, PSUInventoryPath, 13, "0068"};
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0xFFFF));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), true);
    EXPECT_EQ(psu.hasInputFault(), true);
    EXPECT_EQ(psu.hasMFRFault(), true);
    EXPECT_EQ(psu.hasVINUVFault(), true);
    EXPECT_CALL(mockPMBus, read("in1_input", _))
        .Times(1)
        .WillOnce(Return(209000));
    psu.clearFaults();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);
}

TEST_F(PowerSupplyTests, UpdateInventory)
{
    auto bus = sdbusplus::bus::new_default();
    EXPECT_CALL(mockedUtil, getPresence(_, StrEq(PSUInventoryPath)))
        .Times(1)
        .WillOnce(Return(true)); // present
    PowerSupply psu{bus, PSUInventoryPath, 3, "0068"};
    psu.updateInventory();
    // TODO: Checks / Story #921
}

TEST_F(PowerSupplyTests, IsPresent)
{
    auto bus = sdbusplus::bus::new_default();
    EXPECT_CALL(mockedUtil, getPresence(_, StrEq(PSUInventoryPath))).Times(1);
    PowerSupply psu{bus, PSUInventoryPath, 3, "0068"};
    EXPECT_EQ(psu.isPresent(), false);

    EXPECT_CALL(mockedUtil, getPresence(_, _))
        .WillOnce(Return(true)); // present
    PowerSupply psu2{bus, PSUInventoryPath, 10, "006b"};
    EXPECT_EQ(psu2.isPresent(), true);
}

TEST_F(PowerSupplyTests, IsFaulted)
{
    auto bus = sdbusplus::bus::new_default();
    EXPECT_CALL(mockedUtil, getPresence(_, _))
        .WillOnce(Return(true)); // present
    PowerSupply psu{bus, PSUInventoryPath, 11, "006f"};
    EXPECT_EQ(psu.isFaulted(), false);
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0xFFFF));
    psu.analyze();
    EXPECT_EQ(psu.isFaulted(), true);
}

TEST_F(PowerSupplyTests, HasInputFault)
{
    auto bus = sdbusplus::bus::new_default();
    EXPECT_CALL(mockedUtil, getPresence(_, _))
        .WillOnce(Return(true)); // present
    PowerSupply psu{bus, PSUInventoryPath, 3, "0068"};
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_CALL(mockPMBus, read(_, _))
        .Times(1)
        .WillOnce(Return(status_word::INPUT_FAULT_WARN));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), true);
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), false);
}

TEST_F(PowerSupplyTests, HasMFRFault)
{
    auto bus = sdbusplus::bus::new_default();
    EXPECT_CALL(mockedUtil, getPresence(_, _))
        .WillOnce(Return(true)); // present
    PowerSupply psu{bus, PSUInventoryPath, 3, "0068"};
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_CALL(mockPMBus, read(_, _))
        .Times(1)
        .WillOnce(Return(status_word::MFR_SPECIFIC_FAULT));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), true);
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), false);
}

TEST_F(PowerSupplyTests, HasVINUVFault)
{
    auto bus = sdbusplus::bus::new_default();
    EXPECT_CALL(mockedUtil, getPresence(_, _))
        .WillOnce(Return(true)); // present
    PowerSupply psu{bus, PSUInventoryPath, 3, "0068"};
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    EXPECT_EQ(psu.hasVINUVFault(), false);
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), false);
    EXPECT_CALL(mockPMBus, read(_, _))
        .Times(1)
        .WillOnce(Return(status_word::VIN_UV_FAULT));
    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), true);
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), false);
}
