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

#include "ucd90320_monitor.hpp"

#include "utility.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <fstream>
#include <map>
#include <string>

namespace phosphor::power::sequencer
{

using json = nlohmann::json;
using namespace pmbus;
using namespace phosphor::logging;
using namespace phosphor::power;

const std::string compatibleInterface =
    "xyz.openbmc_project.Configuration.IBMCompatibleSystem";
const std::string compatibleNamesProperty = "Names";

namespace device_error = sdbusplus::xyz::openbmc_project::Common::Device::Error;

UCD90320Monitor::UCD90320Monitor(sdbusplus::bus::bus& bus, std::uint8_t i2cBus,
                                 std::uint16_t i2cAddress) :
    PowerSequencerMonitor(bus),
    match{bus,
          sdbusplus::bus::match::rules::interfacesAdded() +
              sdbusplus::bus::match::rules::sender(
                  "xyz.openbmc_project.EntityManager"),
          std::bind(&UCD90320Monitor::interfacesAddedHandler, this,
                    std::placeholders::_1)},
    pmbusInterface{
        fmt::format("/sys/bus/i2c/devices/{}-{:04x}", i2cBus, i2cAddress)
            .c_str(),
        "ucd9000", 0}

{
    // Use the compatible system types information, if already available, to
    // load the configuration file
    findCompatibleSystemTypes();
}

void UCD90320Monitor::findCompatibleSystemTypes()
{
    try
    {
        auto subTree = util::getSubTree(bus, "/xyz/openbmc_project/inventory",
                                        compatibleInterface, 0);

        auto objectIt = subTree.cbegin();
        if (objectIt != subTree.cend())
        {
            const auto& objPath = objectIt->first;

            // Get the first service name
            auto serviceIt = objectIt->second.cbegin();
            if (serviceIt != objectIt->second.cend())
            {
                std::string service = serviceIt->first;
                if (!service.empty())
                {
                    std::vector<std::string> compatibleSystemTypes;

                    // Get compatible system types property value
                    util::getProperty(compatibleInterface,
                                      compatibleNamesProperty, objPath, service,
                                      bus, compatibleSystemTypes);

                    log<level::DEBUG>(
                        fmt::format("Found compatible systems: {}",
                                    compatibleSystemTypes)
                            .c_str());
                    // Use compatible systems information to find config file
                    findConfigFile(compatibleSystemTypes);
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Compatible system types information is not available.
    }
}

void UCD90320Monitor::findConfigFile(
    const std::vector<std::string>& compatibleSystemTypes)
{
    // Expected config file path name:
    // /usr/share/phosphor-power-sequencer/UCD90320Monitor_<systemType>.json

    // Add possible file names based on compatible system types (if any)
    for (const std::string& systemType : compatibleSystemTypes)
    {
        // Check if file exists
        std::filesystem::path pathName{
            "/usr/share/phosphor-power-sequencer/UCD90320Monitor_" +
            systemType + ".json"};
        if (std::filesystem::exists(pathName))
        {
            log<level::INFO>(
                fmt::format("Config file path: {}", pathName.string()).c_str());
            parseConfigFile(pathName);
            break;
        }
    }
}

void UCD90320Monitor::interfacesAddedHandler(sdbusplus::message::message& msg)
{
    // Only continue if message is valid and rails / pins have not already been
    // found
    if (!msg || !rails.empty())
    {
        return;
    }

    try
    {
        // Read the dbus message
        sdbusplus::message::object_path objPath;
        std::map<std::string,
                 std::map<std::string, std::variant<std::vector<std::string>>>>
            interfaces;
        msg.read(objPath, interfaces);

        // Find the compatible interface, if present
        auto itIntf = interfaces.find(compatibleInterface);
        if (itIntf != interfaces.cend())
        {
            // Find the Names property of the compatible interface, if present
            auto itProp = itIntf->second.find(compatibleNamesProperty);
            if (itProp != itIntf->second.cend())
            {
                // Get value of Names property
                const auto& propValue = std::get<0>(itProp->second);
                if (!propValue.empty())
                {
                    log<level::INFO>(
                        fmt::format(
                            "InterfacesAdded for compatible systems: {}",
                            propValue)
                            .c_str());

                    // Use compatible systems information to find config file
                    findConfigFile(propValue);
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Error trying to read interfacesAdded message.
    }
}

void UCD90320Monitor::parseConfigFile(const std::filesystem::path& pathName)
{
    try
    {
        std::ifstream file{pathName};
        json rootElement = json::parse(file);

        // Parse rail information from config file
        auto railsIterator = rootElement.find("rails");
        if (railsIterator != rootElement.end())
        {
            for (const auto& railElement : *railsIterator)
            {
                std::string rail = railElement.get<std::string>();
                rails.emplace_back(std::move(rail));
            }
        }
        else
        {
            log<level::ERR>(
                fmt::format("No rails found in configuration file: {}",
                            pathName.string())
                    .c_str());
        }
        log<level::DEBUG>(fmt::format("Found rails: {}", rails).c_str());

        // Parse pin information from config file
        auto pinsIterator = rootElement.find("pins");
        if (pinsIterator != rootElement.end())
        {
            for (const auto& pinElement : *pinsIterator)
            {
                auto nameIterator = pinElement.find("name");
                auto lineIterator = pinElement.find("line");

                if (nameIterator != pinElement.end() &&
                    lineIterator != pinElement.end())
                {
                    std::string name = (*nameIterator).get<std::string>();
                    unsigned int line = (*lineIterator).get<unsigned int>();

                    Pin pin;
                    pin.name = name;
                    pin.line = line;
                    pins.emplace_back(std::move(pin));
                }
                else
                {
                    log<level::ERR>(
                        fmt::format(
                            "No name or line found within pin in configuration file: {}",
                            pathName.string())
                            .c_str());
                }
            }
        }
        else
        {
            log<level::ERR>(
                fmt::format("No pins found in configuration file: {}",
                            pathName.string())
                    .c_str());
        }
        log<level::DEBUG>(
            fmt::format("Found number of pins: {}", rails.size()).c_str());
    }
    catch (const std::exception& e)
    {
        // Log error message in journal
        log<level::ERR>(std::string("Exception parsing configuration file: " +
                                    std::string(e.what()))
                            .c_str());
    }
}

void UCD90320Monitor::onFailure(bool timeout,
                                const std::string& powerSupplyError)
{
    std::string message;
    std::map<std::string, std::string> additionalData{};

    try
    {
        onFailureCheckRails(message, additionalData, powerSupplyError);
        log<level::INFO>(
            fmt::format("After onFailureCheckRails, message: {}", message)
                .c_str());
        onFailureCheckPins(message, additionalData);
        log<level::INFO>(
            fmt::format("After onFailureCheckPins, message: {}", message)
                .c_str());
    }
    catch (device_error::ReadFailure& e)
    {
        log<level::ERR>(
            fmt::format("ReadFailure when collecting metadata, error {}",
                        e.what())
                .c_str());
    }

    if (message.empty())
    {
        // Could not isolate, but we know something failed, so issue a timeout
        // or generic power good error
        message = timeout ? "xyz.openbmc_project.Power.Error.PowerOnTimeout"
                          : "xyz.openbmc_project.Power.Error.Shutdown";
    }
    logError(message, additionalData);
}

void UCD90320Monitor::onFailureCheckPins(
    std::string& message, std::map<std::string, std::string>& additionalData)
{
    // Setup a list of all the GPIOs on the chip
    gpiod::chip chip{"ucd90320", gpiod::chip::OPEN_BY_LABEL};
    log<level::INFO>(fmt::format("GPIO chip name: {}", chip.name()).c_str());
    log<level::INFO>(fmt::format("GPIO chip label: {}", chip.label()).c_str());
    unsigned int numberLines = chip.num_lines();
    log<level::INFO>(
        fmt::format("GPIO chip number of lines: {}", numberLines).c_str());

    // Workaround libgpiod bulk line maximum by getting values from individual
    // lines
    std::vector<int> values;
    for (unsigned int offset = 0; offset < numberLines; ++offset)
    {
        gpiod::line line = chip.get_line(offset);
        line.request({"phosphor-power-control",
                      gpiod::line_request::DIRECTION_INPUT, 0});
        values.push_back(line.get_value());
        line.release();
    }

    // Add GPIO values to additional data
    log<level::INFO>(fmt::format("GPIO values: {}", values).c_str());
    additionalData.emplace("GPIO_VALUES", fmt::format("{}", values));

    // Only check GPIOs if no rail fail was found
    if (message.empty())
    {
        for (size_t pin = 0; pin < pins.size(); ++pin)
        {
            unsigned int line = pins[pin].line;
            if (line < values.size())
            {
                int value = values[line];
                if (value == 0)
                {
                    additionalData.emplace("INPUT_NUM",
                                           fmt::format("{}", line));
                    additionalData.emplace("INPUT_NAME", pins[pin].name);
                    additionalData.emplace("INPUT_STATUS",
                                           fmt::format("{}", value));
                    message =
                        "xyz.openbmc_project.Power.Error.PowerSequencerPGOODFault";
                    return;
                }
            }
        }
    }
}

void UCD90320Monitor::onFailureCheckRails(
    std::string& message, std::map<std::string, std::string>& additionalData,
    const std::string& powerSupplyError)
{
    auto statusWord = readStatusWord();
    additionalData.emplace("STATUS_WORD", fmt::format("{:#06x}", statusWord));
    try
    {
        additionalData.emplace("MFR_STATUS",
                               fmt::format("{:#010x}", readMFRStatus()));
    }
    catch (device_error::ReadFailure& e)
    {
        log<level::ERR>(
            fmt::format("ReadFailure when collecting MFR_STATUS, error {}",
                        e.what())
                .c_str());
    }

    // The status_word register has a summary bit to tell us if each page even
    // needs to be checked
    if (statusWord & status_word::VOUT_FAULT)
    {
        constexpr size_t numberPages = 24;
        for (size_t page = 0; page < numberPages; page++)
        {
            auto statusVout = pmbusInterface.insertPageNum(STATUS_VOUT, page);
            if (pmbusInterface.exists(statusVout, Type::Debug))
            {
                uint8_t vout = pmbusInterface.read(statusVout, Type::Debug);

                // If any bits are on log them, though some are just warnings so
                // they won't cause errors
                if (vout)
                {
                    log<level::INFO>(
                        fmt::format("STATUS_VOUT, page: {}, value: {:#04x}", page,
                                    vout)
                            .c_str());
                }

                // Log errors if any non-warning bits on
                if (vout & ~status_vout::WARNING_MASK)
                {
                    additionalData.emplace("STATUS_VOUT",
                                           fmt::format("{:#04x}", vout));
                    additionalData.emplace("PAGE", fmt::format("{}", page));
                    additionalData.emplace("RAIL_NAME", rails[page]);

                    // Use power supply error if set and 12v rail has failed, else
                    // use voltage error
                    message =
                        ((page == 0) && !powerSupplyError.empty())
                            ? powerSupplyError
                            : "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault";
                    return;
                }
            }
        }
    }
}

uint16_t UCD90320Monitor::readStatusWord()
{
    return pmbusInterface.read(STATUS_WORD, Type::Debug);
}

uint32_t UCD90320Monitor::readMFRStatus()
{
    const std::string mfrStatus = "mfr_status";
    return pmbusInterface.read(mfrStatus, Type::HwmonDeviceDebug);
}

} // namespace phosphor::power::sequencer
