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
#include "chassis.hpp"
#include "compare_presence_action.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "mock_action.hpp"
#include "mock_error_logging.hpp"
#include "mock_journal.hpp"
#include "mock_presence_service.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "presence_detection.hpp"
#include "rule.hpp"
#include "system.hpp"
#include "test_sdbus_error.hpp"

#include <sdbusplus/exception.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::Ref;
using ::testing::Return;
using ::testing::Throw;

/**
 * Creates the parent objects that normally contain a PresenceDetection object.
 *
 * A PresenceDetection object is normally contained within a hierarchy of
 * System, Chassis, and Device objects.  These objects are required in order to
 * call the execute() method.
 *
 * Creates the System, Chassis, and Device objects.  The PresenceDetection
 * object is moved into the Device object.
 *
 * @param detection PresenceDetection object to move into object hierarchy
 * @return Pointers to the System, Chassis, and Device objects.  The Chassis and
 *         Device objects are contained within the System object and will be
 *         automatically destructed.
 */
std::tuple<std::unique_ptr<System>, Chassis*, Device*> createParentObjects(
    std::unique_ptr<PresenceDetection> detection)
{
    // Create mock I2CInterface
    std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
        std::make_unique<i2c::MockedI2CInterface>();

    // Create Device that contains PresenceDetection
    std::unique_ptr<Device> device = std::make_unique<Device>(
        "vdd_reg", true,
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg2",
        std::move(i2cInterface), std::move(detection));
    Device* devicePtr = device.get();

    // Create Chassis that contains Device
    std::vector<std::unique_ptr<Device>> devices{};
    devices.emplace_back(std::move(device));
    std::unique_ptr<Chassis> chassis = std::make_unique<Chassis>(
        1, "/xyz/openbmc_project/inventory/system/chassis", std::move(devices));
    Chassis* chassisPtr = chassis.get();

    // Create System that contains Chassis
    std::vector<std::unique_ptr<Rule>> rules{};
    std::vector<std::unique_ptr<Chassis>> chassisVec{};
    chassisVec.emplace_back(std::move(chassis));
    std::unique_ptr<System> system =
        std::make_unique<System>(std::move(rules), std::move(chassisVec));

    return std::make_tuple(std::move(system), chassisPtr, devicePtr);
}

TEST(PresenceDetectionTests, Constructor)
{
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::make_unique<MockAction>());

    PresenceDetection detection{std::move(actions)};
    EXPECT_EQ(detection.getActions().size(), 1);
    EXPECT_FALSE(detection.getCachedPresence().has_value());
}

TEST(PresenceDetectionTests, ClearCache)
{
    // Create MockAction that will return true once
    std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
    EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(true));

    // Create PresenceDetection
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::move(action));
    PresenceDetection* detection = new PresenceDetection(std::move(actions));

    // Create parent System, Chassis, and Device objects
    auto [system, chassis, device] =
        createParentObjects(std::unique_ptr<PresenceDetection>{detection});

    // Verify that initially no presence value is cached
    EXPECT_FALSE(detection->getCachedPresence().has_value());

    // Call execute() which should obtain and cache presence value
    MockServices services{};
    EXPECT_TRUE(detection->execute(services, *system, *chassis, *device));

    // Verify true presence value was cached
    EXPECT_TRUE(detection->getCachedPresence().has_value());
    EXPECT_TRUE(detection->getCachedPresence().value());

    // Clear cached presence value
    detection->clearCache();

    // Verify that no presence value is cached
    EXPECT_FALSE(detection->getCachedPresence().has_value());
}

