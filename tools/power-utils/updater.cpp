/**
 * Copyright © 2019 IBM Corporation
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
#include "config.h"

#include "updater.hpp"

#include "pmbus.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <phosphor-logging/log.hpp>

#include <chrono>
#include <fstream>
#include <thread>

using namespace phosphor::logging;
namespace util = phosphor::power::util;

namespace updater
{

namespace internal
{

/* Get the device name from the device path */
std::string getDeviceName(std::string devPath)
{
    if (devPath.back() == '/')
    {
        devPath.pop_back();
    }
    return fs::path(devPath).stem().string();
}

std::string getDevicePath(const std::string& psuInventoryPath)
{
    auto data = util::loadJSONFromFile(PSU_JSON_PATH);

    if (data == nullptr)
    {
        return {};
    }

    auto devicePath = data["psuDevices"][psuInventoryPath];
    if (devicePath.empty())
    {
        log<level::WARNING>("Unable to find psu devices or path");
    }
    return devicePath;
}

std::pair<uint8_t, uint8_t> parseDeviceName(const std::string& devName)
{
    // Get I2C bus id and device address, e.g. 3-0068
    // is parsed to bus id 3, device address 0x68
    auto pos = devName.find('-');
    assert(pos != std::string::npos);
    uint8_t busId = std::stoi(devName.substr(0, pos));
    uint8_t devAddr = std::stoi(devName.substr(pos + 1), nullptr, 16);
    return {busId, devAddr};
}

} // namespace internal

bool update(const std::string& psuInventoryPath, const std::string& imageDir)
{
    auto devPath = internal::getDevicePath(psuInventoryPath);
    if (devPath.empty())
    {
        return false;
    }

    Updater updater(psuInventoryPath, devPath, imageDir);
    if (!updater.isReadyToUpdate())
    {
        log<level::ERR>("PSU not ready to update",
                        entry("PSU=%s", psuInventoryPath.c_str()));
        return false;
    }

    updater.bindUnbind(false);
    updater.createI2CDevice();
    int ret = updater.doUpdate();
    updater.bindUnbind(true);
    return ret == 0;
}

Updater::Updater(const std::string& psuInventoryPath,
                 const std::string& devPath, const std::string& imageDir) :
    bus(sdbusplus::bus::new_default()), psuInventoryPath(psuInventoryPath),
    devPath(devPath), devName(internal::getDeviceName(devPath)),
    imageDir(imageDir)
{
    fs::path p = fs::path(devPath) / "driver";
    try
    {
        driverPath =
            fs::canonical(p); // Get the path that points to the driver dir
    }
    catch (const fs::filesystem_error& e)
    {
        log<level::ERR>("Failed to get canonical path",
                        entry("DEVPATH=%s", devPath.c_str()),
                        entry("ERROR=%s", e.what()));
        throw;
    }
}

// During PSU update, it needs to access the PSU i2c device directly, so it
// needs to unbind the driver during the update, and re-bind after it's done.
// After unbind, the hwmon sysfs will be gone, and the psu-monitor will report
// errors. So set the PSU inventory's Present property to false so that
// psu-monitor will not report any errors.
void Updater::bindUnbind(bool doBind)
{
    if (!doBind)
    {
        // Set non-present before unbind the driver
        setPresent(doBind);
    }
    auto p = driverPath;
    p /= doBind ? "bind" : "unbind";
    std::ofstream out(p.string());
    out << devName;

    if (doBind)
    {
        // Set to present after bind the driver
        setPresent(doBind);
    }
}

void Updater::setPresent(bool present)
{
    try
    {
        auto service = util::getService(psuInventoryPath, INVENTORY_IFACE, bus);
        util::setProperty(INVENTORY_IFACE, PRESENT_PROP, psuInventoryPath,
                          service, bus, present);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed to set present property",
                        entry("PSU=%s", psuInventoryPath.c_str()),
                        entry("PRESENT=%d", present));
    }
}

bool Updater::isReadyToUpdate()
{
    using namespace phosphor::pmbus;

    // Pre-condition for updating PSU:
    // * Host is powered off
    // * At least one other PSU is present
    // * All other PSUs that are present are having AC input and DC standby
    //   output

    if (util::isPoweredOn(bus, true))
    {
        log<level::WARNING>("Unable to update PSU when host is on");
        return false;
    }

    bool hasOtherPresent = false;
    auto paths = util::getPSUInventoryPaths(bus);
    for (const auto& p : paths)
    {
        if (p == psuInventoryPath)
        {
            // Skip check for itself
            continue;
        }

        // Check PSU present
        bool present = false;
        try
        {
            auto service = util::getService(p, INVENTORY_IFACE, bus);
            util::getProperty(INVENTORY_IFACE, PRESENT_PROP, psuInventoryPath,
                              service, bus, present);
        }
        catch (const std::exception& e)
        {
            log<level::ERR>("Failed to get present property",
                            entry("PSU=%s", p.c_str()));
        }
        if (!present)
        {
            log<level::WARNING>("PSU not present", entry("PSU=%s", p.c_str()));
            continue;
        }
        hasOtherPresent = true;

        // Typically the driver is still bound here, so it is possible to
        // directly read the debugfs to get the status.
        try
        {
            auto path = internal::getDevicePath(p);
            PMBus pmbus(path);
            uint16_t statusWord = pmbus.read(STATUS_WORD, Type::Debug);
            auto status0Vout = pmbus.insertPageNum(STATUS_VOUT, 0);
            uint8_t voutStatus = pmbus.read(status0Vout, Type::Debug);
            if ((statusWord & status_word::VOUT_FAULT) ||
                (statusWord & status_word::INPUT_FAULT_WARN) ||
                (statusWord & status_word::VIN_UV_FAULT) ||
                // For ibm-cffps PSUs, the MFR (0x80)'s OV (bit 2) and VAUX
                // (bit 6) fault map to OV_FAULT, and UV (bit 3) fault maps to
                // UV_FAULT in vout status.
                (voutStatus & status_vout::UV_FAULT) ||
                (voutStatus & status_vout::OV_FAULT))
            {
                log<level::WARNING>(
                    "Unable to update PSU when other PSU has input/ouput fault",
                    entry("PSU=%s", p.c_str()),
                    entry("STATUS_WORD=0x%04x", statusWord),
                    entry("VOUT_BYTE=0x%02x", voutStatus));
                return false;
            }
        }
        catch (const std::exception& ex)
        {
            // If error occurs on accessing the debugfs, it means something went
            // wrong, e.g. PSU is not present, and it's not ready to update.
            log<level::ERR>(ex.what());
            return false;
        }
    }
    return hasOtherPresent;
}

int Updater::doUpdate()
{
    using namespace std::chrono;

    uint8_t data;
    uint8_t unlockData[12] = {0x45, 0x43, 0x44, 0x31, 0x36, 0x30,
                              0x33, 0x30, 0x30, 0x30, 0x34, 0x01};
    uint8_t bootFlag = 0x01;
    static_assert(sizeof(unlockData) == 12);

    i2c->write(0xf0, sizeof(unlockData), unlockData);
    printf("Unlock PSU\n");

    std::this_thread::sleep_for(milliseconds(5));

    i2c->write(0xf1, bootFlag);
    printf("Set boot flag ret\n");

    std::this_thread::sleep_for(seconds(3));

    i2c->read(0xf1, data);
    printf("Read of 0x%02x, 0x%02x\n", 0xf1, data);
    return 0;
}

void Updater::createI2CDevice()
{
    auto [id, addr] = internal::parseDeviceName(devName);
    i2c = i2c::create(id, addr);
}

} // namespace updater
