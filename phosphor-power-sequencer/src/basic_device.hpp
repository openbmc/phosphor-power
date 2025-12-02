/**
 * Copyright © 2025 IBM Corporation
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
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class BasicDevice
 *
 * PowerSequencerDevice sub-class that implements basic functionality.
 *
 * BasicDevice implements the following:
 * - Data members and methods for the power sequencer properties from the JSON
 *   configuration file.
 * - Methods that utilize the named GPIOs, such as powerOn() and getPowerGood().
 */
class BasicDevice : public PowerSequencerDevice
{
  public:
    BasicDevice() = delete;
    BasicDevice(const BasicDevice&) = delete;
    BasicDevice(BasicDevice&&) = delete;
    BasicDevice& operator=(const BasicDevice&) = delete;
    BasicDevice& operator=(BasicDevice&&) = delete;

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
     */
    explicit BasicDevice(const std::string& name, uint8_t bus, uint16_t address,
                         const std::string& powerControlGPIOName,
                         const std::string& powerGoodGPIOName,
                         std::vector<std::unique_ptr<Rail>> rails) :
        name{name}, bus{bus}, address{address},
        powerControlGPIOName{powerControlGPIOName},
        powerGoodGPIOName{powerGoodGPIOName}, rails{std::move(rails)}
    {}

    /**
     * Destructor.
     *
     * Closes the device if it is open.
     */
    virtual ~BasicDevice()
    {
        if (isOpen())
        {
            // Destructors must not throw exceptions
            closeWithoutException();
        }
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

    /** @copydoc PowerSequencerDevice::open() */
    virtual void open(Services& services) override
    {
        if (!isOpen())
        {
            powerControlGPIO = services.createGPIO(powerControlGPIOName);

            powerGoodGPIO = services.createGPIO(powerGoodGPIOName);
            powerGoodGPIO->requestRead();

            isDeviceOpen = true;
        }
    }

    /** @copydoc PowerSequencerDevice::isOpen() */
    virtual bool isOpen() const override
    {
        return isDeviceOpen;
    }

    /** @copydoc PowerSequencerDevice::close() */
    virtual void close() override
    {
        if (isOpen())
        {
            powerControlGPIO.reset();

            // Verify pointer valid in case close() threw exception previously
            if (powerGoodGPIO)
            {
                powerGoodGPIO->release();
            }
            powerGoodGPIO.reset();

            isDeviceOpen = false;
        }
    }

    /** @copydoc PowerSequencerDevice::closeWithoutException() */
    virtual void closeWithoutException() noexcept override
    {
        try
        {
            close();
        }
        catch (...)
        {}
    }

    /** @copydoc PowerSequencerDevice::getPowerControlGPIO() */
    virtual GPIO& getPowerControlGPIO() override
    {
        verifyIsOpen();
        return *powerControlGPIO;
    }

    /** @copydoc PowerSequencerDevice::getPowerGoodGPIO() */
    virtual GPIO& getPowerGoodGPIO() override
    {
        verifyIsOpen();
        return *powerGoodGPIO;
    }

    /** @copydoc PowerSequencerDevice::powerOn() */
    virtual void powerOn() override
    {
        verifyIsOpen();
        powerControlGPIO->requestWrite(1);
        powerControlGPIO->setValue(1);
        powerControlGPIO->release();
    }

    /** @copydoc PowerSequencerDevice::powerOff() */
    virtual void powerOff() override
    {
        verifyIsOpen();
        powerControlGPIO->requestWrite(0);
        powerControlGPIO->setValue(0);
        powerControlGPIO->release();
    }

    /** @copydoc PowerSequencerDevice::getPowerGood() */
    virtual bool getPowerGood() override
    {
        verifyIsOpen();
        return (powerGoodGPIO->getValue() == 1);
    }

  protected:
    /**
     * Verifies that this device is open.
     *
     * Throws an exception if device is not open.
     */
    virtual void verifyIsOpen()
    {
        if (!isOpen())
        {
            throw std::runtime_error{"Device not open: " + name};
        }
    }

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
     * Specifies whether this device is open.
     */
    bool isDeviceOpen{false};

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
