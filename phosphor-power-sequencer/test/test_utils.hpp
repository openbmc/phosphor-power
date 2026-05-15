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
#pragma once

#include "chassis.hpp"
#include "mock_chassis_status_monitor.hpp"
#include "mock_device.hpp"
#include "rail.hpp"

#include <memory>
#include <string>

namespace phosphor::power::sequencer::test_utils
{

using namespace phosphor::power::util;

/**
 * Creates a Rail object that checks for a pgood fault using a GPIO.
 *
 * @param name Unique name for the rail
 * @param gpioLine GPIO line to read to determine the pgood status of the rail
 * @return Rail object
 */
std::unique_ptr<Rail> createRail(const std::string& name,
                                 unsigned int gpioLine);

/**
 * Returns the MockChassisStatusMonitor within a Chassis.
 *
 * Assumes that initializeMonitoring() has been called with a MockServices
 * parameter.
 *
 * Throws an exception if initializeMonitoring() has not been called.
 *
 * @param chassis Chassis object
 * @return MockChassisStatusMonitor reference
 */
MockChassisStatusMonitor& getMockStatusMonitor(Chassis& chassis);

/**
 * Returns a reference to the MockDevice at the specified index within the
 * chassis's vector of power sequencer devices.
 *
 * @param chassis Chassis object
 * @param i Index of power sequencer device
 * @return MockDevice reference
 */
MockDevice& getMockDevice(Chassis& chassis, int i);

/**
 * Sets up the MockChassisStatusMonitor to repeatedly return good status for all
 * D-Bus properties.
 *
 * @param chassis Chassis object
 */
void setChassisStatusToGood(Chassis& chassis);

/**
 * Sets up the MockChassisStatusMonitor to repeatedly return good status for all
 * D-Bus properties except isAvailable.
 *
 * @param chassis Chassis object
 */
void setChassisStatusToGoodExceptIsAvailable(Chassis& chassis);

/**
 * Sets up the MockChassisStatusMonitor to repeatedly return good status for all
 * D-Bus properties except isEnabled.
 *
 * @param chassis Chassis object
 */
void setChassisStatusToGoodExceptIsEnabled(Chassis& chassis);

/**
 * Sets up the MockChassisStatusMonitor to repeatedly return good status for all
 * D-Bus properties except isInputPowerGood.
 *
 * @param chassis Chassis object
 */
void setChassisStatusToGoodExceptIsInputPowerGood(Chassis& chassis);

/**
 * Sets up the MockChassisStatusMonitor to repeatedly return good status for all
 * D-Bus properties except isPresent.
 *
 * @param chassis Chassis object
 */
void setChassisStatusToGoodExceptIsPresent(Chassis& chassis);

} // namespace phosphor::power::sequencer::test_utils
