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

#include <map>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class VPDService
 *
 * Abstract base class that provides an interface to hardware VPD (Vital Product
 * Data).
 *
 * The interface is used to obtain VPD keyword values.
 */
class VPDService
{
  public:
    // Specify which compiler-generated methods we want
    VPDService() = default;
    VPDService(const VPDService&) = delete;
    VPDService(VPDService&&) = delete;
    VPDService& operator=(const VPDService&) = delete;
    VPDService& operator=(VPDService&&) = delete;
    virtual ~VPDService() = default;

    /**
     * Clears any cached hardware VPD.
     */
    virtual void clearCache(void) = 0;

    /**
     * Returns the VPD keyword value for the hardware with the specified
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
    virtual std::string getValue(const std::string& inventoryPath,
                                 const std::string& keyword) = 0;
};

/**
 * @class DBusVPDService
 *
 * Implementation of the VPDService interface using D-Bus method calls.
 */
class DBusVPDService : public VPDService
{
  public:
    // Specify which compiler-generated methods we want
    DBusVPDService() = delete;
    DBusVPDService(const DBusVPDService&) = delete;
    DBusVPDService(DBusVPDService&&) = delete;
    DBusVPDService& operator=(const DBusVPDService&) = delete;
    DBusVPDService& operator=(DBusVPDService&&) = delete;
    virtual ~DBusVPDService() = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit DBusVPDService(sdbusplus::bus::bus& bus) : bus{bus}
    {
    }

    /** @copydoc VPDService::clearCache() */
    virtual void clearCache(void) override
    {
        cache.clear();
    }

    /** @copydoc VPDService::getValue() */
    virtual std::string getValue(const std::string& inventoryPath,
                                 const std::string& keyword) override;

  private:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus::bus& bus;

    /**
     * Cached VPD keyword values.
     *
     * Map from inventory paths to VPD keyword values.
     */
    std::map<std::string, std::map<std::string, std::string>> cache{};
};

} // namespace phosphor::power::regulators
