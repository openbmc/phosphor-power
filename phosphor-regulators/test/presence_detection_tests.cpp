/**
 * Copyright © 2020 IBM Corporation
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
#include "action.hpp"
#include "action_error.hpp"
#include "chassis.hpp"
#include "compare_presence_action.hpp"
#include "configuration.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "mock_action.hpp"
#include "mock_journal.hpp"
#include "mock_presence_service.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "presence_detection.hpp"
#include "rule.hpp"
#include "system.hpp"

#include <memory>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::A;
using ::testing::Return;
using ::testing::Throw;

TEST(PresenceDetectionTests, Constructor)
{
    std::vector<std::unique_ptr<Action>> actions{};
    actions.push_back(std::make_unique<MockAction>());

    PresenceDetection presenceDetection(std::move(actions));
    EXPECT_EQ(presenceDetection.getActions().size(), 1);
}

TEST(PresenceDetectionTests, Execute)
{
    // Test where works.
    {
        // Create MockServices. Returns true for the first time call of
        // isPresent() and returns false for the second time.
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService, isPresent(A<const std::string&>()))
            .Times(2)
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        // Create ComparePresenceAction
        std::unique_ptr<ComparePresenceAction> action = std::make_unique<
            ComparePresenceAction>(
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2",
            true);

        // Create mock I2CInterface.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();

        // Create Configuration
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));
        PresenceDetection* presenceDetectionPtr = presenceDetection.get();

        // Create Device that contains Configuration
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Test where works: device is present
        {
            // Execute Configuration
            EXPECT_EQ(presenceDetectionPtr->execute(services, system,
                                                    *chassisPtr, *devicePtr),
                      true);
        }

        // Test where works: device is not present
        {
            // Execute Configuration
            EXPECT_EQ(presenceDetectionPtr->execute(services, system,
                                                    *chassisPtr, *devicePtr),
                      false);
        }
    }

    // Test where fails
    {
        // Create MockServices and MockPresenceServices. Reading presence
        // failed.
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService, isPresent(A<const std::string&>()))
            .Times(1)
            .WillOnce(Throw(std::runtime_error(
                "PresenceService cannot get the presence.")));

        // Create mock journal
        MockJournal& journal = services.getMockJournal();
        std::vector<std::string> expectedErrMessagesException{
            "PresenceService cannot get the presence.",
            "ActionError: compare_presence: { fru: "
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2, "
            "value: true }"};
        EXPECT_CALL(journal, logError(expectedErrMessagesException)).Times(1);
        EXPECT_CALL(journal, logError("Unable to detect presence vdd_reg"))
            .Times(1);

        // Create ComparePresenceAction
        std::unique_ptr<ComparePresenceAction> action = std::make_unique<
            ComparePresenceAction>(
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2",
            true);

        // Create mock I2CInterface.
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();

        // Create Configuration
        std::vector<std::unique_ptr<Action>> actions{};
        actions.emplace_back(std::move(action));
        std::unique_ptr<PresenceDetection> presenceDetection =
            std::make_unique<PresenceDetection>(std::move(actions));
        PresenceDetection* presenceDetectionPtr = presenceDetection.get();

        // Create Device that contains Configuration
        std::unique_ptr<Configuration> configuration{};
        std::unique_ptr<Device> device = std::make_unique<Device>(
            "vdd_reg", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
            std::move(i2cInterface), std::move(presenceDetection),
            std::move(configuration));
        Device* devicePtr = device.get();

        // Create Chassis that contains Device
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(device));
        std::unique_ptr<Chassis> chassis =
            std::make_unique<Chassis>(1, std::move(devices));
        Chassis* chassisPtr = chassis.get();

        // Create System that contains Chassis
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        System system{std::move(rules), std::move(chassisVec)};

        // Execute Configuration
        presenceDetectionPtr->execute(services, system, *chassisPtr,
                                      *devicePtr);
    }
}

TEST(PresenceDetectionTests, GetActions)
{
    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action2});

    PresenceDetection presenceDetection(std::move(actions));
    EXPECT_EQ(presenceDetection.getActions().size(), 2);
    EXPECT_EQ(presenceDetection.getActions()[0].get(), action1);
    EXPECT_EQ(presenceDetection.getActions()[1].get(), action2);
}
