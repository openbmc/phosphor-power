/**
 * Copyright © 2022 IBM Corporation
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

#include "ucd90x_monitor.hpp"

#include "types.hpp"
#include "utility.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <gpiod.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <fstream>

namespace phosphor::power::sequencer
{

using json = nlohmann::json;
using namespace pmbus;
using namespace phosphor::logging;

const std::string compatibleInterface =
    "xyz.openbmc_project.Configuration.IBMCompatibleSystem";
const std::string compatibleNamesProperty = "Names";

UCD90xMonitor::UCD90xMonitor(sdbusplus::bus_t& bus, std::uint8_t i2cBus,
                             std::uint16_t i2cAddress,
                             const std::string& deviceName,
                             size_t numberPages) :
    PowerSequencerMonitor(bus),
    deviceName{deviceName},
    match{bus,
          sdbusplus::bus::match::rules::interfacesAdded() +
              sdbusplus::bus::match::rules::sender(
                  "xyz.openbmc_project.EntityManager"),
          std::bind(&UCD90xMonitor::interfacesAddedHandler, this,
                    std::placeholders::_1)},
    numberPages{numberPages},
    pmbusInterface{
        fmt::format("/sys/bus/i2c/devices/{}-{:04x}", i2cBus, i2cAddress)
            .c_str(),
        "ucd9000", 0}
{
    log<level::DEBUG>(
        fmt::format("Device path: {}", pmbusInterface.path().string()).c_str());
    log<level::DEBUG>(fmt::format("Hwmon path: {}",
                                  pmbusInterface.getPath(Type::Hwmon).string())
                          .c_str());
    log<level::DEBUG>(fmt::format("Debug path: {}",
                                  pmbusInterface.getPath(Type::Debug).string())
                          .c_str());
    log<level::DEBUG>(
        fmt::format("Device debug path: {}",
                    pmbusInterface.getPath(Type::DeviceDebug).string())
            .c_str());
    log<level::DEBUG>(
        fmt::format("Hwmon device debug path: {}",
                    pmbusInterface.getPath(Type::HwmonDeviceDebug).string())
            .c_str());

    // Use the compatible system types information, if already available, to
    // load the configuration file
    findCompatibleSystemTypes();
}

void UCD90xMonitor::findCompatibleSystemTypes()
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

void UCD90xMonitor::findConfigFile(
    const std::vector<std::string>& compatibleSystemTypes)
{
    // Expected config file path name:
    // /usr/share/phosphor-power-sequencer/<deviceName>Monitor_<systemType>.json

    // Add possible file names based on compatible system types (if any)
    for (const std::string& systemType : compatibleSystemTypes)
    {
        // Check if file exists
        std::filesystem::path pathName{"/usr/share/phosphor-power-sequencer/" +
                                       deviceName + "Monitor_" + systemType +
                                       ".json"};
        log<level::DEBUG>(
            fmt::format("Attempting config file path: {}", pathName.string())
                .c_str());
        if (std::filesystem::exists(pathName))
        {
            log<level::INFO>(
                fmt::format("Config file path: {}", pathName.string()).c_str());
            parseConfigFile(pathName);
            break;
        }
    }
}

void UCD90xMonitor::interfacesAddedHandler(sdbusplus::message_t& msg)
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

bool UCD90xMonitor::isPresent(const std::string& inventoryPath)
{
    // Empty path indicates no presence check is needed
    if (inventoryPath.empty())
    {
        return true;
    }

    // Get presence from D-Bus interface/property
    try
    {
        bool present{true};
        util::getProperty(INVENTORY_IFACE, PRESENT_PROP, inventoryPath,
                          INVENTORY_MGR_IFACE, bus, present);
        log<level::INFO>(
            fmt::format("Presence, path: {}, value: {}", inventoryPath, present)
                .c_str());
        return present;
    }
    catch (const std::exception& e)
    {
        log<level::INFO>(
            fmt::format("Error getting presence property, path: {}, error: {}",
                        inventoryPath, e.what())
                .c_str());
        return false;
    }
}

void UCD90xMonitor::formatGpioValues(
    const std::vector<int>& values, unsigned int /*numberLines*/,
    std::map<std::string, std::string>& additionalData) const
{
    log<level::INFO>(fmt::format("GPIO values: {}", values).c_str());
    additionalData.emplace("GPIO_VALUES", fmt::format("{}", values));
}

