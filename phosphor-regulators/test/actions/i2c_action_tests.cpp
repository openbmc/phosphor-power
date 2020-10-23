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
#include "action_environment.hpp"
#include "device.hpp"
#include "i2c_action.hpp"
#include "i2c_interface.hpp"
#include "id_map.hpp"
#include "mock_services.hpp"
#include "mocked_i2c_interface.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

using ::testing::Return;
using ::testing::Throw;

/**
 * Define a test implementation of the I2CAction abstract base class.
 */
class I2CActionImpl : public I2CAction
{
  public:
    virtual bool execute(ActionEnvironment& /* environment */) override
    {
        return true;
    }

    virtual std::string toString() const override
    {
        return "i2c_action_impl: {}";
    }

    // Make test a friend so it can access protected getI2CInterface() method
    FRIEND_TEST(I2CActionTests, GetI2CInterface);
};

TEST(I2CActionTests, GetI2CInterface)
{
    // Test where works: device was not open
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*i2cInterface, open).Times(1);

        // Create Device, IDMap, MockServices, ActionEnvironment, and I2CAction
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};
        I2CActionImpl action{};

        // Get I2CInterface.  Should succeed without an exception.
        action.getI2CInterface(env);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where works: device was already open
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*i2cInterface, open).Times(0);

        // Create Device, IDMap, MockServices, ActionEnvironment, and I2CAction
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};
        I2CActionImpl action{};

        // Get I2CInterface.  Should succeed without an exception.
        action.getI2CInterface(env);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: getting current device fails
    try
    {
        // Create IDMap, MockServices, ActionEnvironment, and I2CAction
        IDMap idMap{};
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};
        I2CActionImpl action{};

        // Get I2CInterface.  Should throw an exception since "reg1" is not a
        // valid device in the IDMap.
        action.getI2CInterface(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Unable to find device with ID \"reg1\"");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where fails: opening interface fails
    try
    {
        // Create mock I2CInterface
        std::unique_ptr<i2c::MockedI2CInterface> i2cInterface =
            std::make_unique<i2c::MockedI2CInterface>();
        EXPECT_CALL(*i2cInterface, isOpen).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*i2cInterface, open)
            .Times(1)
            .WillOnce(
                Throw(i2c::I2CException{"Failed to open", "/dev/i2c-1", 0x70}));

        // Create Device, IDMap, ActionEnvironment, and I2CAction
        Device device{
            "reg1", true,
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1",
            std::move(i2cInterface)};
        IDMap idMap{};
        idMap.addDevice(device);
        MockServices services{};
        ActionEnvironment env{idMap, "reg1", services};
        I2CActionImpl action{};

        // Get I2CInterface.  Should throw an exception from the open() call.
        action.getI2CInterface(env);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const i2c::I2CException& e)
    {
        EXPECT_STREQ(e.what(),
                     "I2CException: Failed to open: bus /dev/i2c-1, addr 0x70");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}
