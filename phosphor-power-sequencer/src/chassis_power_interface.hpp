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

#include "power_interface.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace phosphor::power::sequencer
{

// Forward declare Chassis class to avoid circular dependency
class Chassis;

using PowerObject = sdbusplus::server::object_t<PowerInterface>;

/**
 * @class ChassisPowerInterface
 *
 * This class implements the org.openbmc.control.Power D-Bus interface for a
 * chassis.
 */
class ChassisPowerInterface : public PowerObject
{
  public:
    ChassisPowerInterface() = delete;
    ChassisPowerInterface(const ChassisPowerInterface&) = delete;
    ChassisPowerInterface& operator=(const ChassisPowerInterface&) = delete;
    ChassisPowerInterface(ChassisPowerInterface&&) = delete;
    ChassisPowerInterface& operator=(ChassisPowerInterface&&) = delete;
    virtual ~ChassisPowerInterface() = default;

    /**
     * Constructor to put interface onto the specified D-Bus path.

     * @param bus D-Bus bus object
     * @param path D-Bus object path
     * @param state initial value for 'state' property
     * @param pgood initial value for 'pgood' property
     * @param pgoodTimeout initial value for 'pgood_timeout' property
     * @param chassis chassis controlled by this interface
     */
    ChassisPowerInterface(sdbusplus::bus_t& bus, const char* path, int state,
                          int pgood, int pgoodTimeout, Chassis& chassis);

    /** @copydoc PowerInterface::setPowerState() */
    void setPowerState(int newState) override;

    /** @copydoc PowerInterface::setPowerGoodTimeout() */
    void setPowerGoodTimeout(int newTimeout) override;

    /** @copydoc PowerInterface::setPowerSupplyError() */
    void setPowerSupplyError(const std::string& error) override;

  private:
    /**
     * Chassis controlled by this interface.
     */
    Chassis& chassis;
};

} // namespace phosphor::power::sequencer