void UCD90xMonitor::onFailure(bool timeout, const std::string& powerSupplyError)
{
    std::string message;
    std::map<std::string, std::string> additionalData{};

    try
    {
        onFailureCheckRails(message, additionalData, powerSupplyError);
        log<level::DEBUG>(
            fmt::format("After onFailureCheckRails, message: {}", message)
                .c_str());
        onFailureCheckPins(message, additionalData);
        log<level::DEBUG>(
            fmt::format("After onFailureCheckPins, message: {}", message)
                .c_str());
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Error when collecting metadata, error: {}", e.what())
                .c_str());
        additionalData.emplace("ERROR", e.what());
    }

    if (message.empty())
    {
        // Could not isolate, but we know something failed, so issue a timeout
        // or generic power good error
        message = timeout ? powerOnTimeoutError : shutdownError;
    }
    logError(message, additionalData);
    if (!timeout)
    {
        createBmcDump();
    }
}

void UCD90xMonitor::onFailureCheckPins(
    std::string& message, std::map<std::string, std::string>& additionalData)
{
    // Create a lower case version of device name to use as label in libgpiod
    std::string label{deviceName};
    std::transform(label.begin(), label.end(), label.begin(), ::tolower);

    // Setup a list of all the GPIOs on the chip
    gpiod::chip chip{label, gpiod::chip::OPEN_BY_LABEL};
    log<level::INFO>(fmt::format("GPIO chip name: {}", chip.name()).c_str());
    log<level::INFO>(fmt::format("GPIO chip label: {}", chip.label()).c_str());
    unsigned int numberLines = chip.num_lines();
    log<level::INFO>(
        fmt::format("GPIO chip number of lines: {}", numberLines).c_str());

    // Read GPIO values.  Work around libgpiod bulk line maximum by getting
    // values from individual lines.  The libgpiod line offsets are the same as
    // the Pin IDs defined in the UCD90xxx PMBus interface documentation.  These
    // Pin IDs are different from the pin numbers on the chip.  For example, on
    // the UCD90160, "FPWM1/GPIO5" is Pin ID/line offset 0, but it is pin number
    // 17 on the chip.
    std::vector<int> values;
    try
    {
        for (unsigned int offset = 0; offset < numberLines; ++offset)
        {
            gpiod::line line = chip.get_line(offset);
            line.request({"phosphor-power-control",
                          gpiod::line_request::DIRECTION_INPUT, 0});
            values.push_back(line.get_value());
            line.release();
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Error reading device GPIOs, error: {}", e.what())
                .c_str());
        additionalData.emplace("GPIO_ERROR", e.what());
    }

    formatGpioValues(values, numberLines, additionalData);

    // Only check GPIOs if no rail fail was found
    if (message.empty())
    {
        for (size_t pin = 0; pin < pins.size(); ++pin)
        {
            unsigned int line = pins[pin].line;
            if (line < values.size())
            {
                int value = values[line];

                if ((value == 0) && isPresent(pins[pin].presence))
                {
                    additionalData.emplace("INPUT_NUM",
                                           fmt::format("{}", line));
                    additionalData.emplace("INPUT_NAME", pins[pin].name);
                    message =
                        "xyz.openbmc_project.Power.Error.PowerSequencerPGOODFault";
                    return;
                }
            }
        }
    }
}

void UCD90xMonitor::onFailureCheckRails(
    std::string& message, std::map<std::string, std::string>& additionalData,
    const std::string& powerSupplyError)
{
    auto statusWord = readStatusWord();
    additionalData.emplace("STATUS_WORD", fmt::format("{:#06x}", statusWord));
    try
    {
        additionalData.emplace("MFR_STATUS",
                               fmt::format("{:#014x}", readMFRStatus()));
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Error when collecting MFR_STATUS, error: {}", e.what())
                .c_str());
        additionalData.emplace("ERROR", e.what());
    }

    // The status_word register has a summary bit to tell us if each page even
    // needs to be checked
    if (statusWord & status_word::VOUT_FAULT)
    {
        for (size_t page = 0; page < numberPages; page++)
        {
            auto statusVout = pmbusInterface.insertPageNum(STATUS_VOUT, page);
            if (pmbusInterface.exists(statusVout, Type::Debug))
            {
                uint8_t vout = pmbusInterface.read(statusVout, Type::Debug);

                if (vout)
                {
                    // If any bits are on log them, though some are just
                    // warnings so they won't cause errors
                    log<level::INFO>(
                        fmt::format("{}, value: {:#04x}", statusVout, vout)
                            .c_str());

                    // Log errors if any non-warning bits on
                    if (vout & ~status_vout::WARNING_MASK)
                    {
                        additionalData.emplace(
                            fmt::format("STATUS{}_VOUT", page),
                            fmt::format("{:#04x}", vout));

                        // Base the callouts on the first present vout failure
                        // found
                        if (message.empty() && (page < rails.size()) &&
                            isPresent(rails[page].presence))
                        {
                            additionalData.emplace("RAIL_NAME",
                                                   rails[page].name);

                            // Use power supply error if set and 12v rail has
                            // failed, else use voltage error
                            message =
                                ((page == 0) && !powerSupplyError.empty())
                                    ? powerSupplyError
                                    : "xyz.openbmc_project.Power.Error.PowerSequencerVoltageFault";
                        }
                    }
                }
            }
        }
    }
    // If no vout failure found, but power supply error is set, use power supply
    // error
    if (message.empty())
    {
        message = powerSupplyError;
    }
}

