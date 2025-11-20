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
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class GPIOsOnlyDevice
 *
 * PowerSequencerDevice sub-class that only uses the named GPIOs.
 *
 * This class uses named GPIOs to power the device on/off and read the power
 * good signal from the device.
 *
 * No other communication is performed to the device over I2C or through a
 * device driver. If a pgood fault occurs, no attempt will be made to determine
 * which voltage rail caused the fault.
 *
 * This device type is useful for simple systems that do not require pgood fault
 * isolation. It is also useful as a temporary solution when performing early
 * bring-up work on a new system.
 */
class GPIOsOnlyDevice : public PowerSequencerDevice
{
  public:
    GPIOsOnlyDevice() = delete;
    GPIOsOnlyDevice(const GPIOsOnlyDevice&) = delete;
    GPIOsOnlyDevice(GPIOsOnlyDevice&&) = delete;
    GPIOsOnlyDevice& operator=(const GPIOsOnlyDevice&) = delete;
    GPIOsOnlyDevice& operator=(GPIOsOnlyDevice&&) = delete;
    virtual ~GPIOsOnlyDevice() = default;

    /**
     * Constructor.
     *
     * Throws an exception if an error occurs during initialization.
     *
     * @param powerControlGPIOName name of the GPIO that turns this device on
     *                             and off
     * @param powerGoodGPIOName name of the GPIO that reads the power good
     *                          signal from this device
     * @param services System services like hardware presence and the journal
     */
    explicit GPIOsOnlyDevice(const std::string& powerControlGPIOName,
                             const std::string& powerGoodGPIOName,
                             Services& services) :
        powerControlGPIOName{powerControlGPIOName},
        powerGoodGPIOName{powerGoodGPIOName}
    {
        powerControlGPIO = services.createGPIO(powerControlGPIOName);

        powerGoodGPIO = services.createGPIO(powerGoodGPIOName);
        powerGoodGPIO->request(GPIO::RequestType::Read);
    }

    /** @copydoc PowerSequencerDevice::getName() */
    virtual const std::string& getName() const override
    {
        return deviceName;
    }

    /** @copydoc PowerSequencerDevice::getBus() */
    virtual uint8_t getBus() const override
    {
        return 0;
    }

    /** @copydoc PowerSequencerDevice::getAddress() */
    virtual uint16_t getAddress() const override
    {
        return 0;
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

    /** @copydoc PowerSequencerDevice::getGPIOValues() */
    virtual std::vector<int> getGPIOValues(
        [[maybe_unused]] Services& services) override
    {
        throw std::logic_error{"getGPIOValues() is not supported"};
    }

    /** @copydoc PowerSequencerDevice::getStatusWord() */
    virtual uint16_t getStatusWord([[maybe_unused]] uint8_t page) override
    {
        throw std::logic_error{"getStatusWord() is not supported"};
    }

    /** @copydoc PowerSequencerDevice::getStatusVout() */
    virtual uint8_t getStatusVout([[maybe_unused]] uint8_t page) override
    {
        throw std::logic_error{"getStatusVout() is not supported"};
    }

    /** @copydoc PowerSequencerDevice::getReadVout() */
    virtual double getReadVout([[maybe_unused]] uint8_t page) override
    {
        throw std::logic_error{"getReadVout() is not supported"};
    }

    /** @copydoc PowerSequencerDevice::getVoutUVFaultLimit() */
    virtual double getVoutUVFaultLimit([[maybe_unused]] uint8_t page) override
    {
        throw std::logic_error{"getVoutUVFaultLimit() is not supported"};
    }

    /** @copydoc PowerSequencerDevice::findPgoodFault() */
    virtual std::string findPgoodFault(
        [[maybe_unused]] Services& services,
        [[maybe_unused]] const std::string& powerSupplyError,
        [[maybe_unused]] std::map<std::string, std::string>& additionalData)
        override
    {
        return std::string{};
    }

    inline static const std::string deviceName{"gpios_only_device"};

  protected:
    /**
     * Name of the GPIO that turns this device on and off.
     */
    std::string powerControlGPIOName{};

    /**
     * Name of the GPIO that reads the power good signal from this device.
     */
    std::string powerGoodGPIOName{};

    /**
     * Empty list of voltage rails to return from getRails().
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
