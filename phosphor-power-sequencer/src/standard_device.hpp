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

#include "gpio.hpp"
#include "power_sequencer_device.hpp"
#include "rail.hpp"
#include "services.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class StandardDevice
 *
 * PowerSequencerDevice sub-class that implements the standard pgood fault
 * detection algorithm.
 *
 * When adding support for a new power sequencer device type, create a sub-class
 * of StandardDevice if possible.  This will ensure that pgood fault detection
 * works consistently across device types.
 */
class StandardDevice : public PowerSequencerDevice
{
  public:
    // Specify which compiler-generated methods we want
    StandardDevice() = delete;
    StandardDevice(const StandardDevice&) = delete;
    StandardDevice(StandardDevice&&) = delete;
    StandardDevice& operator=(const StandardDevice&) = delete;
    StandardDevice& operator=(StandardDevice&&) = delete;
    virtual ~StandardDevice() = default;

    /**
     * Constructor.
     *
     * Throws an exception if an error occurs during initialization.
     *
     * @param name device name
     * @param bus I2C bus for the device
     * @param address I2C address for the device
     * @param powerControlGPIOName name of the GPIO that turns this device on
     *                             and off
     * @param powerGoodGPIOName name of the GPIO that reads the power good
     *                          signal from this device
     * @param rails voltage rails that are enabled and monitored by this device
     * @param services System services like hardware presence and the journal
     */
    explicit StandardDevice(
        const std::string& name, uint8_t bus, uint16_t address,
        const std::string& powerControlGPIOName,
        const std::string& powerGoodGPIOName,
        std::vector<std::unique_ptr<Rail>> rails, Services& services) :
        name{name}, bus{bus}, address{address},
        powerControlGPIOName{powerControlGPIOName},
        powerGoodGPIOName{powerGoodGPIOName}, rails{std::move(rails)}
    {
        powerControlGPIO = services.createGPIO(powerControlGPIOName);

        powerGoodGPIO = services.createGPIO(powerGoodGPIOName);
        powerGoodGPIO->request(GPIO::RequestType::Read);
    }

    /** @copydoc PowerSequencerDevice::getName() */
    virtual const std::string& getName() const override
    {
        return name;
    }

    /** @copydoc PowerSequencerDevice::getBus() */
    virtual uint8_t getBus() const override
    {
        return bus;
    }

    /** @copydoc PowerSequencerDevice::getAddress() */
    virtual uint16_t getAddress() const override
    {
        return address;
    }

    /** @copydoc PowerSequencerDevice::getPowerControlGPIOName() */
    virtual const std::string& getPowerControlGPIOName() const override
    {
        return powerControlGPIOName;
    }

    /** @copydoc PowerSequencerDevice::getPowerGoodGPIOName() */
    virtual const std::string& getPowerGoodGPIOName() const override
    {
        return powerGoodGPIOName;
    }

    /** @copydoc PowerSequencerDevice::getRails() */
    virtual const std::vector<std::unique_ptr<Rail>>& getRails() const override
    {
        return rails;
    }

    /** @copydoc PowerSequencerDevice::getPowerControlGPIO() */
    virtual GPIO& getPowerControlGPIO() override
    {
        return *powerControlGPIO;
    }

    /** @copydoc PowerSequencerDevice::getPowerGoodGPIO() */
    virtual GPIO& getPowerGoodGPIO() override
    {
        return *powerGoodGPIO;
    }

    /** @copydoc PowerSequencerDevice::powerOn() */
    virtual void powerOn() override
    {
        powerControlGPIO->request(GPIO::RequestType::Write);
        powerControlGPIO->setValue(1);
        powerControlGPIO->release();
    }

    /** @copydoc PowerSequencerDevice::powerOff() */
    virtual void powerOff() override
    {
        powerControlGPIO->request(GPIO::RequestType::Write);
        powerControlGPIO->setValue(0);
        powerControlGPIO->release();
    }

    /** @copydoc PowerSequencerDevice::getPowerGood() */
    virtual bool getPowerGood() override
    {
        return (powerGoodGPIO->getValue() == 1);
    }

