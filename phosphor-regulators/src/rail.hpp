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

namespace phosphor::power::regulators
{

/**
 * @class Rail
 *
 * A voltage rail produced by a voltage regulator.
 *
 * Voltage regulators produce one or more rails.  Each rail typically provides a
 * different output voltage level, such as 1.1V.
 */
class Rail
{
  public:
    // Specify which compiler-generated methods we want
    Rail() = delete;
    Rail(const Rail&) = delete;
    Rail(Rail&&) = delete;
    Rail& operator=(const Rail&) = delete;
    Rail& operator=(Rail&&) = delete;
    ~Rail() = default;

    /**
     * Constructor.
     *
     * @param id unique rail ID
     */
    explicit Rail(const std::string& id) : id{id}
    {
    }

    /**
     * Returns the unique ID of this rail.
     *
     * @return rail ID
     */
    const std::string& getID() const
    {
        return id;
    }

  private:
    /**
     * Unique ID of this rail.
     */
    const std::string id{};
};

} // namespace phosphor::power::regulators
