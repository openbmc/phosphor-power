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
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Sensor/Device/error.hpp>
#include <xyz/openbmc_project/Control/Device/error.hpp>
#include <xyz/openbmc_project/Power/Fault/error.hpp>
#include "elog-errors.hpp"
#include "power_supply.hpp"
#include "pmbus.hpp"
#include "utility.hpp"

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Control::Device::Error;
using namespace sdbusplus::xyz::openbmc_project::Sensor::Device::Error;
using namespace sdbusplus::xyz::openbmc_project::Power::Fault::Error;

namespace witherspoon
{
namespace power
{
namespace psu
{


void PowerSupply::analyze()
{
    using namespace witherspoon::pmbus;

    try
    {
        auto curUVFault = pmbusIntf.readBit(VIN_UV_FAULT, Type::Hwmon);
        //TODO: 3 consecutive reads should be performed.
        // If 3 consecutive reads are seen, log the fault.
        // Driver gives cached value, read once a second.
        // increment for fault on, decrement for fault off, to deglitch.
        // If count reaches 3, we have fault. If count reaches 0, fault is
        // cleared.

        //TODO: INPUT FAULT or WARNING bit to check from STATUS_WORD
        // pmbus-core update to read high byte of STATUS_WORD?

        if ((curUVFault != vinUVFault) || inputFault)
        {
            if (curUVFault)
            {
                //FIXME - metadata
                report<PowerSupplyUnderVoltageFault>();
                vinUVFault = true;
            }
            else
            {
                log<level::INFO>("VIN_UV_FAULT cleared");
                vinUVFault = false;
            }
        }
    }
    catch (ReadFailure& e)
    {
        if (!readFailLogged)
        {
            commit<ReadFailure>();
            readFailLogged = true;
            // TODO - Need to reset that to false at start of power on, or
            // presence change.
        }
    }

    return;
}

void PowerSupply::clearFaults()
{
    //TODO - Clear faults at pre-poweron.
    return;
}

}
}
}