    /** @copydoc PowerSequencerDevice::findPgoodFault()
     *
     * Calls prepareForPgoodFaultDetection() before starting detection.  If a
     * pgood fault is detected, calls storePgoodFaultDebugData().
     */
    virtual std::string findPgoodFault(
        Services& services, const std::string& powerSupplyError,
        std::map<std::string, std::string>& additionalData) override;

  protected:
    /**
     * Prepare for pgood fault detection.
     *
     * Perform any actions that are necessary to prepare for fault detection.
     * For example, cache information that is slow to obtain and is used
     * multiple times during detection.
     *
     * Default implementation does nothing.  Override in sub-classes if needed.
     *
     * @param services System services like hardware presence and the journal
     */
    virtual void prepareForPgoodFaultDetection(
        [[maybe_unused]] Services& services)
    {}

    /**
     * Returns the GPIO values that can be read from the device, if possible.
     *
     * If the device does not support reading GPIO values or an error occurs, an
     * empty vector is returned.
     *
     * @param services System services like hardware presence and the journal
     * @return GPIO values, or empty vector if values could not be read
     */
    virtual std::vector<int> getGPIOValuesIfPossible(Services& services);

    /**
     * Checks whether a pgood fault has occurred on one of the rails being
     * monitored by this device.
     *
     * If a pgood fault was found in a rail, a pointer to the Rail object is
     * returned.
     *
     * Throws an exception if an error occurs while trying to obtain the status
     * of the rails.
     *
     * @param services System services like hardware presence and the journal
     * @param gpioValues GPIO values obtained from the device (if any)
     * @param additionalData Additional data to include in the error log if
     *                       a pgood fault was found
     * @return pointer to Rail object where fault was found, or nullptr if no
     *         Rail found
     */
    virtual Rail* findRailWithPgoodFault(
        Services& services, const std::vector<int>& gpioValues,
        std::map<std::string, std::string>& additionalData);

    /**
     * Store pgood fault debug data in the specified additional data map.
     *
     * The default implementation stores the device name and then calls
     * storeGPIOValues().
     *
     * Sub-classes should override if needed to store device-specific data.
     *
     * This method should NOT throw exceptions.  If debug data cannot be
     * obtained, the error should be caught and ignored so that pgood error
     * handling can continue.
     *
     * @param services System services like hardware presence and the journal
     * @param gpioValues GPIO values obtained from the device (if any)
     * @param additionalData Additional data to include in an error log
     */
    virtual void storePgoodFaultDebugData(
        Services& services, const std::vector<int>& gpioValues,
        std::map<std::string, std::string>& additionalData);

    /**
     * Store GPIO values in the specified additional data map.
     *
     * The default implementation stores the values as a simple list of
     * integers.
     *
     * Sub-classes should override if more advanced formatting is needed.  For
     * example, GPIOs could be stored individually with a name and value, or
     * related GPIOs could be formatted as a group.
     *
     * @param services System services like hardware presence and the journal
     * @param values GPIO values obtained from the device (if any)
     * @param additionalData Additional data to include in an error log
     */
    virtual void storeGPIOValues(
        Services& services, const std::vector<int>& values,
        std::map<std::string, std::string>& additionalData);

    /**
     * Device name.
     */
    std::string name{};

    /**
     * I2C bus for the device.
     */
    uint8_t bus;

    /**
     * I2C address for the device.
     */
    uint16_t address;

    /**
     * Name of the GPIO that turns this device on and off.
     */
    std::string powerControlGPIOName{};

    /**
     * Name of the GPIO that reads the power good signal from this device.
     */
    std::string powerGoodGPIOName{};

    /**
     * Voltage rails that are enabled and monitored by this device.
     */
    std::vector<std::unique_ptr<Rail>> rails{};

    /**
     * GPIO that turns this device on and off.
     */
    std::unique_ptr<GPIO> powerControlGPIO{};

    /**
     * GPIO that reads the power good signal from this device.
     */
    std::unique_ptr<GPIO> powerGoodGPIO{};
};

} // namespace phosphor::power::sequencer
