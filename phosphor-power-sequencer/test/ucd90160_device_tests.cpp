/**
 * Copyright © 2024 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mock_pmbus.hpp"
#include "mock_services.hpp"
#include "pmbus.hpp"
#include "rail.hpp"
#include "services.hpp"
#include "ucd90160_device.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::sequencer;
using namespace phosphor::pmbus;

using ::testing::Return;

/**
 * Creates a Rail object that checks for a pgood fault using a GPIO.
 *
 * @param name Unique name for the rail
 * @param gpio GPIO line to read to determine the pgood status of the rail
 * @return Rail object
 */
static std::unique_ptr<Rail> createRail(const std::string& name,
                                        unsigned int gpioLine)
{
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    bool activeLow{false};
    std::optional<GPIO> gpio{GPIO{gpioLine, activeLow}};
    return std::make_unique<Rail>(name, presence, page, isPowerSupplyRail,
                                  checkStatusVout, compareVoltageToLimit, gpio);
}

TEST(UCD90160DeviceTests, Constructor)
{
    MockServices services;

    std::vector<std::unique_ptr<Rail>> rails;
    rails.emplace_back(createRail("VDD", 5));
    rails.emplace_back(createRail("VIO", 7));
    uint8_t bus{3};
    uint16_t address{0x72};
    UCD90160Device device{std::move(rails), services, bus, address};

    EXPECT_EQ(device.getName(), "UCD90160");
    EXPECT_EQ(device.getRails().size(), 2);
    EXPECT_EQ(device.getRails()[0]->getName(), "VDD");
    EXPECT_EQ(device.getRails()[1]->getName(), "VIO");
    EXPECT_EQ(device.getBus(), bus);
    EXPECT_EQ(device.getAddress(), address);
    EXPECT_EQ(device.getDriverName(), "ucd9000");
    EXPECT_EQ(device.getInstance(), 0);
    EXPECT_NE(&(device.getPMBusInterface()), nullptr);
}

