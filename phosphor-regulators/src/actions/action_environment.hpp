/**
 * Copyright © 2019 IBM Corporation
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

#include "id_map.hpp"
#include "phase_fault.hpp"
#include "services.hpp"

#include <cstddef> // for size_t
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace phosphor::power::regulators
{

// Forward declarations to avoid circular dependencies
class Device;
class Rule;

/**
 * @class ActionEnvironment
 *
 * The current environment when executing actions.
 *
 * The ActionEnvironment contains the following information:
 *   - current device ID
 *   - current volts value (if any)
 *   - mapping from device and rule IDs to the corresponding objects
 *   - rule call stack depth (to detect infinite recursion)
 *   - reference to system services
 *   - faults detected by actions (if any)
 *   - additional error data captured by actions (if any)
 */
class ActionEnvironment
{
  public:
    // Specify which compiler-generated methods we want
    ActionEnvironment() = delete;
    ActionEnvironment(const ActionEnvironment&) = delete;
    ActionEnvironment(ActionEnvironment&&) = delete;
    ActionEnvironment& operator=(const ActionEnvironment&) = delete;
    ActionEnvironment& operator=(ActionEnvironment&&) = delete;
    ~ActionEnvironment() = default;

    /**
     * Maximum rule call stack depth.  Used to detect infinite recursion.
     */
    static constexpr size_t maxRuleDepth{30};

    /**
     * Constructor.
     *
     * @param idMap mapping from IDs to the associated Device/Rule objects
     * @param deviceID current device ID
     * @param services system services like error logging and the journal
     */
    explicit ActionEnvironment(const IDMap& idMap, const std::string& deviceID,
                               Services& services) :
        idMap{idMap}, deviceID{deviceID}, services{services}
    {}

    /**
     * Adds the specified key/value pair to the map of additional error data
     * that has been captured.
     *
     * This data provides more information about an error and will be stored in
     * the error log.
     *
     * @param key key name
     * @param value value expressed as a string
     */
    void addAdditionalErrorData(const std::string& key,
                                const std::string& value)
    {
        additionalErrorData.emplace(key, value);
    }

    /**
     * Adds the specified phase fault to the set of faults that have been
     * detected.
     *
     * @param type phase fault type
     */
    void addPhaseFault(PhaseFaultType type)
    {
        phaseFaults.emplace(type);
    }

    /**
     * Decrements the rule call stack depth by one.
     *
     * Should be used when a call to a rule returns.  Does nothing if depth is
     * already 0.
     */
    void decrementRuleDepth()
    {
        if (ruleDepth > 0)
        {
            --ruleDepth;
        }
    }

    /**
     * Returns the additional error data that has been captured (if any).
     *
     * @return additional error data
     */
    const std::map<std::string, std::string>& getAdditionalErrorData() const
    {
        return additionalErrorData;
    }

    /**
     * Returns the device with the current device ID.
     *
     * Throws invalid_argument if no device is found with current ID.
     *
     * @return device with current device ID
     */
    Device& getDevice() const
    {
        return idMap.getDevice(deviceID);
    }

    /**
     * Returns the current device ID.
     *
     * @return current device ID
     */
    const std::string& getDeviceID() const
    {
        return deviceID;
    }

    /**
     * Returns the set of phase faults that have been detected (if any).
     *
     * @return phase faults detected
     */
    const std::set<PhaseFaultType>& getPhaseFaults() const
    {
        return phaseFaults;
    }

    /**
     * Returns the rule with the specified ID.
     *
     * Throws invalid_argument if no rule is found with specified ID.
     *
     * @param id rule ID
     * @return rule with specified ID
     */
    Rule& getRule(const std::string& id) const
    {
        return idMap.getRule(id);
    }

    /**
     * Returns the current rule call stack depth.
     *
     * The depth is 0 if no rules have been called.
     *
     * @return rule call stack depth
     */
    size_t getRuleDepth() const
    {
        return ruleDepth;
    }

    /**
     * Returns the services in this action environment.
     *
     * @return system services
     */
    Services& getServices() const
    {
        return services;
    }

    /**
     * Returns the current volts value, if set.
     *
     * @return current volts value
     */
    std::optional<double> getVolts() const
    {
        return volts;
    }

    /**
     * Increments the rule call stack depth by one.
     *
     * Should be used when a rule is called.
     *
     * Throws runtime_error if the new depth exceeds maxRuleDepth.  This
     * indicates that infinite recursion has probably occurred (rule A -> rule B
     * -> rule A).
     *
     * @param ruleID ID of the rule that is being called
     */
    void incrementRuleDepth(const std::string& ruleID)
    {
        if (ruleDepth >= maxRuleDepth)
        {
            throw std::runtime_error("Maximum rule depth exceeded by rule " +
                                     ruleID + '.');
        }
        ++ruleDepth;
    }

    /**
     * Sets the current device ID.
     *
     * @param id device ID
     */
    void setDeviceID(const std::string& id)
    {
        deviceID = id;
    }

    /**
     * Sets the current volts value.
     *
     * @param volts new volts value.
     */
    void setVolts(double volts)
    {
        this->volts = volts;
    }

  private:
    /**
     * Mapping from string IDs to the associated Device and Rule objects.
     */
    const IDMap& idMap;

    /**
     * Current device ID.
     */
    std::string deviceID{};

    /**
     * System services like error logging and the journal.
     */
    Services& services;

    /**
     * Current volts value (if set).
     */
    std::optional<double> volts{};

    /**
     * Rule call stack depth.
     */
    size_t ruleDepth{0};

    /**
     * Redundant phase faults that have been detected.
     */
    std::set<PhaseFaultType> phaseFaults{};

    /**
     * Additional error data that has been captured.
     */
    std::map<std::string, std::string> additionalErrorData{};
};

} // namespace phosphor::power::regulators
