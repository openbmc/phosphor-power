#include "power_supply.hpp"

#include "types.hpp"
#include "util.hpp"

#include <xyz/openbmc_project/Common/Device/error.hpp>

namespace phosphor::power::psu
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;

void PowerSupply::updatePresence()
{
    try
    {
        present = getPresence(bus, inventoryPath);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        // Relying on property change or interface added to retry.
        // Log an informational trace to the journal.
        log<level::INFO>("D-Bus property access failure exception");
    }
}

void PowerSupply::analyze()
{
    using namespace phosphor::pmbus;

    if (present)
    {
        try
        {
            auto statusWord{pmbusIntf->read(STATUS_WORD, Type::Debug)};

            if (statusWord)
            {
                if (statusWord & status_word::INPUT_FAULT_WARN)
                {
                    if (!inputFault)
                    {
                        log<level::INFO>(
                            "INPUT fault",
                            entry("STATUS_WORD=0x%04X",
                                  static_cast<uint16_t>(statusWord)));
                    }

                    faultFound = true;
                    inputFault = true;
                }

                if (statusWord & status_word::MFR_SPECIFIC_FAULT)
                {
                    if (!mfrFault)
                    {
                        log<level::INFO>(
                            "MFRSPECIFIC fault",
                            entry("STATUS_WORD=0x%04X",
                                  static_cast<uint16_t>(statusWord)));
                    }
                    faultFound = true;
                    mfrFault = true;
                }

                if (statusWord & status_word::VIN_UV_FAULT)
                {
                    if (!vinUVFault)
                    {
                        log<level::INFO>(
                            "VIN_UV fault",
                            entry("STATUS_WORD=0x%04X",
                                  static_cast<uint16_t>(statusWord)));
                    }

                    faultFound = true;
                    vinUVFault = true;
                }
            }
            else
            {
                faultFound = false;
                inputFault = false;
                mfrFault = false;
                vinUVFault = false;
            }
        }
        catch (ReadFailure& e)
        {
            phosphor::logging::commit<ReadFailure>();
        }
    }
}

void PowerSupply::onOffConfig(uint8_t data)
{
    using namespace phosphor::pmbus;

    if (present)
    {
        log<level::INFO>("ON_OFF_CONFIG write", entry("DATA=0x%02X", data));
        try
        {
            std::vector<uint8_t> configData{data};
            pmbusIntf->writeBinary(ON_OFF_CONFIG, configData,
                                   Type::HwmonDeviceDebug);
        }
        catch (...)
        {
            // The underlying code in writeBinary will log a message to the
            // journal if the write fails. If the ON_OFF_CONFIG is not setup as
            // desired, later fault detection and analysis code should catch any
            // of the fall out. We should not need to terminate the application
            // if this write fails.
        }
    }
}

void PowerSupply::clearFaults()
{
    faultFound = false;
    inputFault = false;
    mfrFault = false;
    vinUVFault = false;

    // The PMBus device driver does not allow for writing CLEAR_FAULTS
    // directly. However, the pmbus hwmon device driver code will send a
    // CLEAR_FAULTS after reading from any of the hwmon "files" in sysfs, so
    // reading in1_input should result in clearing the fault bits in
    // STATUS_BYTE/STATUS_WORD.
    // I do not care what the return value is.
    try
    {
        static_cast<void>(
            pmbusIntf->read("in1_input", phosphor::pmbus::Type::Hwmon));
    }
    catch (ReadFailure& e)
    {
        // Since I do not care what the return value is, I really do not
        // care much if it gets a ReadFailure either. However, this should not
        // prevent the application from continuing to run, so catching the read
        // failure.
    }
}

void PowerSupply::inventoryChanged(sdbusplus::message::message& msg)
{
    std::string msgSensor;
    std::map<std::string, std::variant<uint32_t, bool>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find(PRESENT_PROP);
    if (valPropMap != msgData.end())
    {
        if (std::get<bool>(valPropMap->second))
        {
            present = true;
            onOffConfig(phosphor::pmbus::ON_OFF_CONFIG_CONTROL_PIN_ONLY);
            clearFaults();
        }
        else
        {
            present = false;

            // Clear out the now outdated inventory properties
            updateInventory();
        }
    }
}

} // namespace phosphor::power::psu
