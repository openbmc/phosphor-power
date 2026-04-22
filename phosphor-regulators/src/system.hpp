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
#pragma once

#include "chassis.hpp"
#include "id_map.hpp"
#include "rule.hpp"
#include "services.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class System
 *
 * The computer system being controlled and monitored by the BMC.
 *
 * The system contains one or more chassis. Chassis are typically physical
 * enclosures that contain system components such as CPUs, fans, power
 * supplies, and PCIe cards.
 */
class System
{
  public:
    // Specify which compiler-generated methods we want
    System() = delete;
    System(const System&) = delete;
    System(System&&) = delete;
    System& operator=(const System&) = delete;
    System& operator=(System&&) = delete;
    ~System() = default;

    /**
     * Constructor.
     *
     * @param rules rules used to monitor and control regulators in the system
     * @param chassis chassis in the system
     */
    explicit System(std::vector<std::unique_ptr<Rule>> rules,
                    std::vector<std::unique_ptr<Chassis>> chassis) :
        rules{std::move(rules)}, chassis{std::move(chassis)}
    {
        buildIDMap();
    }

    /**
     * Clear any cached data about hardware devices.
     */
    void clearCache();

    /**
     * Clears all error history.
     *
     * All data on previously logged errors will be deleted.  If errors occur
     * again in the future they will be logged again.
     *
     * This method is normally called when the system is being powered on.
     */
    void clearErrorHistory();

    /**
     * Close the regulator devices in the system.
     *
     * @param services system services like error logging and the journal
     */
    void closeDevices(Services& services);

    /**
     * Configure the regulator devices in the system.
     *
     * This method should be called during the boot before regulators are
     * enabled.
     *
     * @param services system services like error logging and the journal
     */
    void configure(Services& services);

    /**
     * Detect redundant phase faults in regulator devices in the system.
     *
     * This method should be called repeatedly based on a timer.
     *
     * @param services system services like error logging and the journal
     */
    void detectPhaseFaults(Services& services);

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
     * Returns the IDMap for the system.
     *
     * The IDMap provides a mapping from string IDs to the associated Device,
     * Rail, and Rule objects.
     *
     * @return IDMap
     */
    const IDMap& getIDMap() const
    {
        return idMap;
    }

    /**
     * Returns the rules used to monitor and control regulators in the system.
     *
     * @return rules
     */
    const std::vector<std::unique_ptr<Rule>>& getRules() const
    {
        return rules;
    }

    /**
     * Initializes system monitoring.
     *
     * This method must be called before any methods that return or check the
     * system status.
     *
     * Normally this method is only called once. However, it can be called
     * multiple times if required, such as for automated testing.
     *
     * @param services system services like error logging and the journal
     */
    void initializeMonitoring(Services& services);

    /**
     * Monitors the sensors for the voltage rails produced by this system, if
     * any.
     *
     * This method should be called repeatedly based on a timer.
     *
     * @param services system services like error logging and the journal
     */
    void monitorSensors(Services& services);

  private:
    /**
     * Builds the IDMap for the system.
     *
     * Adds the Device, Rail, and Rule objects in the system to the map.
     */
    void buildIDMap();

    /**
     * Verifies that system monitoring has been initialized.
     *
     * Throws an exception if monitoring has not been initialized.
     */
    void verifyMonitoringInitialized()
    {
        if (!isMonitoringInitialized)
        {
            throw std::runtime_error{
                "System monitoring has not been initialized"};
        }
    }

    /**
     * Rules used to monitor and control regulators in the system.
     */
    std::vector<std::unique_ptr<Rule>> rules{};

    /**
     * Chassis in the system.
     */
    std::vector<std::unique_ptr<Chassis>> chassis{};

    /**
     * Mapping from string IDs to the associated Device, Rail, and Rule objects.
     */
    IDMap idMap{};

    /**
     * Indicates whether system monitoring has been initialized.
     */
    bool isMonitoringInitialized{false};
};

} // namespace phosphor::power::regulators
