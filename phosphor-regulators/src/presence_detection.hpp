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
#include "services.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace phosphor::power::regulators
{

// Forward declarations to avoid circular dependencies
class Chassis;
class Device;
class System;

/**
 * @class PresenceDetection
 *
 * Specifies how to detect whether a device is present.
 *
 * Some devices are only present in certain system configurations.  For example:
 * - A regulator is only present when a related processor or memory module is
 *   present.
 * - A system supports multiple storage backplane types, and the device only
 *   exists on one of the backplanes.
 *
 * Device presence is detected by executing actions, such as
 * ComparePresenceAction and CompareVPDAction.
 *
 * Device operations like configuration and sensor monitoring will only be
 * performed if the actions indicate the device is present.
 *
 * Device presence will only be detected once per boot of the system.  Presence
 * will be determined prior to the first device operation (such as
 * configuration).  When the system is re-booted, presence will be re-detected.
 * As a result, presence detection is not supported for devices that can be
 * removed or added (hot-plugged) while the system is booted and running.
 */
class PresenceDetection
{
  public:
    // Specify which compiler-generated methods we want
    PresenceDetection() = delete;
    PresenceDetection(const PresenceDetection&) = delete;
    PresenceDetection(PresenceDetection&&) = delete;
    PresenceDetection& operator=(const PresenceDetection&) = delete;
    PresenceDetection& operator=(PresenceDetection&&) = delete;
    ~PresenceDetection() = default;

    /**
     * Constructor.
     *
     * @param actions actions that detect whether the device is present
     */
    explicit PresenceDetection(std::vector<std::unique_ptr<Action>> actions) :
        actions{std::move(actions)}
    {
    }

    /**
     * Clears the cached presence value.
     */
    void clearCache(void)
    {
        isPresent.reset();
    }

    /**
     * Executes the actions to detect whether the device is present.
     *
     * The return value of the last action indicates whether the device is
     * present.  A return value of true means the device is present; false means
     * the device is missing.
     *
     * Caches the resulting presence value.  Subsequent calls to execute() will
     * return the cached value rather than re-executing the actions.  This
     * provides a performance improvement since the actions may be expensive to
     * execute, such as I2C reads or D-Bus method calls.  The cached value can
     * be cleared by calling clearCache().
     *
     * @return true if device is present, false otherwise
     */
    bool execute(Services& services, System& system, Chassis& chassis,
                 Device& device);

    /**
     * Returns the actions that detect whether the device is present.
     *
     * @return actions
     */
    const std::vector<std::unique_ptr<Action>>& getActions() const
    {
        return actions;
    }

    /**
     * Returns the cached presence value, if any.
     *
     * @return cached presence value
     */
    std::optional<bool> getCachedPresence() const
    {
        return isPresent;
    }

  private:
    /**
     * Actions that detect whether the device is present.
     */
    std::vector<std::unique_ptr<Action>> actions{};

    /**
     * Cached presence value.  Initially has no value.
     */
    std::optional<bool> isPresent{};
};

} // namespace phosphor::power::regulators
