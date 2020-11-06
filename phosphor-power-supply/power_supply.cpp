#include "config.h"

#include "power_supply.hpp"

#include "types.hpp"
#include "util.hpp"

#include <fmt/format.h>

#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <chrono>  // sleep_for()
#include <cstdint> // uint8_t...
#include <thread>  // sleep_for()

namespace phosphor::power::psu
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;

PowerSupply::PowerSupply(sdbusplus::bus::bus& bus, const std::string& invpath,
                         std::uint8_t i2cbus, const std::uint16_t i2caddr) :
    bus(bus),
    i2cbus(i2cbus), i2caddr(i2caddr), inventoryPath(invpath)
{
    if (inventoryPath.empty())
    {
        throw std::invalid_argument{"Invalid empty inventoryPath"};
    }

    // Setup the functions to call when the D-Bus inventory path for the
    // Present property changes.
    presentMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(inventoryPath,
                                                        INVENTORY_IFACE),
        [this](auto& msg) { this->inventoryChanged(msg); });
    presentAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::path_namespace(inventoryPath),
        [this](auto& msg) { this->inventoryChanged(msg); });
    // Get the current state of the Present property.
    updatePresence();
}

void PowerSupply::updatePresence()
{
    try
    {
        present = getPresence(bus, inventoryPath);
        if (present)
        {
            if (nullptr == pmbusIntf)
            { // createPMBus()
                std::ostringstream ss;
                ss << std::hex << std::setw(4) << std::setfill('0') << i2caddr;
                std::string addrStr = ss.str();
                pmbusIntf = phosphor::pmbus::createPMBus(i2cbus, addrStr);
            }
        }
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

    if ((present) && (readFail < LOG_LIMIT))
    {
        try
        {
            statusWord = pmbusIntf->read(STATUS_WORD, Type::Debug);
            // Read worked, reset the fail count.
            readFail = 0;

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
            readFail++;
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
    readFail = 0;
    faultLogged = false;

    // The PMBus device driver does not allow for writing CLEAR_FAULTS
    // directly. However, the pmbus hwmon device driver code will send a
    // CLEAR_FAULTS after reading from any of the hwmon "files" in sysfs, so
    // reading in1_input should result in clearing the fault bits in
    // STATUS_BYTE/STATUS_WORD.
    // I do not care what the return value is.
    if (present)
    {
        try
        {
            static_cast<void>(
                pmbusIntf->read("in1_input", phosphor::pmbus::Type::Hwmon));
        }
        catch (ReadFailure& e)
        {
            // Since I do not care what the return value is, I really do not
            // care much if it gets a ReadFailure either. However, this should
            // not prevent the application from continuing to run, so catching
            // the read failure.
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
            // TODO: Immediately trying to read or write the "files" causes read
            // or write failures.
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(20ms);
            if (nullptr == pmbusIntf)
            {
                std::ostringstream ss;
                ss << std::hex << std::setw(4) << std::setfill('0') << i2caddr;
                std::string addrStr = ss.str();
                pmbusIntf = phosphor::pmbus::createPMBus(i2cbus, addrStr);
            }
            else
            {
                pmbusIntf->findHwmonDir();
            }
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
    using PropertyMap =
        std::map<std::string,
                 std::variant<std::string, std::vector<uint8_t>, bool>>;
    PropertyMap assetProps;
    PropertyMap operProps;
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
            fwVersion =
                pmbusIntf->readString(FW_VERSION, Type::HwmonDeviceDebug);
            versionProps.emplace(VERSION_PROP, fwVersion);
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

        // Update the Functional
        operProps.emplace(FUNCTIONAL_PROP, present);
        interfaces.emplace(OPERATIONAL_STATE_IFACE, std::move(operProps));

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
            log<level::ERR>(
                std::string(e.what() + std::string(" PATH=") + inventoryPath)
                    .c_str());
        }
#endif
    }
}

} // namespace phosphor::power::psu
