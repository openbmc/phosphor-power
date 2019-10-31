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

#include "device.hpp"
#include "id_map.hpp"
#include "rule.hpp"

#include <cstddef> // for size_t
#include <stdexcept>
#include <string>

namespace phosphor
{
namespace power
{
namespace regulators
{

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
     */
    explicit ActionEnvironment(const IDMap& idMap,
                               const std::string& deviceID) :
        idMap{idMap},
        deviceID{deviceID}
    {
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
     * Returns the current volts value.
     *
     * Call hasVolts() first to check whether a volts value has been set.
     *
     * Throws logic_error if no volts value has been set.
     *
     * @return current volts value
     */
    double getVolts() const
    {
        if (!hasVoltsValue)
        {
            throw std::logic_error{"No volts value has been set."};
        }
        return volts;
    }

    /**
     * Returns whether a volts value has been set.
     *
     * @return true if a volts value has been set, false otherwise
     */
    bool hasVolts() const
    {
        return hasVoltsValue;
    }

    /**
     * Increments the rule call stack depth by one.
     *
     * Should be used when a rule is called.
     *
     * Throws runtime_error if the new depth exceeds maxRuleDepth.  This
     * indicates that infinite recursion has probably occurred (rule A -> rule B
     * -> rule A).
     */
    void incrementRuleDepth()
    {
        if (ruleDepth >= maxRuleDepth)
        {
            throw std::runtime_error("Maximum rule depth exceeded.");
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
        hasVoltsValue = true;
    }

  private:
    /**
     * Mapping from string IDs to the associated Device and Rule objects.
     */
    const IDMap& idMap{};

    /**
     * Current device ID.
     */
    std::string deviceID{};

    /**
     * Indicates whether a volts value has been set.
     */
    bool hasVoltsValue{false};

    /**
     * Current volts value (if set).
     */
    double volts{0};

    /**
     * Rule call stack depth.
     */
    size_t ruleDepth{0};
};

} // namespace regulators
} // namespace power
} // namespace phosphor