TEST(UCD90160DeviceTests, StoreGPIOValues)
{
    // This is a protected method and cannot be called directly from a gtest.
    // Call findPgoodFault() which calls storeGPIOValues().

    // Test where works
    {
        std::vector<int> gpioValues{
            1, 0, 0, 1, // group 1 in journal
            1, 1, 0, 0, // group 2 in journal
            1, 0, 1, 1, // group 3 in journal
            0, 0, 1, 1, // group 4 in journal
            1, 0, 0, 0, // group 5 in journal
            1, 0, 0, 1, // group 6 in journal
            1, 1        // group 7 in journal
        };

        MockServices services;
        EXPECT_CALL(services, getGPIOValues("ucd90160"))
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(services, logInfoMsg("Device UCD90160 GPIO values:"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg(
                        "[FPWM1_GPIO5, FPWM2_GPIO6, FPWM3_GPIO7, FPWM4_GPIO8]: "
                        "[1, 0, 0, 1]"))
            .Times(1);
        EXPECT_CALL(
            services,
            logInfoMsg(
                "[FPWM5_GPIO9, FPWM6_GPIO10, FPWM7_GPIO11, FPWM8_GPIO12]: "
                "[1, 1, 0, 0]"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("[GPI1_PWM1, GPI2_PWM2, GPI3_PWM3, GPI4_PWM4]: "
                               "[1, 0, 1, 1]"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("[GPIO14, GPIO15, TDO_GPIO20, TCK_GPIO19]: "
                               "[0, 0, 1, 1]"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("[TMS_GPIO22, TDI_GPIO21, GPIO1, GPIO2]: "
                               "[1, 0, 0, 0]"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("[GPIO3, GPIO4, GPIO13, GPIO16]: "
                                         "[1, 0, 0, 1]"))
            .Times(1);
        EXPECT_CALL(services, logInfoMsg("[GPIO17, GPIO18]: "
                                         "[1, 1]"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("Device UCD90160 MFR_STATUS: 0x123456789abc"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device UCD90160"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD pgood GPIO line offset 2 has inactive value 0"))
            .Times(1);

        std::vector<std::unique_ptr<Rail>> rails;
        rails.emplace_back(createRail("VDD", 2));
        uint8_t bus{3};
        uint16_t address{0x72};
        UCD90160Device device{std::move(rails), services, bus, address};

        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return("/tmp"));
        EXPECT_CALL(pmbus, read("mfr_status", Type::HwmonDeviceDebug, true))
            .Times(1)
            .WillOnce(Return(0x123456789abcull));

        // Call findPgoodFault() which calls storeGPIOValues()
        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        std::string error =
            device.findPgoodFault(services, powerSupplyError, additionalData);
        EXPECT_EQ(error,
                  "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault");
        EXPECT_EQ(additionalData.size(), 31);
        EXPECT_EQ(additionalData["MFR_STATUS"], "0x123456789abc");
        EXPECT_EQ(additionalData["DEVICE_NAME"], "UCD90160");
        EXPECT_EQ(additionalData["FPWM1_GPIO5"], "1");
        EXPECT_EQ(additionalData["FPWM2_GPIO6"], "0");
        EXPECT_EQ(additionalData["FPWM3_GPIO7"], "0");
        EXPECT_EQ(additionalData["FPWM4_GPIO8"], "1");
        EXPECT_EQ(additionalData["FPWM5_GPIO9"], "1");
        EXPECT_EQ(additionalData["FPWM6_GPIO10"], "1");
        EXPECT_EQ(additionalData["FPWM7_GPIO11"], "0");
        EXPECT_EQ(additionalData["FPWM8_GPIO12"], "0");
        EXPECT_EQ(additionalData["GPI1_PWM1"], "1");
        EXPECT_EQ(additionalData["GPI2_PWM2"], "0");
        EXPECT_EQ(additionalData["GPI3_PWM3"], "1");
        EXPECT_EQ(additionalData["GPI4_PWM4"], "1");
        EXPECT_EQ(additionalData["GPIO14"], "0");
        EXPECT_EQ(additionalData["GPIO15"], "0");
        EXPECT_EQ(additionalData["TDO_GPIO20"], "1");
        EXPECT_EQ(additionalData["TCK_GPIO19"], "1");
        EXPECT_EQ(additionalData["TMS_GPIO22"], "1");
        EXPECT_EQ(additionalData["TDI_GPIO21"], "0");
        EXPECT_EQ(additionalData["GPIO1"], "0");
        EXPECT_EQ(additionalData["GPIO2"], "0");
        EXPECT_EQ(additionalData["GPIO3"], "1");
        EXPECT_EQ(additionalData["GPIO4"], "0");
        EXPECT_EQ(additionalData["GPIO13"], "0");
        EXPECT_EQ(additionalData["GPIO16"], "1");
        EXPECT_EQ(additionalData["GPIO17"], "1");
        EXPECT_EQ(additionalData["GPIO18"], "1");
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD");
        EXPECT_EQ(additionalData["GPIO_LINE"], "2");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "0");
    }

    // Test where there are the wrong number of GPIOs (27 instead of 26)
    {
        std::vector<int> gpioValues{
            1, 0, 0, 1, // group 1 in journal
            1, 1, 0, 0, // group 2 in journal
            1, 0, 1, 1, // group 3 in journal
            0, 0, 1, 1, // group 4 in journal
            1, 0, 0, 0, // group 5 in journal
            1, 0, 0, 1, // group 6 in journal
            1, 1, 0     // group 7 in journal + extra value
        };

        MockServices services;
        EXPECT_CALL(services, getGPIOValues("ucd90160"))
            .Times(1)
            .WillOnce(Return(gpioValues));
        EXPECT_CALL(services,
                    logInfoMsg("Device UCD90160 GPIO values: ["
                               "1, 0, 0, 1, "
                               "1, 1, 0, 0, "
                               "1, 0, 1, 1, "
                               "0, 0, 1, 1, "
                               "1, 0, 0, 0, "
                               "1, 0, 0, 1, "
                               "1, 1, 0]"))
            .Times(1);
        EXPECT_CALL(services,
                    logInfoMsg("Device UCD90160 MFR_STATUS: 0x123456789abc"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Pgood fault found in rail monitored by device UCD90160"))
            .Times(1);
        EXPECT_CALL(services, logErrorMsg("Pgood fault detected in rail VDD"))
            .Times(1);
        EXPECT_CALL(
            services,
            logErrorMsg(
                "Rail VDD pgood GPIO line offset 2 has inactive value 0"))
            .Times(1);

        std::vector<std::unique_ptr<Rail>> rails;
        rails.emplace_back(createRail("VDD", 2));
        uint8_t bus{3};
        uint16_t address{0x72};
        UCD90160Device device{std::move(rails), services, bus, address};

        MockPMBus& pmbus = static_cast<MockPMBus&>(device.getPMBusInterface());
        EXPECT_CALL(pmbus, getPath(Type::Hwmon))
            .Times(1)
            .WillOnce(Return("/tmp"));
        EXPECT_CALL(pmbus, read("mfr_status", Type::HwmonDeviceDebug, true))
            .Times(1)
            .WillOnce(Return(0x123456789abcull));

        // Call findPgoodFault() which calls storeGPIOValues()
        std::string powerSupplyError{};
        std::map<std::string, std::string> additionalData{};
        std::string error =
            device.findPgoodFault(services, powerSupplyError, additionalData);
        EXPECT_EQ(error,
                  "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault");
        EXPECT_EQ(additionalData.size(), 6);
        EXPECT_EQ(additionalData["MFR_STATUS"], "0x123456789abc");
        EXPECT_EQ(additionalData["DEVICE_NAME"], "UCD90160");
        EXPECT_EQ(additionalData["GPIO_VALUES"],
                  "[1, 0, 0, 1, "
                  "1, 1, 0, 0, "
                  "1, 0, 1, 1, "
                  "0, 0, 1, 1, "
                  "1, 0, 0, 0, "
                  "1, 0, 0, 1, "
                  "1, 1, 0]");
        EXPECT_EQ(additionalData["RAIL_NAME"], "VDD");
        EXPECT_EQ(additionalData["GPIO_LINE"], "2");
        EXPECT_EQ(additionalData["GPIO_VALUE"], "0");
    }
}
