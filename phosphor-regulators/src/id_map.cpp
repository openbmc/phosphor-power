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

#include "id_map.hpp"

#include "device.hpp"
#include "rail.hpp"
#include "rule.hpp"

namespace phosphor::power::regulators
{

void IDMap::addDevice(Device& device)
{
    const std::string& id = device.getID();
    if (deviceMap.count(id) != 0)
    {
        throw std::invalid_argument{
            "Unable to add device: Duplicate ID \"" + id + '"'};
    }
    deviceMap[id] = &device;
}

void IDMap::addRail(Rail& rail)
{
    const std::string& id = rail.getID();
    if (railMap.count(id) != 0)
    {
        throw std::invalid_argument{
            "Unable to add rail: Duplicate ID \"" + id + '"'};
    }
    railMap[id] = &rail;
}

void IDMap::addRule(Rule& rule)
{
    const std::string& id = rule.getID();
    if (ruleMap.count(id) != 0)
    {
        throw std::invalid_argument{
            "Unable to add rule: Duplicate ID \"" + id + '"'};
    }
    ruleMap[id] = &rule;
}

} // namespace phosphor::power::regulators
