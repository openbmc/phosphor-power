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

#include "types.hpp"
#include "utility.hpp"

#include <fstream>
#include <phosphor-logging/log.hpp>

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

    auto devices = data.find("psuDevices");
    if (devices == data.end())
    {
        log<level::WARNING>("Unable to find psuDevices");
        return {};
    }
    auto devicePath = devices->find(psuInventoryPath);
    if (devicePath == devices->end())
    {
        log<level::WARNING>("Unable to find path for PSU",
                            entry("PATH=%s", psuInventoryPath.c_str()));
        return {};
    }
    return *devicePath;
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
    int ret = updater.doUpdate();
    updater.bindUnbind(true);
    return ret == 0;
}

Updater::Updater(const std::string& psuInventoryPath,
                 const std::string& devPath, const std::string& imageDir) :
    bus(sdbusplus::bus::new_default()),
    psuInventoryPath(psuInventoryPath), devPath(devPath),
    devName(internal::getDeviceName(devPath)), imageDir(imageDir)
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
// errors. So set the PSU inventory's Presnet property to false so that
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
    // Pre-condition for updating PSU:
    // * Host is powered off
    // * All other PSUs are having AC input
    // * All other PSUs are having standby output
    if (util::isPoweredOn(bus))
    {
        return false;
    }
    // TODO
    return true;
}

int Updater::doUpdate()
{
    // TODO
    return 0;
}

} // namespace updater