TEST(PresenceDetectionTests, Execute)
{
    // Create ComparePresenceAction
    std::unique_ptr<ComparePresenceAction> action =
        std::make_unique<ComparePresenceAction>(
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2",
            true);

    // Create PresenceDetection
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::move(action));
    PresenceDetection* detection = new PresenceDetection(std::move(actions));

    // Create parent System, Chassis, and Device objects
    auto [system, chassis, device] =
        createParentObjects(std::unique_ptr<PresenceDetection>{detection});

    // Test where works: Present: Value is not cached
    {
        EXPECT_FALSE(detection->getCachedPresence().has_value());

        // Create MockServices.  MockPresenceService::isPresent() should return
        // true.
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService,
                    isPresent("/xyz/openbmc_project/inventory/system/chassis/"
                              "motherboard/cpu2"))
            .Times(1)
            .WillOnce(Return(true));

        // Execute PresenceDetection
        EXPECT_TRUE(detection->execute(services, *system, *chassis, *device));

        EXPECT_TRUE(detection->getCachedPresence().has_value());
        EXPECT_TRUE(detection->getCachedPresence().value());
    }

    // Test where works: Present: Value is cached
    {
        EXPECT_TRUE(detection->getCachedPresence().has_value());

        // Create MockServices.  MockPresenceService::isPresent() should not be
        // called.
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService, isPresent).Times(0);

        // Execute PresenceDetection
        EXPECT_TRUE(detection->execute(services, *system, *chassis, *device));
    }

    // Test where works: Not present: Value is not cached
    {
        // Clear cached presence value
        detection->clearCache();
        EXPECT_FALSE(detection->getCachedPresence().has_value());

        // Create MockServices.  MockPresenceService::isPresent() should return
        // false.
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService,
                    isPresent("/xyz/openbmc_project/inventory/system/chassis/"
                              "motherboard/cpu2"))
            .Times(1)
            .WillOnce(Return(false));

        // Execute PresenceDetection
        EXPECT_FALSE(detection->execute(services, *system, *chassis, *device));

        EXPECT_TRUE(detection->getCachedPresence().has_value());
        EXPECT_FALSE(detection->getCachedPresence().value());
    }

    // Test where works: Not present: Value is cached
    {
        EXPECT_TRUE(detection->getCachedPresence().has_value());

        // Create MockServices.  MockPresenceService::isPresent() should not be
        // called.
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService, isPresent).Times(0);

        // Execute PresenceDetection
        EXPECT_FALSE(detection->execute(services, *system, *chassis, *device));
    }

    // Test where fails
    {
        // Clear cached presence value
        detection->clearCache();
        EXPECT_FALSE(detection->getCachedPresence().has_value());

        // Create MockServices.  MockPresenceService::isPresent() should throw
        // an exception.
        MockServices services{};
        MockPresenceService& presenceService =
            services.getMockPresenceService();
        EXPECT_CALL(presenceService,
                    isPresent("/xyz/openbmc_project/inventory/system/chassis/"
                              "motherboard/cpu2"))
            .Times(1)
            .WillOnce(Throw(TestSDBusError{"DBusError: Invalid object path."}));

        // Define expected journal messages that should be passed to MockJournal
        MockJournal& journal = services.getMockJournal();
        std::vector<std::string> exceptionMessages{
            "DBusError: Invalid object path.",
            "ActionError: compare_presence: { fru: "
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu2, "
            "value: true }"};
        EXPECT_CALL(journal, logError(exceptionMessages)).Times(1);
        EXPECT_CALL(journal,
                    logError("Unable to determine presence of vdd_reg"))
            .Times(1);

        // Expect logDBusError() to be called
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging,
                    logDBusError(Entry::Level::Warning, Ref(journal)))
            .Times(1);

        // Execute PresenceDetection.  Should return true when an error occurs.
        EXPECT_TRUE(detection->execute(services, *system, *chassis, *device));

        EXPECT_TRUE(detection->getCachedPresence().has_value());
        EXPECT_TRUE(detection->getCachedPresence().value());
    }
}

TEST(PresenceDetectionTests, GetActions)
{
    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.emplace_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.emplace_back(std::unique_ptr<MockAction>{action2});

    PresenceDetection detection{std::move(actions)};
    EXPECT_EQ(detection.getActions().size(), 2);
    EXPECT_EQ(detection.getActions()[0].get(), action1);
    EXPECT_EQ(detection.getActions()[1].get(), action2);
}

TEST(PresenceDetectionTests, GetCachedPresence)
{
    // Create MockAction that will return false once
    std::unique_ptr<MockAction> action = std::make_unique<MockAction>();
    EXPECT_CALL(*action, execute).Times(1).WillOnce(Return(false));

    // Create PresenceDetection
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::move(action));
    PresenceDetection* detection = new PresenceDetection(std::move(actions));

    // Create parent System, Chassis, and Device objects
    auto [system, chassis, device] =
        createParentObjects(std::unique_ptr<PresenceDetection>{detection});

    // Verify that initially no presence value is cached
    EXPECT_FALSE(detection->getCachedPresence().has_value());

    // Call execute() which should obtain and cache presence value
    MockServices services{};
    EXPECT_FALSE(detection->execute(services, *system, *chassis, *device));

    // Verify false presence value was cached
    EXPECT_TRUE(detection->getCachedPresence().has_value());
    EXPECT_FALSE(detection->getCachedPresence().value());

    // Clear cached presence value
    detection->clearCache();

    // Verify that no presence value is cached
    EXPECT_FALSE(detection->getCachedPresence().has_value());
}
