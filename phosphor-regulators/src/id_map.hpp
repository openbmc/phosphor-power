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
#include "rail.hpp"
#include "rule.hpp"

#include <map>
#include <stdexcept>
#include <string>

namespace phosphor
{
namespace power
{
namespace regulators
{

using std::invalid_argument;
using std::map;
using std::string;

/**
 * @class IDMap
 *
 * This class provides a mapping from string IDs to the associated Device, Rail,
 * and Rule objects.
 */
class IDMap
{
  public:
    /**
     * Adds the specified device to this IDMap.
     *
     * @param device device to add
     */
    void addDevice(Device& device)
    {
        deviceMap[device.getID()] = &device;
    }

    /**
     * Adds the specified rail to this IDMap.
     *
     * @param rail rail to add
     */
    void addRail(Rail& rail)
    {
        railMap[rail.getID()] = &rail;
    }

    /**
     * Adds the specified rule to this IDMap.
     *
     * @param rule rule to add
     */
    void addRule(Rule& rule)
    {
        ruleMap[rule.getID()] = &rule;
    }

    /**
     * Returns the device with the specified ID.
     *
     * Throws invalid_argument if no device is found with specified ID.
     *
     * @param id device ID
     * @return device with specified ID
     */
    Device& getDevice(const string& id) const
    {
        auto it = deviceMap.find(id);
        if (it == deviceMap.end())
        {
            throw invalid_argument{"Unable to find device with ID \"" + id +
                                   '"'};
        }
        return *(it->second);
    }

    /**
     * Returns the rail with the specified ID.
     *
     * Throws invalid_argument if no rail is found with specified ID.
     *
     * @param id rail ID
     * @return rail with specified ID
     */
    Rail& getRail(const string& id) const
    {
        auto it = railMap.find(id);
        if (it == railMap.end())
        {
            throw invalid_argument{"Unable to find rail with ID \"" + id + '"'};
        }
        return *(it->second);
    }

    /**
     * Returns the rule with the specified ID.
     *
     * Throws invalid_argument if no rule is found with specified ID.
     *
     * @param id rule ID
     * @return rule with specified ID
     */
    Rule& getRule(const string& id) const
    {
        auto it = ruleMap.find(id);
        if (it == ruleMap.end())
        {
            throw invalid_argument{"Unable to find rule with ID \"" + id + '"'};
        }
        return *(it->second);
    }

  private:
    /**
     * Map from device IDs to Device objects.  Does not own the objects.
     */
    map<string, Device*> deviceMap{};

    /**
     * Map from rail IDs to Rail objects.  Does not own the objects.
     */
    map<string, Rail*> railMap{};

    /**
     * Map from rule IDs to Rule objects.  Does not own the objects.
     */
    map<string, Rule*> ruleMap{};
};

} // namespace regulators
} // namespace power
} // namespace phosphor
