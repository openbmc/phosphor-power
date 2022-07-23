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

#include "dbus_sensor.hpp"
#include "sensors.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

#include <chrono>
#include <map>
#include <memory>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class DBusSensors
 *
 * Implementation of the Sensors interface using D-Bus.
 */
class DBusSensors : public Sensors
{
  public:
    // Specify which compiler-generated methods we want
    DBusSensors() = delete;
    DBusSensors(const DBusSensors&) = delete;
    DBusSensors(DBusSensors&&) = delete;
    DBusSensors& operator=(const DBusSensors&) = delete;
    DBusSensors& operator=(DBusSensors&&) = delete;
    virtual ~DBusSensors() = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit DBusSensors(sdbusplus::bus_t& bus) :
        bus{bus}, manager{bus, sensorsObjectPath}
    {}

    /** @copydoc Sensors::enable() */
    virtual void enable() override;

    /** @copydoc Sensors::endCycle() */
    virtual void endCycle() override;

    /** @copydoc Sensors::endRail() */
    virtual void endRail(bool errorOccurred) override;

    /** @copydoc Sensors::disable() */
    virtual void disable() override;

    /** @copydoc Sensors::setValue() */
    virtual void setValue(SensorType type, double value) override;

    /** @copydoc Sensors::startCycle() */
    virtual void startCycle() override;

    /** @copydoc Sensors::startRail() */
    virtual void startRail(const std::string& rail,
                           const std::string& deviceInventoryPath,
                           const std::string& chassisInventoryPath) override;

  private:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t& bus;

    /**
     * D-Bus object manager.
     *
     * Causes this application to implement the
     * org.freedesktop.DBus.ObjectManager interface.
     */
    sdbusplus::server::manager_t manager;

    /**
     * Map from sensor names to DBusSensor objects.
     */
    std::map<std::string, std::unique_ptr<DBusSensor>> sensors{};

    /**
     * Time that current monitoring cycle started.
     */
    std::chrono::system_clock::time_point cycleStartTime{};

    /**
     * Current voltage rail.
     *
     * This is set by startRail().
     */
    std::string rail{};

    /**
     * Current device inventory path.
     *
     * This is set by startRail().
     */
    std::string deviceInventoryPath{};

    /**
     * Current chassis inventory path.
     *
     * This is set by startRail().
     */
    std::string chassisInventoryPath{};
};

} // namespace phosphor::power::regulators
