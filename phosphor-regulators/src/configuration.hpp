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

#include "action.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class Configuration
 *
 * Configuration changes that should be applied to a device or regulator rail.
 * These changes usually override hardware default settings.
 *
 * The most common configuration change is setting the output voltage for a
 * regulator rail.  Other examples include modifying pgood thresholds and
 * overcurrent settings.
 *
 * The configuration changes are applied during the boot before regulators are
 * enabled.
 *
 * The configuration changes are applied by executing one or more actions.
 *
 * An output voltage value can be specified if necessary.  The value will be
 * stored in the ActionEnvironment when the actions are executed.  Actions that
 * require a volts value, such as PMBusWriteVoutCommandAction, can obtain it
 * from the ActionEnvironment.
 */
class Configuration
{
  public:
    // Specify which compiler-generated methods we want
    Configuration() = delete;
    Configuration(const Configuration&) = delete;
    Configuration(Configuration&&) = delete;
    Configuration& operator=(const Configuration&) = delete;
    Configuration& operator=(Configuration&&) = delete;
    ~Configuration() = default;

    /**
     * Constructor.
     *
     * @param volts optional output voltage value
     * @param actions actions that configure the device/rail
     */
    explicit Configuration(std::optional<double> volts,
                           std::vector<std::unique_ptr<Action>> actions) :
        volts{volts},
        actions{std::move(actions)}
    {
    }

    /**
     * Executes the actions to configure the device/rail.
     */
    void execute()
    {
        // TODO: Create ActionEnvironment, set volts value if specified, execute
        // actions, catch and handle any exceptions
    }

    /**
     * Returns the actions that configure the device/rail.
     *
     * @return actions
     */
    const std::vector<std::unique_ptr<Action>>& getActions() const
    {
        return actions;
    }

    /**
     * Returns the optional output voltage value.
     *
     * @return optional volts value
     */
    std::optional<double> getVolts() const
    {
        return volts;
    }

  private:
    /**
     * Optional output voltage value.
     */
    const std::optional<double> volts{};

    /**
     * Actions that configure the device/rail.
     */
    std::vector<std::unique_ptr<Action>> actions{};
};

} // namespace phosphor::power::regulators
