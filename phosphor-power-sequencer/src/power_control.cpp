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

#include "power_control.hpp"

#include "config_file_parser.hpp"
#include "format_utils.hpp"
#include "types.hpp"
#include "ucd90160_device.hpp"
#include "ucd90320_device.hpp"
#include "utility.hpp"

#include <exception>
#include <format>
#include <functional>
#include <map>
#include <span>
#include <stdexcept>
#include <thread>
#include <utility>

namespace phosphor::power::sequencer
{

const std::string powerOnTimeoutError =
    "xyz.openbmc_project.Power.Error.PowerOnTimeout";

const std::string powerOffTimeoutError =
    "xyz.openbmc_project.Power.Error.PowerOffTimeout";

const std::string shutdownError = "xyz.openbmc_project.Power.Error.Shutdown";

PowerControl::PowerControl(sdbusplus::bus_t& bus,
                           const sdeventplus::Event& event) :
    PowerObject{bus, POWER_OBJ_PATH, PowerObject::action::defer_emit}, bus{bus},
    services{bus},
    pgoodWaitTimer{event, std::bind(&PowerControl::onFailureCallback, this)},
    powerOnAllowedTime{std::chrono::steady_clock::now() + minimumColdStartTime},
    timer{event, std::bind(&PowerControl::pollPgood, this), pollInterval}
{
    // Obtain dbus service name
    bus.request_name(POWER_IFACE);

    compatSysTypesFinder = std::make_unique<util::CompatibleSystemTypesFinder>(
        bus, std::bind_front(&PowerControl::compatibleSystemTypesFound, this));

    deviceFinder = std::make_unique<DeviceFinder>(
        bus, std::bind_front(&PowerControl::deviceFound, this));

    setUpGpio();
}

int PowerControl::getPgood() const
{
    return pgood;
}

int PowerControl::getPgoodTimeout() const
{
    return timeout.count();
}

int PowerControl::getState() const
{
    return state;
}

void PowerControl::onFailureCallback()
{
    services.logInfoMsg("After onFailure wait");

    onFailure(false);

    // Power good has failed, call for chassis hard power off
    auto method = bus.new_method_call(util::SYSTEMD_SERVICE, util::SYSTEMD_ROOT,
                                      util::SYSTEMD_INTERFACE, "StartUnit");
    method.append(util::POWEROFF_TARGET);
    method.append("replace");
    bus.call_noreply(method);
}

void PowerControl::onFailure(bool wasTimeOut)
{
    std::string error;
    std::map<std::string, std::string> additionalData{};

    // Check if pgood fault occurred on rail monitored by power sequencer device
    if (device)
    {
        try
        {
            error = device->findPgoodFault(services, powerSupplyError,
                                           additionalData);
        }
        catch (const std::exception& e)
        {
            services.logErrorMsg(e.what());
            additionalData.emplace("ERROR", e.what());
        }
    }

    // If fault was not isolated to a voltage rail, select a more generic error
    if (error.empty())
    {
        if (!powerSupplyError.empty())
        {
            error = powerSupplyError;
        }
        else if (wasTimeOut)
        {
            error = powerOnTimeoutError;
        }
        else
        {
            error = shutdownError;
        }
    }

    services.logError(error, Entry::Level::Critical, additionalData);

    if (!wasTimeOut)
    {
        services.createBMCDump();
    }
}

void PowerControl::pollPgood()
{
    if (inStateTransition)
    {
        // In transition between power on and off, check for timeout
        const auto now = std::chrono::steady_clock::now();
        if (now > pgoodTimeoutTime)
        {
            services.logErrorMsg(std::format(
                "Power state transition timeout, state: {}", state));
            inStateTransition = false;

            if (state)
            {
                // Time out powering on
                onFailure(true);
            }
            else
            {
                // Time out powering off
                std::map<std::string, std::string> additionalData{};
                services.logError(powerOffTimeoutError, Entry::Level::Critical,
                                  additionalData);
            }

            failureFound = true;
            return;
        }
    }

    int pgoodState = pgoodLine.get_value();
    if (pgoodState != pgood)
    {
        // Power good has changed since last read
        pgood = pgoodState;
        if (pgoodState == 0)
        {
            emitPowerLostSignal();
        }
        else
        {
            emitPowerGoodSignal();
            // Clear any errors on the transition to power on
            powerSupplyError.clear();
            failureFound = false;
        }
        emitPropertyChangedSignal("pgood");
    }
    if (pgoodState == state)
    {
        // Power good matches requested state
        inStateTransition = false;
    }
    else if (!inStateTransition && (pgoodState == 0) && !failureFound)
    {
        // Not in power off state, not changing state, and power good is off
        services.logErrorMsg("Chassis pgood failure");
        pgoodWaitTimer.restartOnce(std::chrono::seconds(7));
        failureFound = true;
    }
}

void PowerControl::setPgoodTimeout(int t)
{
    if (timeout.count() != t)
    {
        timeout = std::chrono::seconds(t);
        emitPropertyChangedSignal("pgood_timeout");
    }
}

void PowerControl::setPowerSupplyError(const std::string& error)
{
    powerSupplyError = error;
}

void PowerControl::setState(int s)
{
    if (state == s)
    {
        services.logInfoMsg(
            std::format("Power already at requested state: {}", state));
        return;
    }
    if (s == 0)
    {
        // Wait for two seconds when powering down. This is to allow host and
        // other BMC applications time to complete power off processing
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    else
    {
        // If minimum power off time has not passed, wait
        if (powerOnAllowedTime > std::chrono::steady_clock::now())
        {
            services.logInfoMsg(std::format(
                "Waiting {} seconds until power on allowed",
                std::chrono::duration_cast<std::chrono::seconds>(
                    powerOnAllowedTime - std::chrono::steady_clock::now())
                    .count()));
        }
        std::this_thread::sleep_until(powerOnAllowedTime);
    }

    services.logInfoMsg(std::format("setState: {}", s));
    services.logInfoMsg(std::format("Powering chassis {}", (s ? "on" : "off")));
    powerControlLine.request(
        {"phosphor-power-control", gpiod::line_request::DIRECTION_OUTPUT, 0});
    powerControlLine.set_value(s);
    powerControlLine.release();

    if (s == 0)
    {
        // Set a minimum amount of time to wait before next power on
        powerOnAllowedTime =
            std::chrono::steady_clock::now() + minimumPowerOffTime;
    }

    pgoodTimeoutTime = std::chrono::steady_clock::now() + timeout;
    inStateTransition = true;
    state = s;
    emitPropertyChangedSignal("state");
}

void PowerControl::compatibleSystemTypesFound(
    const std::vector<std::string>& types)
{
    // If we don't already have compatible system types
    if (compatibleSystemTypes.empty())
    {
        std::string typesStr = format_utils::toString(std::span{types});
        services.logInfoMsg(
            std::format("Compatible system types found: {}", typesStr));

        // Store compatible system types
        compatibleSystemTypes = types;

        // Load config file and create device object if possible
        loadConfigFileAndCreateDevice();
    }
}

void PowerControl::deviceFound(const DeviceProperties& properties)
{
    // If we don't already have device properties
    if (!deviceProperties)
    {
        services.logInfoMsg(std::format(
            "Power sequencer device found: type={}, name={}, bus={:d}, address={:#02x}",
            properties.type, properties.name, properties.bus,
            properties.address));

        // Store device properties
        deviceProperties = properties;

        // Load config file and create device object if possible
        loadConfigFileAndCreateDevice();
    }
}

void PowerControl::setUpGpio()
{
    const std::string powerControlLineName = "power-chassis-control";
    const std::string pgoodLineName = "power-chassis-good";

    pgoodLine = gpiod::find_line(pgoodLineName);
    if (!pgoodLine)
    {
        std::string errorString{"GPIO line name not found: " + pgoodLineName};
        services.logErrorMsg(errorString);
        throw std::runtime_error(errorString);
    }
    powerControlLine = gpiod::find_line(powerControlLineName);
    if (!powerControlLine)
    {
        std::string errorString{
            "GPIO line name not found: " + powerControlLineName};
        services.logErrorMsg(errorString);
        throw std::runtime_error(errorString);
    }

    pgoodLine.request(
        {"phosphor-power-control", gpiod::line_request::DIRECTION_INPUT, 0});
    int pgoodState = pgoodLine.get_value();
    pgood = pgoodState;
    state = pgoodState;
    services.logInfoMsg(std::format("Pgood state: {}", pgoodState));
}

void PowerControl::loadConfigFileAndCreateDevice()
{
    // If compatible system types and device properties have been found
    if (!compatibleSystemTypes.empty() && deviceProperties)
    {
        // Find the JSON configuration file
        std::filesystem::path configFile = findConfigFile();
        if (!configFile.empty())
        {
            // Parse the JSON configuration file
            std::vector<std::unique_ptr<Rail>> rails;
            if (parseConfigFile(configFile, rails))
            {
                // Create the power sequencer device object
                createDevice(std::move(rails));
            }
        }
    }
}

std::filesystem::path PowerControl::findConfigFile()
{
    // Find config file for current system based on compatible system types
    std::filesystem::path configFile;
    if (!compatibleSystemTypes.empty())
    {
        try
        {
            configFile = config_file_parser::find(compatibleSystemTypes);
            if (!configFile.empty())
            {
                services.logInfoMsg(std::format(
                    "JSON configuration file found: {}", configFile.string()));
            }
        }
        catch (const std::exception& e)
        {
            services.logErrorMsg(std::format(
                "Unable to find JSON configuration file: {}", e.what()));
        }
    }
    return configFile;
}

bool PowerControl::parseConfigFile(const std::filesystem::path& configFile,
                                   std::vector<std::unique_ptr<Rail>>& rails)
{
    // Parse JSON configuration file
    bool wasParsed{false};
    try
    {
        rails = config_file_parser::parse(configFile);
        wasParsed = true;
    }
    catch (const std::exception& e)
    {
        services.logErrorMsg(std::format(
            "Unable to parse JSON configuration file: {}", e.what()));
    }
    return wasParsed;
}

void PowerControl::createDevice(std::vector<std::unique_ptr<Rail>> rails)
{
    // Create power sequencer device based on device properties
    if (deviceProperties)
    {
        try
        {
            if (deviceProperties->type == UCD90160Device::deviceName)
            {
                device = std::make_unique<UCD90160Device>(
                    std::move(rails), services, deviceProperties->bus,
                    deviceProperties->address);
            }
            else if (deviceProperties->type == UCD90320Device::deviceName)
            {
                device = std::make_unique<UCD90320Device>(
                    std::move(rails), services, deviceProperties->bus,
                    deviceProperties->address);
            }
            else
            {
                throw std::runtime_error{std::format(
                    "Unsupported device type: {}", deviceProperties->type)};
            }
            services.logInfoMsg(std::format(
                "Power sequencer device created: {}", device->getName()));
        }
        catch (const std::exception& e)
        {
            services.logErrorMsg(
                std::format("Unable to create device object: {}", e.what()));
        }
    }
}

} // namespace phosphor::power::sequencer
