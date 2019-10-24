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

#include <string>

namespace phosphor
{
namespace power
{
namespace regulators
{

/**
 * @class Device
 *
 * A hardware device, such as a voltage regulator or I/O expander.
 */
class Device
{
  public:
    // Specify which compiler-generated methods we want
    Device() = delete;
    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;
    ~Device() = default;

    /**
     * Constructor.
     *
     * @param id unique device ID
     */
    Device(const std::string& id) : id{id}
    {
    }

    /**
     * Returns the unique ID of this device.
     *
     * @return device ID
     */
    const std::string& getId() const
    {
        return id;
    }

  private:
    /**
     * Unique ID of this device.
     */
    const std::string id{};
};

} // namespace regulators
} // namespace power
} // namespace phosphor
