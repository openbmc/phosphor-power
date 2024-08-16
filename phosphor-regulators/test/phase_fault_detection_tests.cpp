/**
 * Copyright © 2021 IBM Corporation
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
#include "device.hpp"
#include "i2c_capture_bytes_action.hpp"
#include "i2c_compare_bit_action.hpp"
#include "i2c_interface.hpp"
#include "if_action.hpp"
#include "log_phase_fault_action.hpp"
#include "mock_action.hpp"
#include "mock_error_logging.hpp"
#include "mock_journal.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"
#include "phase_fault.hpp"
#include "phase_fault_detection.hpp"
#include "rule.hpp"
#include "system.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::_;
using ::testing::A;
using ::testing::NotNull;
using ::testing::Ref;
using ::testing::Return;
using ::testing::SetArrayArgument;
using ::testing::Throw;
using ::testing::TypedEq;

class PhaseFaultDetectionTests : public ::testing::Test
{
  public:
    /**
     * Constructor.
     *
     * Creates the following objects needed for calling the
     * PhaseFaultDetection::execute() method:
     * - Regulator Device
     * - I/O expander Device
     * - Chassis that contains the Devices
     * - System that contains the Chassis
     *
     * Saves pointers to these objects in data members so they can be easily
     * accessed in tests.  Also saves pointers to the MockI2CInterface objects
     * so they can be used in mock expectations.
     */
    PhaseFaultDetectionTests() : ::testing::Test{}
    {
        // Create mock I2CInterface for regulator Device and save pointer
        auto regI2CInterface = std::make_unique<i2c::MockedI2CInterface>();
        this->regI2CInterface = regI2CInterface.get();

        // Create regulator Device and save pointer
        auto regulator = std::make_unique<Device>(
            "vdd1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/vdd1",
            std::move(regI2CInterface));
        this->regulator = regulator.get();

        // Create mock I2CInterface for I/O expander Device and save pointer
        auto ioExpI2CInterface = std::make_unique<i2c::MockedI2CInterface>();
        this->ioExpI2CInterface = ioExpI2CInterface.get();

        // Create I/O expander Device and save pointer
        auto ioExpander = std::make_unique<Device>(
            "ioexp1", false,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/ioexp1",
            std::move(ioExpI2CInterface));
        this->ioExpander = ioExpander.get();

        // Create Chassis that contains Devices and save pointer
        std::vector<std::unique_ptr<Device>> devices{};
        devices.emplace_back(std::move(regulator));
        devices.emplace_back(std::move(ioExpander));
        auto chassis = std::make_unique<Chassis>(
            1, "/xyz/openbmc_project/inventory/system/chassis",
            std::move(devices));
        this->chassis = chassis.get();

        // Create System that contains Chassis and save pointer
        std::vector<std::unique_ptr<Rule>> rules{};
        std::vector<std::unique_ptr<Chassis>> chassisVec{};
        chassisVec.emplace_back(std::move(chassis));
        this->system =
            std::make_unique<System>(std::move(rules), std::move(chassisVec));
    }

  protected:
    /**
     * Note: The following pointers do NOT need to be explicitly deleted.  They
     * point to objects that are owned by the System object.  All the objects
     * will be automatically deleted.
     */
    i2c::MockedI2CInterface* regI2CInterface{nullptr};
    Device* regulator{nullptr};
    i2c::MockedI2CInterface* ioExpI2CInterface{nullptr};
    Device* ioExpander{nullptr};
    Chassis* chassis{nullptr};

    /**
     * System object.  Owns all the other objects and will automatically delete
     * them.
     */
    std::unique_ptr<System> system{};
};

TEST_F(PhaseFaultDetectionTests, Constructor)
{
    // Test where device ID not specified
    {
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        PhaseFaultDetection detection{std::move(actions)};
        EXPECT_EQ(detection.getActions().size(), 1);
        EXPECT_EQ(detection.getDeviceID(), "");
    }

    // Test where device ID not specified
    {
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());
        actions.push_back(std::make_unique<MockAction>());

        PhaseFaultDetection detection{std::move(actions), "ioexp1"};
        EXPECT_EQ(detection.getActions().size(), 2);
        EXPECT_EQ(detection.getDeviceID(), "ioexp1");
    }
}

