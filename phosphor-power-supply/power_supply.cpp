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
