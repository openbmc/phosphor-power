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

#include "dbus_sensors.hpp"
#include "error_logging.hpp"
#include "journal.hpp"
#include "presence_service.hpp"
#include "sensors.hpp"
#include "vpd.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor::power::regulators
{

/**
 * @class Services
 *
 * Abstract base class that provides an interface to system services like error
 * logging and the journal.
 *
 * This interface is a container for a set of system services.  It can be passed
 * as a single parameter to the rest of the application.
 */
class Services
{
  public:
    // Specify which compiler-generated methods we want
    Services() = default;
    Services(const Services&) = delete;
    Services(Services&&) = delete;
    Services& operator=(const Services&) = delete;
    Services& operator=(Services&&) = delete;
    virtual ~Services() = default;

    /**
     * Returns the D-Bus bus object.
     *
     * @return D-Bus bus
     */
    virtual sdbusplus::bus_t& getBus() = 0;

    /**
     * Returns the error logging interface.
     *
     * @return error logging interface
     */
    virtual ErrorLogging& getErrorLogging() = 0;

    /**
     * Returns the journal interface.
     *
     * @return journal interface
     */
    virtual Journal& getJournal() = 0;

    /**
     * Returns the interface to hardware presence data.
     *
     * @return hardware presence interface
     */
    virtual PresenceService& getPresenceService() = 0;

    /**
     * Returns the sensors interface.
     *
     * @return sensors interface
     */
    virtual Sensors& getSensors() = 0;

    /**
     * Returns the interface to hardware VPD (Vital Product Data).
     *
     * @return hardware VPD interface
     */
    virtual VPD& getVPD() = 0;
};

/**
 * @class BMCServices
 *
 * Implementation of the Services interface using standard BMC system services.
 */
class BMCServices : public Services
{
  public:
    // Specify which compiler-generated methods we want
    BMCServices() = delete;
    BMCServices(const BMCServices&) = delete;
    BMCServices(BMCServices&&) = delete;
    BMCServices& operator=(const BMCServices&) = delete;
    BMCServices& operator=(BMCServices&&) = delete;
    virtual ~BMCServices() = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit BMCServices(sdbusplus::bus_t& bus) :
        bus{bus}, errorLogging{bus},
        presenceService{bus}, sensors{bus}, vpd{bus}
    {}

    /** @copydoc Services::getBus() */
    virtual sdbusplus::bus_t& getBus() override
    {
        return bus;
    }

    /** @copydoc Services::getErrorLogging() */
    virtual ErrorLogging& getErrorLogging() override
    {
        return errorLogging;
    }

    /** @copydoc Services::getJournal() */
    virtual Journal& getJournal() override
    {
        return journal;
    }

    /** @copydoc Services::getPresenceService() */
    virtual PresenceService& getPresenceService() override
    {
        return presenceService;
    }

    /** @copydoc Services::getSensors() */
    virtual Sensors& getSensors() override
    {
        return sensors;
    }

    /** @copydoc Services::getVPD() */
    virtual VPD& getVPD() override
    {
        return vpd;
    }

  private:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus_t& bus;

    /**
     * Implementation of the ErrorLogging interface using D-Bus method calls.
     */
    DBusErrorLogging errorLogging;

    /**
     * Implementation of the Journal interface that writes to the systemd
     * journal.
     */
    SystemdJournal journal{};

    /**
     * Implementation of the PresenceService interface using D-Bus method calls.
     */
    DBusPresenceService presenceService;

    /**
     * Implementation of the Sensors interface using D-Bus.
     */
    DBusSensors sensors;

    /**
     * Implementation of the VPD interface using D-Bus method calls.
     */
    DBusVPD vpd;
};

} // namespace phosphor::power::regulators