void UCD90xMonitor::parseConfigFile(const std::filesystem::path& pathName)
{
    try
    {
        log<level::DEBUG>(
            std::string("Loading configuration file " + pathName.string())
                .c_str());

        std::ifstream file{pathName};
        json rootElement = json::parse(file);
        log<level::DEBUG>(fmt::format("Parsed, root element is_object: {}",
                                      rootElement.is_object())
                              .c_str());

        // Parse rail information from config file
        auto railsIterator = rootElement.find("rails");
        if (railsIterator != rootElement.end())
        {
            for (const auto& railElement : *railsIterator)
            {
                log<level::DEBUG>(fmt::format("Rail element is_object: {}",
                                              railElement.is_object())
                                      .c_str());

                auto nameIterator = railElement.find("name");
                if (nameIterator != railElement.end())
                {
                    log<level::DEBUG>(fmt::format("Name element is_string: {}",
                                                  (*nameIterator).is_string())
                                          .c_str());
                    Rail rail;
                    rail.name = (*nameIterator).get<std::string>();

                    // Presence element is optional
                    auto presenceIterator = railElement.find("presence");
                    if (presenceIterator != railElement.end())
                    {
                        log<level::DEBUG>(
                            fmt::format("Presence element is_string: {}",
                                        (*presenceIterator).is_string())
                                .c_str());

                        rail.presence = (*presenceIterator).get<std::string>();
                    }

                    log<level::DEBUG>(
                        fmt::format("Adding rail, name: {}, presence: {}",
                                    rail.name, rail.presence)
                            .c_str());
                    rails.emplace_back(std::move(rail));
                }
                else
                {
                    log<level::ERR>(
                        fmt::format(
                            "No name found within rail in configuration file: {}",
                            pathName.string())
                            .c_str());
                }
            }
        }
        else
        {
            log<level::ERR>(
                fmt::format("No rails found in configuration file: {}",
                            pathName.string())
                    .c_str());
        }
        log<level::DEBUG>(
            fmt::format("Found number of rails: {}", rails.size()).c_str());

        // Parse pin information from config file
        auto pinsIterator = rootElement.find("pins");
        if (pinsIterator != rootElement.end())
        {
            for (const auto& pinElement : *pinsIterator)
            {
                log<level::DEBUG>(fmt::format("Pin element is_object: {}",
                                              pinElement.is_object())
                                      .c_str());
                auto nameIterator = pinElement.find("name");
                auto lineIterator = pinElement.find("line");
                if (nameIterator != pinElement.end() &&
                    lineIterator != pinElement.end())
                {
                    log<level::DEBUG>(fmt::format("Name element is_string: {}",
                                                  (*nameIterator).is_string())
                                          .c_str());
                    log<level::DEBUG>(
                        fmt::format("Line element is_number_integer: {}",
                                    (*lineIterator).is_number_integer())
                            .c_str());
                    Pin pin;
                    pin.name = (*nameIterator).get<std::string>();
                    pin.line = (*lineIterator).get<unsigned int>();

                    // Presence element is optional
                    auto presenceIterator = pinElement.find("presence");
                    if (presenceIterator != pinElement.end())
                    {
                        log<level::DEBUG>(
                            fmt::format("Presence element is_string: {}",
                                        (*presenceIterator).is_string())
                                .c_str());
                        pin.presence = (*presenceIterator).get<std::string>();
                    }

                    log<level::DEBUG>(
                        fmt::format(
                            "Adding pin, name: {}, line: {}, presence: {}",
                            pin.name, pin.line, pin.presence)
                            .c_str());
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
            fmt::format("Found number of pins: {}", pins.size()).c_str());
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Error parsing configuration file, error: {}", e.what())
                .c_str());
    }
}

uint16_t UCD90xMonitor::readStatusWord()
{
    return pmbusInterface.read(STATUS_WORD, Type::Debug);
}

uint64_t UCD90xMonitor::readMFRStatus()
{
    const std::string mfrStatus = "mfr_status";
    return pmbusInterface.read(mfrStatus, Type::HwmonDeviceDebug);
}

} // namespace phosphor::power::sequencer
