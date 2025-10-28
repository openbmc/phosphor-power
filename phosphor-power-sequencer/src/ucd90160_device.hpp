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

#include "rail.hpp"
#include "services.hpp"
#include "ucd90x_device.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class UCD90160Device
 *
 * Class representing the UCD90160 power sequencer device.
 */
class UCD90160Device : public UCD90xDevice
{
  public:
    // Specify which compiler-generated methods we want
    UCD90160Device() = delete;
    UCD90160Device(const UCD90160Device&) = delete;
    UCD90160Device(UCD90160Device&&) = delete;
    UCD90160Device& operator=(const UCD90160Device&) = delete;
    UCD90160Device& operator=(UCD90160Device&&) = delete;
    virtual ~UCD90160Device() = default;

    /**
     * Constructor.
     *
     * @param bus I2C bus for the device
     * @param address I2C address for the device
     * @param rails Voltage rails that are enabled and monitored by this device
     * @param services System services like hardware presence and the journal
     */
    explicit UCD90160Device(uint8_t bus, uint16_t address,
                            std::vector<std::unique_ptr<Rail>> rails,
                            Services& services) :
        UCD90xDevice(deviceName, bus, address, std::move(rails), services)
    {}

    constexpr static std::string deviceName{"UCD90160"};

  protected:
    /** @copydoc UCD90xDevice::storeGPIOValues() */
    virtual void storeGPIOValues(
        Services& services, const std::vector<int>& values,
        std::map<std::string, std::string>& additionalData) override;
};

} // namespace phosphor::power::sequencer
