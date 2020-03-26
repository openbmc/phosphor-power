#include "config.h"

#include "power_supply.hpp"

#include "types.hpp"
#include "util.hpp"

#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <chrono>  // sleep_for()
#include <cstdint> // uint8_t...
#include <thread>  // sleep_for()

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
            updateInventory();
        }
        else
        {
            present = false;

            // Clear out the now outdated inventory properties
            updateInventory();
        }
    }
}

void PowerSupply::updateInventory()
{
    using namespace phosphor::pmbus;

#ifdef IBM_VPD
    std::string ccin;
    std::string pn;
    std::string fn;
    std::string header;
    std::string sn;
    std::string version;
    using PropertyMap =
        std::map<std::string, std::variant<std::string, std::vector<uint8_t>>>;
    PropertyMap assetProps;
    PropertyMap versionProps;
    PropertyMap ipzvpdDINFProps;
    PropertyMap ipzvpdVINIProps;
    using InterfaceMap = std::map<std::string, PropertyMap>;
    InterfaceMap interfaces;
    using ObjectMap = std::map<sdbusplus::message::object_path, InterfaceMap>;
    ObjectMap object;
#endif

    if (present)
    {
        // TODO: non-IBM inventory updates?

#ifdef IBM_VPD
        // TODO: Immediately trying to read the "files" causes read failure.
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2ms);

        try
        {
            ccin = pmbusIntf->readString(CCIN, Type::HwmonDeviceDebug);
            assetProps.emplace(MODEL_PROP, ccin);
        }
        catch (ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure, path...
            // TODO - ibm918
            // https://github.com/openbmc/docs/blob/master/designs/vpd-collection.md
            // The BMC must log errors if any of the VPD cannot be properly
            // parsed or fails ECC checks.
        }

        try
        {
            pn = pmbusIntf->readString(PART_NUMBER, Type::HwmonDeviceDebug);
            assetProps.emplace(PN_PROP, pn);
        }
        catch (ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure, path...
        }

        try
        {
            fn = pmbusIntf->readString(FRU_NUMBER, Type::HwmonDeviceDebug);
        }
        catch (ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure, path...
        }

        try
        {
            header =
                pmbusIntf->readString(SERIAL_HEADER, Type::HwmonDeviceDebug);
            sn = pmbusIntf->readString(SERIAL_NUMBER, Type::HwmonDeviceDebug);
            assetProps.emplace(SN_PROP, sn);
        }
        catch (ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure, path...
        }

        try
        {
            version = pmbusIntf->readString(FW_VERSION, Type::HwmonDeviceDebug);
            versionProps.emplace(VERSION_PROP, version);
        }
        catch (ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure, path...
        }

        ipzvpdVINIProps.emplace("CC",
                                std::vector<uint8_t>(ccin.begin(), ccin.end()));
        ipzvpdVINIProps.emplace("PN",
                                std::vector<uint8_t>(pn.begin(), pn.end()));
        ipzvpdVINIProps.emplace("FN",
                                std::vector<uint8_t>(fn.begin(), fn.end()));
        std::string header_sn = header + sn + '\0';
        ipzvpdVINIProps.emplace(
            "SN", std::vector<uint8_t>(header_sn.begin(), header_sn.end()));
        std::string description = "IBM PS";
        ipzvpdVINIProps.emplace(
            "DR", std::vector<uint8_t>(description.begin(), description.end()));

        // Update the Resource Identifier (RI) keyword
        // 2 byte FRC: 0x0003
        // 2 byte RID: 0x1000, 0x1001...
        std::uint8_t num = std::stoul(
            inventoryPath.substr(inventoryPath.size() - 1, 1), nullptr, 0);
        std::vector<uint8_t> ri{0x00, 0x03, 0x10, num};
        ipzvpdDINFProps.emplace("RI", ri);

        // Fill in the FRU Label (FL) keyword.
        std::string fl = "E";
        fl.push_back(inventoryPath.back());
        fl.resize(FL_KW_SIZE, ' ');
        ipzvpdDINFProps.emplace("FL",
                                std::vector<uint8_t>(fl.begin(), fl.end()));

        interfaces.emplace(ASSET_IFACE, std::move(assetProps));
        interfaces.emplace(VERSION_IFACE, std::move(versionProps));
        interfaces.emplace(DINF_IFACE, std::move(ipzvpdDINFProps));
        interfaces.emplace(VINI_IFACE, std::move(ipzvpdVINIProps));

        auto path = inventoryPath.substr(strlen(INVENTORY_OBJ_PATH));
        object.emplace(path, std::move(interfaces));

        try
        {
            auto service =
                util::getService(INVENTORY_OBJ_PATH, INVENTORY_MGR_IFACE, bus);

            if (service.empty())
            {
                log<level::ERR>("Unable to get inventory manager service");
                return;
            }

            auto method =
                bus.new_method_call(service.c_str(), INVENTORY_OBJ_PATH,
                                    INVENTORY_MGR_IFACE, "Notify");

            method.append(std::move(object));

            auto reply = bus.call(method);
        }
        catch (std::exception& e)
        {
            log<level::ERR>(e.what(), entry("PATH=%s", inventoryPath.c_str()));
        }
#endif
    }
}

} // namespace phosphor::power::psu
