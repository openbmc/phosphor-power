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

#include "pmbus_driver_device.hpp"

#include <ctype.h> // for tolower()

#include <algorithm>
#include <exception>
#include <filesystem>
#include <format>
#include <regex>
#include <stdexcept>

namespace phosphor::power::sequencer
{

using namespace pmbus;
namespace fs = std::filesystem;

std::vector<int> PMBusDriverDevice::getGPIOValues(Services& services)
{
    // Get lower case version of device name to use as chip label
    std::string label{name};
    std::transform(label.begin(), label.end(), label.begin(), ::tolower);

    // Read the GPIO values by specifying the chip label
    std::vector<int> values;
    try
    {
        values = services.getGPIOValues(label);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to read GPIO values from device {} using label {}: {}",
            name, label, e.what())};
    }
    return values;
}

uint16_t PMBusDriverDevice::getStatusWord(uint8_t page)
{
    uint16_t value{0};
    try
    {
        std::string fileName = std::format("status{:d}", page);
        value = pmbusInterface->read(fileName, Type::Debug);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to read STATUS_WORD for PAGE {:d} of device {}: {}", page,
            name, e.what())};
    }
    return value;
}

uint8_t PMBusDriverDevice::getStatusVout(uint8_t page)
{
    uint8_t value{0};
    try
    {
        std::string fileName = std::format("status{:d}_vout", page);
        value = pmbusInterface->read(fileName, Type::Debug);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to read STATUS_VOUT for PAGE {:d} of device {}: {}", page,
            name, e.what())};
    }
    return value;
}

double PMBusDriverDevice::getReadVout(uint8_t page)
{
    double volts{0.0};
    try
    {
        unsigned int fileNumber = getFileNumber(page);
        std::string fileName = std::format("in{}_input", fileNumber);
        std::string millivoltsStr =
            pmbusInterface->readString(fileName, Type::Hwmon);
        unsigned long millivolts = std::stoul(millivoltsStr);
        volts = millivolts / 1000.0;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to read READ_VOUT for PAGE {:d} of device {}: {}", page,
            name, e.what())};
    }
    return volts;
}

double PMBusDriverDevice::getVoutUVFaultLimit(uint8_t page)
{
    double volts{0.0};
    try
    {
        unsigned int fileNumber = getFileNumber(page);
        std::string fileName = std::format("in{}_lcrit", fileNumber);
        std::string millivoltsStr =
            pmbusInterface->readString(fileName, Type::Hwmon);
        unsigned long millivolts = std::stoul(millivoltsStr);
        volts = millivolts / 1000.0;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{std::format(
            "Unable to read VOUT_UV_FAULT_LIMIT for PAGE {:d} of device {}: {}",
            page, name, e.what())};
    }
    return volts;
}

unsigned int PMBusDriverDevice::getFileNumber(uint8_t page)
{
    if (pageToFileNumber.empty())
    {
        buildPageToFileNumberMap();
    }

    auto it = pageToFileNumber.find(page);
    if (it == pageToFileNumber.end())
    {
        throw std::runtime_error{std::format(
            "Unable to find hwmon file number for PAGE {:d} of device {}", page,
            name)};
    }

    return it->second;
}

void PMBusDriverDevice::buildPageToFileNumberMap()
{
    // Clear any existing mappings
    pageToFileNumber.clear();

    // Build mappings using voltage label files in hwmon directory
    try
    {
        fs::path hwmonDir = pmbusInterface->getPath(Type::Hwmon);
        if (fs::is_directory(hwmonDir))
        {
            // Loop through all files in hwmon directory
            std::string fileName;
            unsigned int fileNumber;
            std::optional<uint8_t> page;
            for (const auto& f : fs::directory_iterator{hwmonDir})
            {
                // If this is a voltage label file
                fileName = f.path().filename().string();
                if (isLabelFile(fileName, fileNumber))
                {
                    // Read PMBus PAGE number from label file contents
                    page = readPageFromLabelFile(fileName);
                    if (page)
                    {
                        // Add mapping from PAGE number to file number
                        pageToFileNumber.emplace(*page, fileNumber);
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error{
            std::format("Unable to map PMBus PAGE numbers to hwmon file "
                        "numbers for device {}: {}",
                        name, e.what())};
    }
}

bool PMBusDriverDevice::isLabelFile(const std::string& fileName,
                                    unsigned int& fileNumber)
{
    bool isLabel{false};
    try
    {
        // Check if file name has expected pattern for voltage label file
        std::regex regex{"in(\\d+)_label"};
        std::smatch results;
        if (std::regex_match(fileName, results, regex))
        {
            // Verify 2 match results: entire match and one sub-match
            if (results.size() == 2)
            {
                // Get sub-match that contains the file number
                std::string fileNumberStr = results.str(1);
                fileNumber = std::stoul(fileNumberStr);
                isLabel = true;
            }
        }
    }
    catch (...)
    {
        // Ignore error.  If this file is needed for pgood fault detection, an
        // error will occur later when the necessary mapping is missing.  Avoid
        // logging unnecessary errors for files that may not be required.
    }
    return isLabel;
}

std::optional<uint8_t>
    PMBusDriverDevice::readPageFromLabelFile(const std::string& fileName)
{
    std::optional<uint8_t> page;
    try
    {
        // Read voltage label file contents
        std::string contents =
            pmbusInterface->readString(fileName, Type::Hwmon);

        // Check if file contents match the expected pattern
        std::regex regex{"vout(\\d+)"};
        std::smatch results;
        if (std::regex_match(contents, results, regex))
        {
            // Verify 2 match results: entire match and one sub-match
            if (results.size() == 2)
            {
                // Get sub-match that contains the page number + 1
                std::string pageStr = results.str(1);
                page = std::stoul(pageStr) - 1;
            }
        }
    }
    catch (...)
    {
        // Ignore error.  If this file is needed for pgood fault detection, an
        // error will occur later when the necessary mapping is missing.  Avoid
        // logging unnecessary errors for files that may not be required.
    }
    return page;
}

} // namespace phosphor::power::sequencer
