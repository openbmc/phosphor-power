/**
 * Copyright © 2017 IBM Corporation
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

#include "runtime_monitor.hpp"

#include "elog-errors.hpp"
#include "utility.hpp"

#include <org/open_power/Witherspoon/Fault/error.hpp>
#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace power
{

using namespace sdbusplus::org::open_power::Witherspoon::Fault::Error;

int RuntimeMonitor::run()
{
#ifdef DEVICE_ACCESS
    return DeviceMonitor::run();
#else
    return EXIT_SUCCESS;
#endif
}

void RuntimeMonitor::onPowerLost(sdbusplus::message_t&)
{
    lg2::info("PGOOD failure detected.  Checking for faults.");

    try
    {
        timer.setEnabled(false);

#ifdef DEVICE_ACCESS
        device->onFailure();
#endif
        // Note: This application only runs when the system has
        // power, so it will be killed by systemd sometime shortly
        // after this power off is issued.

        util::powerOff<Shutdown>(bus);
    }
    catch (const std::exception& e)
    {
        // No need to crash
        lg2::error(
            "Exception occurred while checking for PGOOD faults in onPowerLost: {ERROR}",
            "ERROR", e);
    }
}

} // namespace power
} // namespace phosphor
