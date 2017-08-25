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
#include "names_values.hpp"
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

constexpr auto INVENTORY_OBJ_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTERFACE = "xyz.openbmc_project.Inventory.Item";
constexpr auto PRESENT_PROP = "Present";
constexpr auto POWER_OBJ_PATH = "/org/openbmc/control/power0";
constexpr auto POWER_INTERFACE = "org.openbmc.control.Power";

PowerSupply::PowerSupply(const std::string& name, size_t inst,
                         const std::string& objpath,
                         const std::string& invpath,
                         sdbusplus::bus::bus& bus,
                         event::Event& e,
                         std::chrono::seconds& t)
    : Device(name, inst), monitorPath(objpath), pmbusIntf(objpath),
      inventoryPath(invpath), bus(bus), event(e), powerOnInterval(t),
      powerOnTimer(e, [this]()
                   {
                       this->powerOn = true;
                   })
{
    using namespace sdbusplus::bus;
    auto present_obj_path = INVENTORY_OBJ_PATH + inventoryPath;
    presentMatch = std::make_unique<match_t>(bus,
                                             match::rules::propertiesChanged(
                                                     present_obj_path,
                                                     INVENTORY_INTERFACE),
                                             [this](auto& msg)
                                             {
                                                 this->inventoryChanged(msg);
                                             });
    // Get initial presence state.
    updatePresence();

    // Subscribe to power state changes
    powerOnMatch = std::make_unique<match_t>(bus,
                                             match::rules::propertiesChanged(
                                                     POWER_OBJ_PATH,
                                                     POWER_INTERFACE),
                                             [this](auto& msg)
                                             {
                                                 this->powerStateChanged(msg);
                                             });
    // Get initial power state.
    updatePowerState();
}


