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
#include "services.hpp"

#include <sdbusplus/bus.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace phosphor::power::chassis
{

using ChassisStatusMonitor = phosphor::power::util::ChassisStatusMonitor;

/**
 * @class System
 *
 * The computer system being controlled and monitored by the BMC.
 *
 * The system contains one or more chassis.
 */
class System
{
  public:
    System() = delete;
    System(const System&) = delete;
    System(System&&) = delete;
    System& operator=(const System&) = delete;
    System& operator=(System&&) = delete;
    ~System() = default;

    /**
     * Constructor.
     *
     * @param chassis  Chassis in the system
     * @param services Platform services provider
     */
    System(std::vector<std::unique_ptr<Chassis>> chassis, Services& services) :
        chassis{std::move(chassis)}, services{services}
    {}

    /**
     * Returns the chassis in the system.
     *
     * @return chassis
     */
    const std::vector<std::unique_ptr<Chassis>>& getChassis() const
    {
        return chassis;
    }

    /**
     * Initializes each chassis power system inputs status to be good.
     *
     * @param bus D-Bus bus object
     */
    void initializePowerSystemInputs(sdbusplus::bus_t& bus);

    /**
     * Initializes chassis presence to be true on the primary BMC.
     */
    void initializePresence();

    /**
     * Initializes status monitors for the system and all chassis.
     */
    void initializeStatusMonitors();

    /**
     * Clears the error history in all chassis.
     *
     * This should be called when the system reboots.
     */
    void clearErrorHistory();

    /**
     * Monitors the status of all chassis.
     */
    void monitor();

  private:
    /**
     * Chassis in the system.
     */
    std::vector<std::unique_ptr<Chassis>> chassis{};

    /**
     * System-level status monitor.
     *
     * Monitors the entire system properties. Shared with all chassis.
     */
    std::shared_ptr<ChassisStatusMonitor> systemMonitor{};

    /**
     * Flag for if chassis presence has been initialized.
     */
    bool initializedPresence = false;

    /**
     * System services (D-Bus, GPIO, etc.).
     */
    Services& services;
};

} // namespace phosphor::power::chassis
