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
#include <map>
#include <memory>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <elog-errors.hpp>
#include <xyz/openbmc_project/Sensor/Device/error.hpp>
#include <xyz/openbmc_project/Control/Device/error.hpp>
#include <xyz/openbmc_project/Power/Fault/error.hpp>
#include "ucd90160.hpp"

namespace witherspoon
{
namespace power
{

using namespace std::string_literals;

const auto CLEAR_LOGGED_FAULTS = "clear_logged_faults"s;

const auto DEVICE_NAME = "UCD90160"s;
const auto DRIVER_NAME = "ucd9000"s;

using namespace pmbus;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Control::Device::Error;
using namespace sdbusplus::xyz::openbmc_project::Sensor::Device::Error;
using namespace sdbusplus::xyz::openbmc_project::Power::Fault::Error;

UCD90160::UCD90160(size_t instance) :
    Device(DEVICE_NAME, instance),
    interface(std::get<ucd90160::pathField>(
                      deviceMap.find(instance)->second),
              DRIVER_NAME,
              instance)
{
}

void UCD90160::onFailure()
{
    try
    {
        auto voutError = checkVOUTFaults();

        auto pgoodError = checkPGOODFaults(false);

        //Not a voltage or PGOOD fault, but we know something
        //failed so still create an error log.
        if (!voutError && !pgoodError)
        {
            createPowerFaultLog();
        }
    }
    catch (ReadFailure& e)
    {
        if (!accessError)
        {
            commit<ReadFailure>();
            accessError = true;
        }
    }
}

void UCD90160::analyze()
{
    try
    {
        //Note: Voltage faults are always fatal, so they just
        //need to be analyzed in onFailure().

        checkPGOODFaults(true);
    }
    catch (ReadFailure& e)
    {
        if (!accessError)
        {
            commit<ReadFailure>();
            accessError = true;
        }
    }
}

void UCD90160::clearFaults()
{
    try
    {
        interface.write(CLEAR_LOGGED_FAULTS, 1, Type::Base);
    }
    catch (WriteFailure& e)
    {
        if (!accessError)
        {
            log<level::ERR>("UCD90160 clear logged faults command failed");
            commit<WriteFailure>();
            accessError = true;
        }
    }
}

bool UCD90160::checkVOUTFaults()
{
    return false;
}

bool UCD90160::checkPGOODFaults(bool polling)
{
    return false;
}

void UCD90160::createPowerFaultLog()
{

}

}
}