void PowerSupply::analyze()
{
    using namespace witherspoon::pmbus;

    try
    {
        if (present)
        {
            std::uint16_t statusWord = 0;
            std::uint8_t  statusInput = 0;

            // Read the 2 byte STATUS_WORD value to check for faults.
            statusWord = pmbusIntf.read(STATUS_WORD, Type::Debug);

            //TODO: 3 consecutive reads should be performed.
            // If 3 consecutive reads are seen, log the fault.
            // Driver gives cached value, read once a second.
            // increment for fault on, decrement for fault off, to deglitch.
            // If count reaches 3, we have fault. If count reaches 0, fault is
            // cleared.

            if ((statusWord & status_word::VIN_UV_FAULT) && !vinUVFault)
            {
                util::NamesValues nv;
                nv.add("STATUS_WORD", statusWord);

                using metadata = xyz::openbmc_project::Power::Fault::
                        PowerSupplyUnderVoltageFault;

                report<PowerSupplyUnderVoltageFault>(
                        metadata::RAW_STATUS(nv.get().c_str()));

                vinUVFault = true;
            }
            else
            {
                vinUVFault = false;
                log<level::INFO>("VIN_UV_FAULT cleared",
                                 entry("POWERSUPPLY=%s",
                                 inventoryPath.c_str()));
            }

            if ((statusWord & status_word::INPUT_FAULT_WARN) && !inputFault)
            {
                inputFault = true;

                statusInput = pmbusIntf.read(STATUS_INPUT, Type::Debug);

                util::NamesValues nv;
                nv.add("STATUS_WORD", statusWord);
                nv.add("STATUS_INPUT", statusInput);

                using metadata = xyz::openbmc_project::Power::Fault::
                        PowerSupplyInputFault;

                report<PowerSupplyInputFault>(metadata::RAW_STATUS(
                                                      nv.get().c_str()));
            }
            else
            {
                if ((inputFault) &&
                    !(statusWord & status_word::INPUT_FAULT_WARN))
                {
                    inputFault = false;

                    statusInput = pmbusIntf.read(STATUS_INPUT, Type::Debug);

                    log<level::INFO>("INPUT_FAULT_WARN cleared",
                                     entry("POWERSUPPLY=%s",
                                             inventoryPath.c_str()),
                                     entry("STATUS_WORD=0x%04X", statusWord),
                                     entry("STATUS_INPUT=0x%02X", statusInput));
                }
            }

            if (powerOn)
            {
                std::uint8_t statusVout = 0;
                std::uint8_t statusIout = 0;
                std::uint8_t statusMFR  = 0;

                // Check PG# and UNIT_IS_OFF
                if (((statusWord & status_word::POWER_GOOD_NEGATED) ||
                     (statusWord & status_word::UNIT_IS_OFF)) &&
                    !powerOnFault)
                {
                    std::uint8_t  statusVout = 0;
                    std::uint8_t  statusIout = 0;
                    std::uint8_t  statusMFR  = 0;

                    statusInput = pmbusIntf.read(STATUS_INPUT, Type::Debug);
                    auto status0Vout = pmbusIntf.insertPageNum(STATUS_VOUT, 0);
                    statusVout = pmbusIntf.read(status0Vout, Type::Debug);
                    statusIout = pmbusIntf.read(STATUS_IOUT, Type::Debug);
                    statusMFR = pmbusIntf.read(STATUS_MFR, Type::Debug);

                    util::NamesValues nv;
                    nv.add("STATUS_WORD", statusWord);
                    nv.add("STATUS_INPUT", statusInput);
                    nv.add("STATUS_VOUT", statusVout);
                    nv.add("STATUS_IOUT", statusIout);
                    nv.add("MFR_SPECIFIC", statusMFR);

                    using metadata = xyz::openbmc_project::Power::Fault::
                            PowerSupplyShouldBeOn;

                    // A power supply is OFF (or pgood low) but should be on.
                    report<PowerSupplyShouldBeOn>(
                            metadata::RAW_STATUS(nv.get().c_str()),
                            metadata::CALLOUT_INVENTORY_PATH(
                                    inventoryPath.c_str()));

                    powerOnFault = true;
                }

                // Check for an output overcurrent fault.
                if ((statusWord & status_word::IOUT_OC_FAULT) &&
                    !outputOCFault)
                {
                    statusInput = pmbusIntf.read(STATUS_INPUT, Type::Debug);
                    statusVout = pmbusIntf.read(STATUS_VOUT, Type::Debug);
                    statusIout = pmbusIntf.read(STATUS_IOUT, Type::Debug);
                    statusMFR = pmbusIntf.read(STATUS_MFR, Type::Debug);

                    util::NamesValues nv;
                    nv.add("STATUS_WORD", statusWord);
                    nv.add("STATUS_INPUT", statusInput);
                    nv.add("STATUS_VOUT", statusVout);
                    nv.add("STATUS_IOUT", statusIout);
                    nv.add("MFR_SPECIFIC", statusMFR);

                    using metadata = xyz::openbmc_project::Power::Fault::
                            PowerSupplyOutputOvercurrent;

                    report<PowerSupplyOutputOvercurrent>(
                            metadata::RAW_STATUS(nv.get().c_str()),
                            metadata::CALLOUT_INVENTORY_PATH(
                                    inventoryPath.c_str()));

                    outputOCFault = true;
                }

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

void PowerSupply::inventoryChanged(sdbusplus::message::message& msg)
{
    std::string msgSensor;
    std::map<std::string, sdbusplus::message::variant<uint32_t, bool>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find(PRESENT_PROP);
    if (valPropMap != msgData.end())
    {
        present = sdbusplus::message::variant_ns::get<bool>(valPropMap->second);

        if (present)
        {
            readFailLogged = false;
            vinUVFault = false;
            inputFault = false;
            outputOCFault = false;
        }
    }

    return;
}

void PowerSupply::updatePresence()
{
    // Use getProperty utility function to get presence status.
    std::string path = INVENTORY_OBJ_PATH + inventoryPath;
    std::string service = "xyz.openbmc_project.Inventory.Manager";

    try
    {
        util::getProperty(INVENTORY_INTERFACE, PRESENT_PROP, path,
                          service, bus, this->present);
    }
    catch (std::exception& e)
    {
        // If we happen to be trying to update presence just as it is being
        // updated, we may encounter a runtime_error. Just catch that for
        // now, let the inventoryChanged signal handler update presence later.
        present = false;
    }

}

void PowerSupply::powerStateChanged(sdbusplus::message::message& msg)
{
    int32_t state = 0;
    std::string msgSensor;
    std::map<std::string, sdbusplus::message::variant<int32_t, int32_t>>
            msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find("state");
    if (valPropMap != msgData.end())
    {
        state = sdbusplus::message::variant_ns::get<int32_t>(valPropMap->second);

        // Power is on when state=1. Set the fault logged variables to false
        // and start the power on timer when the state changes to 1.
        if (state)
        {
            readFailLogged = false;
            vinUVFault = false;
            inputFault = false;
            powerOnFault = false;
            outputOCFault = false;
            powerOnTimer.start(powerOnInterval, Timer::TimerType::oneshot);
        }
        else
        {
            powerOnTimer.stop();
            powerOn = false;
        }
    }

}

void PowerSupply::updatePowerState()
{
    // When state = 1, system is powered on
    int32_t state = 0;

    try
    {
        auto service = util::getService(POWER_OBJ_PATH,
                                        POWER_INTERFACE,
                                        bus);

        // Use getProperty utility function to get power state.
        util::getProperty<int32_t>(POWER_INTERFACE,
                                   "state",
                                   POWER_OBJ_PATH,
                                   service,
                                   bus,
                                   state);

        if (state)
        {
            powerOn = true;
        }
        else
        {
            powerOn = false;
        }
    }
    catch (std::exception& e)
    {
        log<level::INFO>("Failed to get power state. Assuming it is off.");
        powerOn = false;
    }

}

void PowerSupply::clearFaults()
{
    //TODO - Clear faults at pre-poweron. openbmc/openbmc#1736
    return;
}

}
}
}
