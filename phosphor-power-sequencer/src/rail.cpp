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

#include "rail.hpp"

#include "pmbus.hpp"
#include "power_sequencer_device.hpp"

#include <exception>
#include <format>

namespace phosphor::power::sequencer
{
namespace status_vout = phosphor::pmbus::status_vout;

bool Rail::isPresent(Services& services)
{
    // Initially assume rail is present
    bool present{true};

    // If presence data member contains an inventory path to check
    if (presence)
    {
        const std::string& inventoryPath = *presence;
        try
        {
            present = services.isPresent(inventoryPath);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error{std::format(
                "Unable to determine presence of rail {} using inventory path {}: {}",
                name, inventoryPath, e.what())};
        }
    }

    return present;
}

uint16_t Rail::getStatusWord(PowerSequencerDevice& device)
{
    uint16_t value{0};
    try
    {
        verifyHasPage();
        value = device.getStatusWord(*page);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{
            std::format("Unable to read STATUS_WORD value for rail {}: {}",
                        name, e.what())};
    }
    return value;
}

uint8_t Rail::getStatusVout(PowerSequencerDevice& device)
{
    uint8_t value{0};
    try
    {
        verifyHasPage();
        value = device.getStatusVout(*page);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{
            std::format("Unable to read STATUS_VOUT value for rail {}: {}",
                        name, e.what())};
    }
    return value;
}

double Rail::getReadVout(PowerSequencerDevice& device)
{
    double value{0.0};
    try
    {
        verifyHasPage();
        value = device.getReadVout(*page);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to read READ_VOUT value for rail {}: {}", name, e.what())};
    }
    return value;
}

double Rail::getVoutUVFaultLimit(PowerSequencerDevice& device)
{
    double value{0.0};
    try
    {
        verifyHasPage();
        value = device.getVoutUVFaultLimit(*page);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to read VOUT_UV_FAULT_LIMIT value for rail {}: {}", name,
            e.what())};
    }
    return value;
}

bool Rail::hasPgoodFault(PowerSequencerDevice& device, Services& services,
                         const std::vector<int>& gpioValues,
                         std::map<std::string, std::string>& additionalData)
{
    // If rail is not present, return false and don't check anything else
    if (!isPresent(services))
    {
        services.logInfoMsg(std::format("Rail {} is not present", name));
        return false;
    }

    // Check if STATUS_VOUT indicates a pgood fault occurred
    bool hasFault = hasPgoodFaultStatusVout(device, services, additionalData);

    // Check if a GPIO value indicates a pgood fault occurred
    if (!hasFault)
    {
        hasFault = hasPgoodFaultGPIO(services, gpioValues, additionalData);
    }

    // Check if output voltage is below UV limit indicating pgood fault occurred
    if (!hasFault)
    {
        hasFault = hasPgoodFaultOutputVoltage(device, services, additionalData);
    }

    // If fault detected, store debug data in additional data map
    if (hasFault)
    {
        services.logErrorMsg(
            std::format("Pgood fault detected in rail {}", name));
        storePgoodFaultDebugData(device, services, additionalData);
    }

    return hasFault;
}

void Rail::verifyHasPage()
{
    if (!page)
    {
        throw std::runtime_error{
            std::format("No PAGE number defined for rail {}", name)};
    }
}

bool Rail::hasPgoodFaultStatusVout(
    PowerSequencerDevice& device, Services& services,
    std::map<std::string, std::string>& additionalData)
{
    bool hasFault{false};

    // If we are checking the value of STATUS_VOUT for the rail
    if (checkStatusVout)
    {
        // Read STATUS_VOUT value from device
        uint8_t statusVout = getStatusVout(device);

        // Check if fault (non-warning) bits are set in value
        if (statusVout & ~status_vout::WARNING_MASK)
        {
            hasFault = true;
            services.logErrorMsg(std::format(
                "Rail {} has fault bits set in STATUS_VOUT: {:#04x}", name,
                statusVout));
            additionalData.emplace("STATUS_VOUT",
                                   std::format("{:#04x}", statusVout));
        }
        else if (statusVout != 0)
        {
            services.logInfoMsg(std::format(
                "Rail {} has warning bits set in STATUS_VOUT: {:#04x}", name,
                statusVout));
        }
    }

    return hasFault;
}

bool Rail::hasPgoodFaultGPIO(Services& services,
                             const std::vector<int>& gpioValues,
                             std::map<std::string, std::string>& additionalData)
{
    bool hasFault{false};

    // If a GPIO is defined for checking pgood status
    if (gpio)
    {
        // Get GPIO value
        unsigned int line = gpio->line;
        bool activeLow = gpio->activeLow;
        if (line >= gpioValues.size())
        {
            throw std::runtime_error{std::format(
                "Invalid GPIO line offset {} for rail {}: Device only has {} GPIO values",
                line, name, gpioValues.size())};
        }
        int value = gpioValues[line];

        // Check if value indicates pgood signal is not active
        if ((activeLow && (value == 1)) || (!activeLow && (value == 0)))
        {
            hasFault = true;
            services.logErrorMsg(std::format(
                "Rail {} pgood GPIO line offset {} has inactive value {}", name,
                line, value));
            additionalData.emplace("GPIO_LINE", std::format("{}", line));
            additionalData.emplace("GPIO_VALUE", std::format("{}", value));
        }
    }

    return hasFault;
}

bool Rail::hasPgoodFaultOutputVoltage(
    PowerSequencerDevice& device, Services& services,
    std::map<std::string, std::string>& additionalData)
{
    bool hasFault{false};

    // If we are comparing output voltage to UV limit to check pgood status
    if (compareVoltageToLimit)
    {
        // Read output voltage and UV fault limit values from device
        double vout = getReadVout(device);
        double uvLimit = getVoutUVFaultLimit(device);

        // If output voltage is at or below UV fault limit
        if (vout <= uvLimit)
        {
            hasFault = true;
            services.logErrorMsg(std::format(
                "Rail {} output voltage {}V is <= UV fault limit {}V", name,
                vout, uvLimit));
            additionalData.emplace("READ_VOUT", std::format("{}", vout));
            additionalData.emplace("VOUT_UV_FAULT_LIMIT",
                                   std::format("{}", uvLimit));
        }
    }

    return hasFault;
}

void Rail::storePgoodFaultDebugData(
    PowerSequencerDevice& device, Services& services,
    std::map<std::string, std::string>& additionalData)
{
    additionalData.emplace("RAIL_NAME", name);
    if (page)
    {
        try
        {
            uint16_t statusWord = getStatusWord(device);
            services.logInfoMsg(
                std::format("Rail {} STATUS_WORD: {:#06x}", name, statusWord));
            additionalData.emplace("STATUS_WORD",
                                   std::format("{:#06x}", statusWord));
        }
        catch (...)
        {
            // Ignore error; don't interrupt pgood fault handling
        }
    }
}

} // namespace phosphor::power::sequencer
