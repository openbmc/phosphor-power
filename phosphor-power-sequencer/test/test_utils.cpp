/**
 * Copyright © 2026 IBM Corporation
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

#include "test_utils.hpp"

#include <gmock/gmock.h>

using ::testing::Return;

namespace phosphor::power::sequencer::test_utils
{

std::unique_ptr<Rail> createRail(const std::string& name, unsigned int gpioLine)
{
    std::optional<std::string> presence{};
    std::optional<uint8_t> page{};
    bool isPowerSupplyRail{false};
    bool checkStatusVout{false};
    bool compareVoltageToLimit{false};
    bool activeLow{false};
    std::optional<PgoodGPIO> gpio{PgoodGPIO{gpioLine, activeLow}};
    return std::make_unique<Rail>(name, presence, page, isPowerSupplyRail,
                                  checkStatusVout, compareVoltageToLimit, gpio);
}

MockChassisStatusMonitor& getMockStatusMonitor(Chassis& chassis)
{
    return static_cast<MockChassisStatusMonitor&>(chassis.getStatusMonitor());
}

MockDevice& getMockDevice(Chassis& chassis, int i)
{
    return static_cast<MockDevice&>(*(chassis.getPowerSequencers()[i]));
}

void setChassisStatusToGood(Chassis& chassis)
{
    auto& monitor = getMockStatusMonitor(chassis);
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
}

void setChassisStatusToGoodExceptIsAvailable(Chassis& chassis)
{
    auto& monitor = getMockStatusMonitor(chassis);
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
}

void setChassisStatusToGoodExceptIsEnabled(Chassis& chassis)
{
    auto& monitor = getMockStatusMonitor(chassis);
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
}

void setChassisStatusToGoodExceptIsInputPowerGood(Chassis& chassis)
{
    auto& monitor = getMockStatusMonitor(chassis);
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isPresent).WillRepeatedly(Return(true));
}

void setChassisStatusToGoodExceptIsPresent(Chassis& chassis)
{
    auto& monitor = getMockStatusMonitor(chassis);
    EXPECT_CALL(monitor, isAvailable).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(monitor, isInputPowerGood).WillRepeatedly(Return(true));
}

} // namespace phosphor::power::sequencer::test_utils