TEST_F(PhaseFaultDetectionTests, ClearErrorHistory)
{
    std::vector<std::unique_ptr<Action>> actions{};

    // Create MockAction that will switch every 5 times between working and
    // throwing an exception.  Expect it to be executed 20 times.
    std::logic_error error{"Logic error"};
    auto action = std::make_unique<MockAction>();
    EXPECT_CALL(*action, execute)
        .Times(20)
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Throw(error))
        .WillOnce(Throw(error))
        .WillOnce(Throw(error))
        .WillOnce(Throw(error))
        .WillOnce(Throw(error))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Throw(error))
        .WillOnce(Throw(error))
        .WillOnce(Throw(error))
        .WillOnce(Throw(error))
        .WillOnce(Throw(error));
    actions.push_back(std::move(action));

    // Create a LogPhaseFaultAction that will log N faults
    actions.push_back(std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n));

    // Create a LogPhaseFaultAction that will log N+1 faults
    actions.push_back(
        std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n_plus_1));

    // Create PhaseFaultDetection
    PhaseFaultDetection detection{std::move(actions)};

    // Create lambda that sets Journal and ErrorLogging expectations when
    // performing phase fault detection 10 times.  The lambda allows us to
    // set the same expectations twice without duplicate code.
    auto setExpectations = [](MockServices& services) {
        // Set Journal service expectations:
        // - 3 error messages for the MockAction exceptions
        // - 3 error messages for inability to detect phase faults
        // - 2 error messages for the N phase fault
        // - 2 error messages for the N+1 phase fault
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(std::vector<std::string>{"Logic error"}))
            .Times(3);
        EXPECT_CALL(journal,
                    logError("Unable to detect phase faults in regulator vdd1"))
            .Times(3);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=2"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=2"))
            .Times(1);

        // Set ErrorLogging service expectations:
        // - Internal error should be logged once for the MockAction exceptions
        // - N phase fault error should be logged once
        // - N+1 phase fault error should be logged once
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logInternalError).Times(1);
        EXPECT_CALL(errorLogging, logPhaseFault(_, _, PhaseFaultType::n, _, _))
            .Times(1);
        EXPECT_CALL(errorLogging,
                    logPhaseFault(_, _, PhaseFaultType::n_plus_1, _, _))
            .Times(1);
    };

    // Perform phase fault detection 10 times to set error history data members
    {
        // Create mock services.  Set expectations via lambda.
        MockServices services{};
        setExpectations(services);

        // Execute PhaseFaultDetection 10 times
        for (int i = 1; i <= 10; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }

    // Clear error history
    detection.clearErrorHistory();

    // Perform phase fault detection 10 more times.  Verify errors logged again.
    {
        // Create mock services.  Set expectations via lambda.
        MockServices services{};
        setExpectations(services);

        // Execute PhaseFaultDetection 10 times
        for (int i = 1; i <= 10; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }
}

TEST_F(PhaseFaultDetectionTests, Execute)
{
    // Test where Device ID was specified
    {
        // Create I2CCompareBitAction that will use an I2CInterface
        auto action = std::make_unique<I2CCompareBitAction>(0x1C, 2, 0);

        // Create PhaseFaultDetection.  Specify device ID of I/O expander.
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        PhaseFaultDetection detection{std::move(actions), "ioexp1"};

        // Set expectations for regulator I2C interface.  Should not be used.
        EXPECT_CALL(*regI2CInterface, isOpen).Times(0);
        EXPECT_CALL(*regI2CInterface, read(0x1C, A<uint8_t&>())).Times(0);

        // Set expectations for I/O expander I2C interface.  Should be used.
        EXPECT_CALL(*ioExpI2CInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*ioExpI2CInterface, read(0x1C, A<uint8_t&>())).Times(1);

        // Create mock services.  Expect no errors to be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        // Execute PhaseFaultDetection
        detection.execute(services, *system, *chassis, *regulator);
    }

    // Test where Device ID was not specified
    {
        // Create I2CCompareBitAction that will use an I2CInterface
        auto action = std::make_unique<I2CCompareBitAction>(0x1C, 2, 0);

        // Create PhaseFaultDetection.  Specify no device ID, which means the
        // regulator should be used.
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        PhaseFaultDetection detection{std::move(actions)};

        // Set expectations for regulator I2C interface.  Should be used.
        EXPECT_CALL(*regI2CInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*regI2CInterface, read(0x1C, A<uint8_t&>())).Times(1);

        // Set expectations for I/O expander I2C interface.  Should not be used.
        EXPECT_CALL(*ioExpI2CInterface, isOpen).Times(0);
        EXPECT_CALL(*ioExpI2CInterface, read(0x1C, A<uint8_t&>())).Times(0);

        // Create mock services.  Expect no errors to be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        // Execute PhaseFaultDetection
        detection.execute(services, *system, *chassis, *regulator);
    }

    // Test where no phase faults detected
    {
        // Create MockAction.  Expect it to be executed 3 times.
        auto action = std::make_unique<MockAction>();
        EXPECT_CALL(*action, execute).Times(3).WillRepeatedly(Return(true));

        // Create PhaseFaultDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        PhaseFaultDetection detection{std::move(actions)};

        // Create mock services.  Expect no errors to be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        // Execute PhaseFaultDetection 3 times
        for (int i = 1; i <= 3; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }

    // Test where N fault occurs, but not twice in a row
    {
        // Create MockAction that will alternate between returning true and
        // false.  Expect it to be executed 6 times.  Use it for the "condition"
        // of an IfAction.
        auto conditionAction = std::make_unique<MockAction>();
        EXPECT_CALL(*conditionAction, execute)
            .Times(6)
            .WillOnce(Return(true))
            .WillOnce(Return(false))
            .WillOnce(Return(true))
            .WillOnce(Return(false))
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        // Create a LogPhaseFaultAction that will log an N phase fault in the
        // ActionEnvironment.  Use it for the "then" clause of an IfAction.
        auto logPhaseFaultAction =
            std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);

        // Create an IfAction that will log an N phase fault in the
        // ActionEnvironment if the mock condition is true.
        std::vector<std::unique_ptr<Action>> thenActions{};
        thenActions.push_back(std::move(logPhaseFaultAction));
        auto ifAction = std::make_unique<IfAction>(std::move(conditionAction),
                                                   std::move(thenActions));

        // Create PhaseFaultDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(ifAction));
        PhaseFaultDetection detection{std::move(actions)};

        // Create mock services.  Expect 3 error messages in the journal for
        // an N phase fault detected with the consecutive count = 1.  Expect no
        // phase fault error to be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=1"))
            .Times(3);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=2"))
            .Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        // Execute PhaseFaultDetection 6 times
        for (int i = 1; i <= 6; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }

    // Test where N+1 fault occurs, but not twice in a row
    {
        // Create MockAction that will alternate between returning true and
        // false.  Expect it to be executed 6 times.  Use it for the "condition"
        // of an IfAction.
        auto conditionAction = std::make_unique<MockAction>();
        EXPECT_CALL(*conditionAction, execute)
            .Times(6)
            .WillOnce(Return(true))
            .WillOnce(Return(false))
            .WillOnce(Return(true))
            .WillOnce(Return(false))
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        // Create a LogPhaseFaultAction that will log an N+1 phase fault in the
        // ActionEnvironment.  Use it for the "then" clause of an IfAction.
        auto logPhaseFaultAction =
            std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n_plus_1);

        // Create an IfAction that will log an N+1 phase fault in the
        // ActionEnvironment if the mock condition is true.
        std::vector<std::unique_ptr<Action>> thenActions{};
        thenActions.push_back(std::move(logPhaseFaultAction));
        auto ifAction = std::make_unique<IfAction>(std::move(conditionAction),
                                                   std::move(thenActions));

        // Create PhaseFaultDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(ifAction));
        PhaseFaultDetection detection{std::move(actions)};

        // Create mock services.  Expect 3 error messages in the journal for
        // an N+1 phase fault detected with the consecutive count = 1.  Expect
        // no phase fault error to be logged.
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=1"))
            .Times(3);
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=2"))
            .Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging, logPhaseFault).Times(0);

        // Execute PhaseFaultDetection 6 times
        for (int i = 1; i <= 6; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }

    // Test where N fault detected twice in a row
    {
        // Create action that will log an N phase fault in ActionEnvironment
        auto action = std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n);

        // Create PhaseFaultDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        PhaseFaultDetection detection{std::move(actions)};

        // Create mock services with the following expectations:
        // - 2 error messages in journal for N phase fault detected
        // - 0 error messages in journal for N+1 phase fault detected
        // - 1 N phase fault error logged
        // - 0 N+1 phase fault errors logged
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=2"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=1"))
            .Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        std::map<std::string, std::string> additionalData{};
        EXPECT_CALL(errorLogging,
                    logPhaseFault(Entry::Level::Warning, Ref(journal),
                                  PhaseFaultType::n, regulator->getFRU(),
                                  additionalData))
            .Times(1);
        EXPECT_CALL(errorLogging,
                    logPhaseFault(_, _, PhaseFaultType::n_plus_1, _, _))
            .Times(0);

        // Execute PhaseFaultDetection 5 times
        for (int i = 1; i <= 5; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }

    // Test where N+1 fault detected twice in a row
    {
        // Create action that will log an N+1 phase fault in ActionEnvironment
        auto action =
            std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n_plus_1);

        // Create PhaseFaultDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        PhaseFaultDetection detection{std::move(actions)};

        // Create mock services with the following expectations:
        // - 2 error messages in journal for N+1 phase fault detected
        // - 0 error messages in journal for N phase fault detected
        // - 1 N+1 phase fault error logged
        // - 0 N phase fault errors logged
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=2"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=1"))
            .Times(0);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        std::map<std::string, std::string> additionalData{};
        EXPECT_CALL(errorLogging,
                    logPhaseFault(Entry::Level::Informational, Ref(journal),
                                  PhaseFaultType::n_plus_1, regulator->getFRU(),
                                  additionalData))
            .Times(1);
        EXPECT_CALL(errorLogging, logPhaseFault(_, _, PhaseFaultType::n, _, _))
            .Times(0);

        // Execute PhaseFaultDetection 5 times
        for (int i = 1; i <= 5; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }

    // Test where both faults detected twice in a row
    {
        std::vector<std::unique_ptr<Action>> actions{};

        // Create action that will log an N+1 phase fault in ActionEnvironment
        actions.push_back(
            std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n_plus_1));

        // Create action that will log an N phase fault in ActionEnvironment
        actions.push_back(
            std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n));

        // Create PhaseFaultDetection
        PhaseFaultDetection detection{std::move(actions)};

        // Create mock services with the following expectations:
        // - 2 error messages in journal for N+1 phase fault detected
        // - 2 error messages in journal for N phase fault detected
        // - 1 N+1 phase fault error logged
        // - 1 N phase fault error logged
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n+1 phase fault detected in regulator vdd1: count=2"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=1"))
            .Times(1);
        EXPECT_CALL(
            journal,
            logError("n phase fault detected in regulator vdd1: count=2"))
            .Times(1);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        std::map<std::string, std::string> additionalData{};
        EXPECT_CALL(errorLogging,
                    logPhaseFault(Entry::Level::Informational, Ref(journal),
                                  PhaseFaultType::n_plus_1, regulator->getFRU(),
                                  additionalData))
            .Times(1);
        EXPECT_CALL(errorLogging,
                    logPhaseFault(Entry::Level::Warning, Ref(journal),
                                  PhaseFaultType::n, regulator->getFRU(),
                                  additionalData))
            .Times(1);

        // Execute PhaseFaultDetection 5 times
        for (int i = 1; i <= 5; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }

    // Test where additional error data is captured
    {
        std::vector<std::unique_ptr<Action>> actions{};

        // Create action that will capture 1 byte from register 0x0F
        actions.push_back(std::make_unique<I2CCaptureBytesAction>(0x0F, 1));

        // Create action that will capture 2 bytes from register 0x21
        actions.push_back(std::make_unique<I2CCaptureBytesAction>(0x21, 2));

        // Create action that will log an N phase fault in ActionEnvironment
        actions.push_back(
            std::make_unique<LogPhaseFaultAction>(PhaseFaultType::n));

        // Create PhaseFaultDetection
        PhaseFaultDetection detection{std::move(actions)};

        // Set expectations for regulator I2C interface:
        // - isOpen() will return true
        // - reading 1 byte from register 0x0F will return 0xDA
        // - reading 2 bytes from register 0x21 will return [ 0x56, 0x14 ]
        EXPECT_CALL(*regI2CInterface, isOpen).WillRepeatedly(Return(true));
        uint8_t register0FValues[] = {0xDA};
        EXPECT_CALL(*regI2CInterface,
                    read(0x0F, TypedEq<uint8_t&>(1), NotNull(),
                         i2c::I2CInterface::Mode::I2C))
            .Times(5)
            .WillRepeatedly(
                SetArrayArgument<2>(register0FValues, register0FValues + 1));
        uint8_t register21Values[] = {0x56, 0x14};
        EXPECT_CALL(*regI2CInterface,
                    read(0x21, TypedEq<uint8_t&>(2), NotNull(),
                         i2c::I2CInterface::Mode::I2C))
            .Times(5)
            .WillRepeatedly(
                SetArrayArgument<2>(register21Values, register21Values + 2));

        // Create mock services with the following expectations:
        // - 2 error messages in journal for N phase fault detected
        // - 1 N phase fault error logged with additional data
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        EXPECT_CALL(journal, logError(A<const std::string&>())).Times(2);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        std::map<std::string, std::string> additionalData{
            {"vdd1_register_0xF", "[ 0xDA ]"},
            {"vdd1_register_0x21", "[ 0x56, 0x14 ]"}};
        EXPECT_CALL(errorLogging,
                    logPhaseFault(Entry::Level::Warning, Ref(journal),
                                  PhaseFaultType::n, regulator->getFRU(),
                                  additionalData))
            .Times(1);

        // Execute PhaseFaultDetection 5 times
        for (int i = 1; i <= 5; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }

    // Test where fails: Exception thrown by actions
    {
        // Create I2CCompareBitAction that will use the I2CInterface
        auto action = std::make_unique<I2CCompareBitAction>(0x7C, 2, 0);

        // Create PhaseFaultDetection
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::move(action));
        PhaseFaultDetection detection{std::move(actions)};

        // Set expectations for regulator I2C interface:
        // - isOpen() will return true
        // - reading 1 byte from register 0x7C will throw an I2CException
        EXPECT_CALL(*regI2CInterface, isOpen).WillRepeatedly(Return(true));
        EXPECT_CALL(*regI2CInterface, read(0x7C, A<uint8_t&>()))
            .Times(5)
            .WillRepeatedly(Throw(
                i2c::I2CException{"Failed to read byte", "/dev/i2c-1", 0x70}));

        // Create mock services with the following expectations:
        // - 3 error messages in journal for exception
        // - 3 error messages in journal for inability to detect phase faults
        // - 1 I2C error logged
        MockServices services{};
        MockJournal& journal = services.getMockJournal();
        std::vector<std::string> exceptionMessages{
            "I2CException: Failed to read byte: bus /dev/i2c-1, addr 0x70",
            "ActionError: i2c_compare_bit: { register: 0x7C, position: 2, "
            "value: 0 }"};
        EXPECT_CALL(journal, logError(exceptionMessages)).Times(3);
        EXPECT_CALL(journal,
                    logError("Unable to detect phase faults in regulator vdd1"))
            .Times(3);
        MockErrorLogging& errorLogging = services.getMockErrorLogging();
        EXPECT_CALL(errorLogging,
                    logI2CError(Entry::Level::Warning, Ref(journal),
                                "/dev/i2c-1", 0x70, 0))
            .Times(1);

        // Execute PhaseFaultDetection 5 times
        for (int i = 1; i <= 5; ++i)
        {
            detection.execute(services, *system, *chassis, *regulator);
        }
    }
}

TEST_F(PhaseFaultDetectionTests, GetActions)
{
    std::vector<std::unique_ptr<Action>> actions{};

    MockAction* action1 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action1});

    MockAction* action2 = new MockAction{};
    actions.push_back(std::unique_ptr<MockAction>{action2});

    PhaseFaultDetection detection{std::move(actions)};
    EXPECT_EQ(detection.getActions().size(), 2);
    EXPECT_EQ(detection.getActions()[0].get(), action1);
    EXPECT_EQ(detection.getActions()[1].get(), action2);
}

TEST_F(PhaseFaultDetectionTests, GetDeviceID)
{
    // Test where device ID not specified
    {
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        PhaseFaultDetection detection{std::move(actions)};
        EXPECT_EQ(detection.getDeviceID(), "");
    }

    // Test where device ID not specified
    {
        std::vector<std::unique_ptr<Action>> actions{};
        actions.push_back(std::make_unique<MockAction>());

        PhaseFaultDetection detection{std::move(actions), "ioexp1"};
        EXPECT_EQ(detection.getDeviceID(), "ioexp1");
    }
}
