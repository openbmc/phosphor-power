/**
 * Copyright © 2021 IBM Corporation
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

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class VPD
 *
 * Abstract base class that provides an interface to hardware VPD (Vital Product
 * Data).
 *
 * The interface is used to obtain VPD keyword values.
 */
class VPD
{
  public:
    // Specify which compiler-generated methods we want
    VPD() = default;
    VPD(const VPD&) = delete;
    VPD(VPD&&) = delete;
    VPD& operator=(const VPD&) = delete;
    VPD& operator=(VPD&&) = delete;
    virtual ~VPD() = default;

    /**
     * Clears any cached hardware VPD values.
     */
    virtual void clearCache(void) = 0;

    /**
     * Returns the value of the specified VPD keyword for the specified
     * inventory path.
     *
     * May return a cached value if one is available to improve performance.
     *
     * Throws an exception if an error occurs while obtaining the VPD
     * value.
     *
     * @param inventoryPath D-Bus inventory path of the hardware
     * @param keyword VPD keyword
     * @return VPD keyword value
     */
    virtual std::vector<uint8_t> getValue(const std::string& inventoryPath,
                                          const std::string& keyword) = 0;
};

/**
 * @class DBusVPD
 *
 * Implementation of the VPD interface using D-Bus method calls.
 */
class DBusVPD : public VPD
{
  public:
    // Specify which compiler-generated methods we want
    DBusVPD() = delete;
    DBusVPD(const DBusVPD&) = delete;
    DBusVPD(DBusVPD&&) = delete;
    DBusVPD& operator=(const DBusVPD&) = delete;
    DBusVPD& operator=(DBusVPD&&) = delete;
    virtual ~DBusVPD() = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit DBusVPD(sdbusplus::bus::bus& bus) : bus{bus}
    {}

    /** @copydoc VPD::clearCache() */
    virtual void clearCache(void) override
    {
        cache.clear();
    }

    /** @copydoc VPD::getValue() */
    virtual std::vector<uint8_t> getValue(const std::string& inventoryPath,
                                          const std::string& keyword) override;

  private:
    /**
     * Gets the value of the specified VPD keyword from a D-Bus interface and
     * property.
     *
     * Throws an exception if an error occurs while obtaining the VPD
     * value.
     *
     * @param inventoryPath D-Bus inventory path of the hardware
     * @param keyword VPD keyword
     * @param value the resulting keyword value
     */
    void getDBusProperty(const std::string& inventoryPath,
                         const std::string& keyword,
                         std::vector<uint8_t>& value);

    /**
     * Returns whether the specified D-Bus exception indicates the VPD interface
     * or property does not exist for the specified inventory path.
     *
     * This is treated as an "empty" keyword value rather than an error
     * condition.
     *
     * @return true if exception indicates interface/property does not exist
     */
    bool isUnknownPropertyException(const sdbusplus::exception_t& e);

    /**
     * Type alias for map from keyword names to values.
     */
    using KeywordMap = std::map<std::string, std::vector<uint8_t>>;

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus::bus& bus;

    /**
     * Cached VPD keyword values.
     *
     * Map from inventory paths to VPD keywords.
     */
    std::map<std::string, KeywordMap> cache{};
};

} // namespace phosphor::power::regulators
