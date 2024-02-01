/**
 * Copyright © 2024 IBM Corporation
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

#include "pmbus.hpp"
#include "xyz/openbmc_project/Logging/Entry/server.hpp"

#include <fmt/format.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace phosphor::power::sequencer
{

using namespace sdbusplus::xyz::openbmc_project::Logging::server;
using PMBusBase = phosphor::pmbus::PMBusBase;
using PMBus = phosphor::pmbus::PMBus;

/**
 * @class Services
 *
 * Abstract base class that provides an interface to system services like error
 * logging and the journal.
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
     * Logs an error message in the system journal.
     *
     * @param message message to log
     */
    virtual void logErrorMsg(const std::string& message) = 0;

    /**
     * Logs an informational message in the system journal.
     *
     * @param message message to log
     */
    virtual void logInfoMsg(const std::string& message) = 0;

    /**
     * Logs an error.
     *
     * If logging fails, a message is written to the system journal but an
     * exception is not thrown.
     *
     * @param message Message property of the error log entry
     * @param severity Severity property of the error log entry
     * @param additionalData AdditionalData property of the error log entry
     */
    virtual void
        logError(const std::string& message, Entry::Level severity,
                 std::map<std::string, std::string>& additionalData) = 0;

    /**
     * Returns whether the hardware with the specified inventory path is
     * present.
     *
     * Throws an exception if an error occurs while obtaining the presence
     * value.
     *
     * @param inventoryPath D-Bus inventory path of the hardware
     * @return true if hardware is present, false otherwise
     */
    virtual bool isPresent(const std::string& inventoryPath) = 0;

    /**
     * Reads all the GPIO values on the chip with the specified label.
     *
     * Throws an exception if an error occurs while obtaining the values.
     *
     * @param chipLabel label identifying the chip with the GPIOs
     * @return GPIO values
     */
    virtual std::vector<int> getGPIOValues(const std::string& chipLabel) = 0;

    /**
     * Creates object for communicating with a PMBus device by reading and
     * writing sysfs files.
     *
     * Throws an exception if an error occurs.
     *
     * @param bus I2C bus
     * @param address I2C address
     * @param driverName Device driver name
     * @param instance Chip instance number
     * @return object for communicating with PMBus device
     */
    virtual std::unique_ptr<PMBusBase>
        createPMBus(uint8_t bus, uint16_t address,
                    const std::string& driverName = "",
                    size_t instance = 0) = 0;
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
    explicit BMCServices(sdbusplus::bus_t& bus) : bus{bus} {}

    /** @copydoc Services::getBus() */
    virtual sdbusplus::bus_t& getBus() override
    {
        return bus;
    }

    /** @copydoc Services::logErrorMsg() */
    virtual void logErrorMsg(const std::string& message) override
    {
        lg2::error(message.c_str());
    }

    /** @copydoc Services::logInfoMsg() */
    virtual void logInfoMsg(const std::string& message) override
    {
        lg2::info(message.c_str());
    }

    /** @copydoc Services::logError() */
    virtual void
        logError(const std::string& message, Entry::Level severity,
                 std::map<std::string, std::string>& additionalData) override;

    /** @copydoc Services::isPresent() */
    virtual bool isPresent(const std::string& inventoryPath) override;

    /** @copydoc Services::getGPIOValues() */
    virtual std::vector<int>
        getGPIOValues(const std::string& chipLabel) override;

    /** @copydoc Services::createPMBus() */
    virtual std::unique_ptr<PMBusBase>
        createPMBus(uint8_t bus, uint16_t address,
                    const std::string& driverName = "",
                    size_t instance = 0) override
    {
        std::string path = fmt::format("/sys/bus/i2c/devices/{}-{:04x}", bus,
                                       address);
        return std::make_unique<PMBus>(path, driverName, instance);
    }

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
};

} // namespace phosphor::power::sequencer
