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

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/State/Decorator/PowerSystemInputs/server.hpp>

namespace phosphor::power::chassis
{

using PowerSystemInputs = sdbusplus::server::xyz::openbmc_project::state::
    decorator::PowerSystemInputs;
using PowerSystemInputsObject = sdbusplus::server::object_t<PowerSystemInputs>;

/**
 * @class ChassisPowerSystemInterface
 *
 * This class implements the
 * xyz.openbmc_project.State.Decorator.PowerSystemInputs D-Bus interface for a
 * chassis.
 */
class ChassisPowerSystemInterface : public PowerSystemInputsObject
{
  public:
    ChassisPowerSystemInterface() = delete;
    ChassisPowerSystemInterface(const ChassisPowerSystemInterface&) = delete;
    ChassisPowerSystemInterface& operator=(const ChassisPowerSystemInterface&) =
        delete;
    ChassisPowerSystemInterface(ChassisPowerSystemInterface&&) = delete;
    ChassisPowerSystemInterface& operator=(ChassisPowerSystemInterface&&) =
        delete;
    virtual ~ChassisPowerSystemInterface() = default;

    /**
     * Constructor to put interface onto the specified D-Bus path.
     *
     * @param bus D-Bus bus object
     * @param path D-Bus object path
     * @param initialStatus initial value for 'status' property
     * @param skipSignal indicates whether to skip sending D-Bus signal due to
     * property change
     */
    ChassisPowerSystemInterface(sdbusplus::bus_t& bus, const char* path,
                                PowerSystemInputs::Status initialStatus,
                                bool skipSignal);
};

} // namespace phosphor::power::chassis
