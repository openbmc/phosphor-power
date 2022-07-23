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

#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <map>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class PresenceService
 *
 * Abstract base class that provides an interface to hardware presence data.
 *
 * The interface is used to determine whether hardware is present.
 */
class PresenceService
{
  public:
    // Specify which compiler-generated methods we want
    PresenceService() = default;
    PresenceService(const PresenceService&) = delete;
    PresenceService(PresenceService&&) = delete;
    PresenceService& operator=(const PresenceService&) = delete;
    PresenceService& operator=(PresenceService&&) = delete;
    virtual ~PresenceService() = default;

    /**
     * Clears any cached hardware presence data.
     */
    virtual void clearCache(void) = 0;

    /**
     * Returns whether the hardware with the specified inventory path is
     * present.
     *
     * May return a cached value if one is available to improve performance.
     *
     * Throws an exception if an error occurs while obtaining the presence
     * value.
     *
     * @param inventoryPath D-Bus inventory path of the hardware
     * @return true if hardware is present, false otherwise
     */
    virtual bool isPresent(const std::string& inventoryPath) = 0;
};

/**
 * @class DBusPresenceService
 *
 * Implementation of the PresenceService interface using D-Bus method calls.
 */
class DBusPresenceService : public PresenceService
{
  public:
    // Specify which compiler-generated methods we want
    DBusPresenceService() = delete;
    DBusPresenceService(const DBusPresenceService&) = delete;
    DBusPresenceService(DBusPresenceService&&) = delete;
    DBusPresenceService& operator=(const DBusPresenceService&) = delete;
    DBusPresenceService& operator=(DBusPresenceService&&) = delete;
    virtual ~DBusPresenceService() = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit DBusPresenceService(sdbusplus::bus_t& bus) : bus{bus}
    {}

    /** @copydoc PresenceService::clearCache() */
    virtual void clearCache(void) override
    {
        cache.clear();
    }

    /** @copydoc PresenceService::isPresent() */
    virtual bool isPresent(const std::string& inventoryPath) override;

  private:
    /**
     * Returns whether the specified D-Bus exception is one of the expected
     * types that can be thrown if hardware is not present.
     *
     * @return true if exception type is expected, false otherwise
     */
    bool isExpectedException(const sdbusplus::exception_t& e);

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t& bus;

    /**
     * Cached presence data.
     *
     * Map from inventory paths to presence values.
     */
    std::map<std::string, bool> cache{};
};

} // namespace phosphor::power::regulators
