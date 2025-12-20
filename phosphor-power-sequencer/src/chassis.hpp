/**
 * Copyright © 2025 IBM Corporation
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

#include "chassis_status_monitor.hpp"
#include "power_sequencer_device.hpp"

#include <stddef.h> // for size_t

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

using ChassisStatusMonitorOptions =
    phosphor::power::util::ChassisStatusMonitorOptions;

/**
 * @class Chassis
 *
 * A chassis within the system.
 *
 * Chassis are typically a physical enclosure that contains system components
 * such as CPUs, fans, power supplies, and PCIe cards. A chassis can be
 * stand-alone, such as a tower or desktop. A chassis can also be designed to be
 * mounted in an equipment rack.
 */
class Chassis
{
  public:
    Chassis() = delete;
    Chassis(const Chassis&) = delete;
    Chassis(Chassis&&) = delete;
    Chassis& operator=(const Chassis&) = delete;
    Chassis& operator=(Chassis&&) = delete;
    ~Chassis() = default;

    /**
     * Constructor.
     *
     * @param number Chassis number within the system. Must be >= 1.
     * @param inventoryPath D-Bus inventory path of the chassis
     * @param powerSequencers Power sequencer devices within the chassis
     * @param monitorOptions Types of chassis status monitoring to perform.
     */
    explicit Chassis(
        size_t number, const std::string& inventoryPath,
        std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers,
        const ChassisStatusMonitorOptions& monitorOptions) :
        number{number}, inventoryPath{inventoryPath},
        powerSequencers{std::move(powerSequencers)},
        monitorOptions{monitorOptions}
    {}

    /**
     * Returns the chassis number within the system.
     *
     * @return chassis number
     */
    size_t getNumber() const
    {
        return number;
    }

    /**
     * Returns the D-Bus inventory path of the chassis.
     *
     * @return inventory path
     */
    const std::string& getInventoryPath() const
    {
        return inventoryPath;
    }

    /**
     * Returns the power sequencer devices within the chassis.
     *
     * @return power sequencer devices
     */
    const std::vector<std::unique_ptr<PowerSequencerDevice>>&
        getPowerSequencers() const
    {
        return powerSequencers;
    }

    /**
     * Returns the types of chassis status monitoring to perform.
     *
     * @return chassis status monitoring options
     */
    const ChassisStatusMonitorOptions& getMonitorOptions() const
    {
        return monitorOptions;
    }

  private:
    /**
     * Chassis number within the system.
     *
     * Chassis numbers start at 1 because chassis 0 represents the entire
     * system.
     */
    size_t number;

    /**
     * D-Bus inventory path of the chassis.
     */
    std::string inventoryPath{};

    /**
     * Power sequencer devices within the chassis.
     */
    std::vector<std::unique_ptr<PowerSequencerDevice>> powerSequencers{};

    /**
     * Types of chassis status monitoring to perform.
     */
    ChassisStatusMonitorOptions monitorOptions{};
};

} // namespace phosphor::power::sequencer
