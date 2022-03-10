#include "config.h"

#include "power_supply.hpp"

#include "types.hpp"
#include "util.hpp"

#include <fmt/format.h>

#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <chrono>  // sleep_for()
#include <cstdint> // uint8_t...
#include <fstream>
#include <thread> // sleep_for()

namespace phosphor::power::psu
{
// Amount of time in milliseconds to delay between power supply going from
// missing to present before running the bind command(s).
constexpr auto bindDelay = 1000;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;

PowerSupply::PowerSupply(sdbusplus::bus::bus& bus, const std::string& invpath,
                         std::uint8_t i2cbus, std::uint16_t i2caddr,
                         const std::string& gpioLineName) :
    bus(bus),
    inventoryPath(invpath), bindPath("/sys/bus/i2c/drivers/ibm-cffps")
{
    if (inventoryPath.empty())
    {
        throw std::invalid_argument{"Invalid empty inventoryPath"};
    }

    if (gpioLineName.empty())
    {
        throw std::invalid_argument{"Invalid empty gpioLineName"};
    }

    log<level::DEBUG>(fmt::format("gpioLineName: {}", gpioLineName).c_str());
    presenceGPIO = createGPIO(gpioLineName);

    std::ostringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << i2caddr;
    std::string addrStr = ss.str();
    std::string busStr = std::to_string(i2cbus);
    bindDevice = busStr;
    bindDevice.append("-");
    bindDevice.append(addrStr);

    pmbusIntf = phosphor::pmbus::createPMBus(i2cbus, addrStr);

    // Get the current state of the Present property.
    try
    {
        updatePresenceGPIO();
    }
    catch (...)
    {
        // If the above attempt to use the GPIO failed, it likely means that the
        // GPIOs are in use by the kernel, meaning it is using gpio-keys.
        // So, I should rely on phosphor-gpio-presence to update D-Bus, and
        // work that way for power supply presence.
        presenceGPIO = nullptr;
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
                sdbusplus::bus::match::rules::argNpath(0, inventoryPath),
            [this](auto& msg) { this->inventoryAdded(msg); });

        updatePresence();
        updateInventory();
    }
}

void PowerSupply::bindOrUnbindDriver(bool present)
{
    auto action = (present) ? "bind" : "unbind";
    auto path = bindPath / action;

    if (present)
    {
        log<level::INFO>(
            fmt::format("Binding device driver. path: {} device: {}",
                        path.string(), bindDevice)
                .c_str());
    }
    else
    {
        log<level::INFO>(
            fmt::format("Unbinding device driver. path: {} device: {}",
                        path.string(), bindDevice)
                .c_str());
    }

    std::ofstream file;

    file.exceptions(std::ofstream::failbit | std::ofstream::badbit |
                    std::ofstream::eofbit);

    try
    {
        file.open(path);
        file << bindDevice;
        file.close();
    }
    catch (const std::exception& e)
    {
        auto err = errno;

        log<level::ERR>(
            fmt::format("Failed binding or unbinding device. errno={}", err)
                .c_str());
    }
}

void PowerSupply::updatePresence()
{
    try
    {
        present = getPresence(bus, inventoryPath);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        // Relying on property change or interface added to retry.
        // Log an informational trace to the journal.
        log<level::INFO>(
            fmt::format("D-Bus property {} access failure exception",
                        inventoryPath)
                .c_str());
    }
}

void PowerSupply::updatePresenceGPIO()
{
    bool presentOld = present;

    try
    {
        if (presenceGPIO->read() > 0)
        {
            present = true;
        }
        else
        {
            present = false;
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format("presenceGPIO read fail: {}", e.what()).c_str());
        throw;
    }

    if (presentOld != present)
    {
        log<level::DEBUG>(
            fmt::format("presentOld: {} present: {}", presentOld, present)
                .c_str());

        auto invpath = inventoryPath.substr(strlen(INVENTORY_OBJ_PATH));
        auto const lastSlashPos = invpath.find_last_of('/');
        std::string prettyName = invpath.substr(lastSlashPos + 1);
        setPresence(bus, invpath, present, prettyName);
        updateInventory();

        // Need Functional to already be correct before calling this
        checkAvailability();

        if (present)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(bindDelay));
            bindOrUnbindDriver(present);
            pmbusIntf->findHwmonDir();
            onOffConfig(phosphor::pmbus::ON_OFF_CONFIG_CONTROL_PIN_ONLY);
            clearFaults();
        }
        else
        {
            bindOrUnbindDriver(present);
        }
    }
}

void PowerSupply::analyzeCMLFault()
{
    if (statusWord & phosphor::pmbus::status_word::CML_FAULT)
    {
        if (cmlFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(fmt::format("CML fault: STATUS_WORD = {:#06x}, "
                                            "STATUS_CML = {:#02x}",
                                            statusWord, statusCML)
                                    .c_str());
            }
            cmlFault++;
        }
    }
    else
    {
        cmlFault = 0;
    }
}

void PowerSupply::analyzeInputFault()
{
    if (statusWord & phosphor::pmbus::status_word::INPUT_FAULT_WARN)
    {
        if (inputFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(
                    fmt::format("INPUT fault: STATUS_WORD = {:#06x}, "
                                "STATUS_MFR_SPECIFIC = {:#04x}, "
                                "STATUS_INPUT = {:#04x}",
                                statusWord, statusMFR, statusInput)
                        .c_str());
            }
            inputFault++;
        }
    }

    // If had INPUT/VIN_UV fault, and now off.
    // Trace that odd behavior.
    if (inputFault &&
        !(statusWord & phosphor::pmbus::status_word::INPUT_FAULT_WARN))
    {
        log<level::INFO>(
            fmt::format("INPUT fault cleared: STATUS_WORD = {:#06x}, "
                        "STATUS_MFR_SPECIFIC = {:#04x}, "
                        "STATUS_INPUT = {:#04x}",
                        statusWord, statusMFR, statusInput)
                .c_str());
        inputFault = 0;
    }
}

void PowerSupply::analyzeVoutOVFault()
{
    if (statusWord & phosphor::pmbus::status_word::VOUT_OV_FAULT)
    {
        if (voutOVFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(
                    fmt::format("VOUT_OV_FAULT fault: STATUS_WORD = {:#06x}, "
                                "STATUS_MFR_SPECIFIC = {:#04x}, "
                                "STATUS_VOUT = {:#02x}",
                                statusWord, statusMFR, statusVout)
                        .c_str());
            }

            voutOVFault++;
        }
    }
    else
    {
        voutOVFault = 0;
    }
}

void PowerSupply::analyzeIoutOCFault()
{
    if (statusWord & phosphor::pmbus::status_word::IOUT_OC_FAULT)
    {
        if (ioutOCFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(
                    fmt::format("IOUT fault: STATUS_WORD = {:#06x}, "
                                "STATUS_MFR_SPECIFIC = {:#04x}, "
                                "STATUS_IOUT = {:#04x}",
                                statusWord, statusMFR, statusIout)
                        .c_str());
            }

            ioutOCFault++;
        }
    }
    else
    {
        ioutOCFault = 0;
    }
}

void PowerSupply::analyzeVoutUVFault()
{
    if ((statusWord & phosphor::pmbus::status_word::VOUT_FAULT) &&
        !(statusWord & phosphor::pmbus::status_word::VOUT_OV_FAULT))
    {
        if (voutUVFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(
                    fmt::format("VOUT_UV_FAULT fault: STATUS_WORD = {:#06x}, "
                                "STATUS_MFR_SPECIFIC = {:#04x}, "
                                "STATUS_VOUT = {:#04x}",
                                statusWord, statusMFR, statusVout)
                        .c_str());
            }
            voutUVFault++;
        }
    }
    else
    {
        voutUVFault = 0;
    }
}

void PowerSupply::analyzeFanFault()
{
    if (statusWord & phosphor::pmbus::status_word::FAN_FAULT)
    {
        if (fanFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(fmt::format("FANS fault/warning: "
                                            "STATUS_WORD = {:#06x}, "
                                            "STATUS_MFR_SPECIFIC = {:#04x}, "
                                            "STATUS_FANS_1_2 = {:#04x}",
                                            statusWord, statusMFR, statusFans12)
                                    .c_str());
            }
            fanFault++;
        }
    }
    else
    {
        fanFault = 0;
    }
}

void PowerSupply::analyzeTemperatureFault()
{
    if (statusWord & phosphor::pmbus::status_word::TEMPERATURE_FAULT_WARN)
    {
        if (tempFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(fmt::format("TEMPERATURE fault/warning: "
                                            "STATUS_WORD = {:#06x}, "
                                            "STATUS_MFR_SPECIFIC = {:#04x}, "
                                            "STATUS_TEMPERATURE = {:#04x}",
                                            statusWord, statusMFR,
                                            statusTemperature)
                                    .c_str());
            }
            tempFault++;
        }
    }
    else
    {
        tempFault = 0;
    }
}

void PowerSupply::analyzePgoodFault()
{
    if ((statusWord & phosphor::pmbus::status_word::POWER_GOOD_NEGATED) ||
        (statusWord & phosphor::pmbus::status_word::UNIT_IS_OFF))
    {
        if (pgoodFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(fmt::format("PGOOD fault: "
                                            "STATUS_WORD = {:#06x}, "
                                            "STATUS_MFR_SPECIFIC = {:#04x}",
                                            statusWord, statusMFR)
                                    .c_str());
            }
            pgoodFault++;
        }
    }
    else
    {
        pgoodFault = 0;
    }
}

void PowerSupply::determineMFRFault()
{
    if (bindPath.string().find("ibm-cffps") != std::string::npos)
    {
        // IBM MFR_SPECIFIC[4] is PS_Kill fault
        if (statusMFR & 0x10)
        {
            if (psKillFault < DEGLITCH_LIMIT)
            {
                psKillFault++;
            }
        }
        else
        {
            psKillFault = 0;
        }
        // IBM MFR_SPECIFIC[6] is 12Vcs fault.
        if (statusMFR & 0x40)
        {
            if (ps12VcsFault < DEGLITCH_LIMIT)
            {
                ps12VcsFault++;
            }
        }
        else
        {
            ps12VcsFault = 0;
        }
        // IBM MFR_SPECIFIC[7] is 12V Current-Share fault.
        if (statusMFR & 0x80)
        {
            if (psCS12VFault < DEGLITCH_LIMIT)
            {
                psCS12VFault++;
            }
        }
        else
        {
            psCS12VFault = 0;
        }
    }
}

void PowerSupply::analyzeMFRFault()
{
    if (statusWord & phosphor::pmbus::status_word::MFR_SPECIFIC_FAULT)
    {
        if (mfrFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(fmt::format("MFR fault: "
                                            "STATUS_WORD = {:#06x} "
                                            "STATUS_MFR_SPECIFIC = {:#04x}",
                                            statusWord, statusMFR)
                                    .c_str());
            }
            mfrFault++;
        }

        determineMFRFault();
    }
    else
    {
        mfrFault = 0;
    }
}

void PowerSupply::analyzeVinUVFault()
{
    if (statusWord & phosphor::pmbus::status_word::VIN_UV_FAULT)
    {
        if (vinUVFault < DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(
                    fmt::format("VIN_UV fault: STATUS_WORD = {:#06x}, "
                                "STATUS_MFR_SPECIFIC = {:#04x}, "
                                "STATUS_INPUT = {:#04x}",
                                statusWord, statusMFR, statusInput)
                        .c_str());
            }
            vinUVFault++;
        }
    }

    if (vinUVFault &&
        !(statusWord & phosphor::pmbus::status_word::VIN_UV_FAULT))
    {
        log<level::INFO>(
            fmt::format("VIN_UV fault cleared: STATUS_WORD = {:#06x}, "
                        "STATUS_MFR_SPECIFIC = {:#04x}, "
                        "STATUS_INPUT = {:#04x}",
                        statusWord, statusMFR, statusInput)
                .c_str());
        vinUVFault = 0;
    }
}

void PowerSupply::analyze()
{
    using namespace phosphor::pmbus;

    if (presenceGPIO)
    {
        updatePresenceGPIO();
    }

    if (present)
    {
        try
        {
            statusWordOld = statusWord;
            statusWord = pmbusIntf->read(STATUS_WORD, Type::Debug,
                                         (readFail < LOG_LIMIT));
            // Read worked, reset the fail count.
            readFail = 0;

            if (statusWord)
            {
                statusInput = pmbusIntf->read(STATUS_INPUT, Type::Debug);
                statusMFR = pmbusIntf->read(STATUS_MFR, Type::Debug);
                statusCML = pmbusIntf->read(STATUS_CML, Type::Debug);
                auto status0Vout = pmbusIntf->insertPageNum(STATUS_VOUT, 0);
                statusVout = pmbusIntf->read(status0Vout, Type::Debug);
                statusIout = pmbusIntf->read(STATUS_IOUT, Type::Debug);
                statusFans12 = pmbusIntf->read(STATUS_FANS_1_2, Type::Debug);
                statusTemperature =
                    pmbusIntf->read(STATUS_TEMPERATURE, Type::Debug);

                analyzeCMLFault();

                analyzeInputFault();

                analyzeVoutOVFault();

                analyzeIoutOCFault();

                analyzeVoutUVFault();

                analyzeFanFault();

                analyzeTemperatureFault();

                analyzePgoodFault();

                analyzeMFRFault();

                analyzeVinUVFault();
            }
            else
            {
                if (statusWord != statusWordOld)
                {
                    log<level::INFO>(fmt::format("STATUS_WORD = {:#06x} {}",
                                                 statusWord, inventoryPath)
                                         .c_str());
                }

                // if INPUT/VIN_UV fault was on, it cleared, trace it.
                if (inputFault)
                {
                    log<level::INFO>(
                        fmt::format(
                            "INPUT fault cleared: STATUS_WORD = {:#06x}",
                            statusWord)
                            .c_str());
                }

                if (vinUVFault)
                {
                    log<level::INFO>(
                        fmt::format("VIN_UV cleared: STATUS_WORD = {:#06x}",
                                    statusWord)
                            .c_str());
                }

                if (pgoodFault > 0)
                {
                    log<level::INFO>(fmt::format("pgoodFault cleared path: {}",
                                                 inventoryPath)
                                         .c_str());
                }

                clearFaultFlags();
            }

            // Save off old inputVoltage value.
            // Get latest inputVoltage.
            // If voltage went from below minimum, and now is not, clear faults.
            // Note: getInputVoltage() has its own try/catch.
            int inputVoltageOld = inputVoltage;
            double actualInputVoltage;
            getInputVoltage(actualInputVoltage, inputVoltage);
            if ((inputVoltageOld == in_input::VIN_VOLTAGE_0) &&
                (inputVoltage != in_input::VIN_VOLTAGE_0))
            {
                log<level::INFO>(
                    fmt::format(
                        "READ_VIN back in range: inputVoltageOld = {} inputVoltage = {}",
                        inputVoltageOld, inputVoltage)
                        .c_str());
                clearFaults();
            }
            else if (vinUVFault && (inputVoltage != in_input::VIN_VOLTAGE_0))
            {
                log<level::INFO>(
                    fmt::format("CLEAR_FAULTS: vinUVFault {} inputVoltage {}",
                                vinUVFault, inputVoltage)
                        .c_str());
                // Do we have a VIN_UV fault latched that can now be cleared
                // due to voltage back in range? Attempt to clear all
                // faults, re-check faults on next call.
                clearFaults();
            }
            else if (inputVoltageOld != inputVoltage)
            {
                log<level::INFO>(
                    fmt::format(
                        "READ_VIN change: inputVoltageOld = {} inputVoltage = {}",
                        inputVoltageOld, inputVoltage)
                        .c_str());
            }

            checkAvailability();
        }
        catch (const ReadFailure& e)
        {
            if (readFail < SIZE_MAX)
            {
                readFail++;
            }
            if (readFail == LOG_LIMIT)
            {
                phosphor::logging::commit<ReadFailure>();
            }
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
            // journal if the write fails. If the ON_OFF_CONFIG is not setup
            // as desired, later fault detection and analysis code should
            // catch any of the fall out. We should not need to terminate
            // the application if this write fails.
        }
    }
}

void PowerSupply::clearFaults()
{
    log<level::DEBUG>(
        fmt::format("clearFaults() inventoryPath: {}", inventoryPath).c_str());
    faultLogged = false;
    // The PMBus device driver does not allow for writing CLEAR_FAULTS
    // directly. However, the pmbus hwmon device driver code will send a
    // CLEAR_FAULTS after reading from any of the hwmon "files" in sysfs, so
    // reading in1_input should result in clearing the fault bits in
    // STATUS_BYTE/STATUS_WORD.
    // I do not care what the return value is.
    if (present)
    {
        clearFaultFlags();
        checkAvailability();
        readFail = 0;

        try
        {
            static_cast<void>(
                pmbusIntf->read("in1_input", phosphor::pmbus::Type::Hwmon));

            std::vector<uint8_t> dontCare{1};
            pmbusIntf->writeBinary(phosphor::pmbus::CLEAR_FAULTS, dontCare,
                                   phosphor::pmbus::Type::HwmonDeviceDebug);
        }
        catch (const ReadFailure& e)
        {
            // Since I do not care what the return value is, I really do not
            // care much if it gets a ReadFailure either. However, this
            // should not prevent the application from continuing to run, so
            // catching the read failure.
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
            // TODO: Immediately trying to read or write the "files" causes
            // read or write failures.
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(20ms);
            pmbusIntf->findHwmonDir();
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
        checkAvailability();
    }
}

void PowerSupply::inventoryAdded(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path path;
    msg.read(path);
    // Make sure the signal is for the PSU inventory path
    if (path == inventoryPath)
    {
        std::map<std::string, std::map<std::string, std::variant<bool>>>
            interfaces;
        // Get map of interfaces and their properties
        msg.read(interfaces);

        auto properties = interfaces.find(INVENTORY_IFACE);
        if (properties != interfaces.end())
        {
            auto property = properties->second.find(PRESENT_PROP);
            if (property != properties->second.end())
            {
                present = std::get<bool>(property->second);

                log<level::INFO>(fmt::format("Power Supply {} Present {}",
                                             inventoryPath, present)
                                     .c_str());

                updateInventory();
                checkAvailability();
            }
        }
    }
}

void PowerSupply::updateInventory()
{
    using namespace phosphor::pmbus;

#if IBM_VPD
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
    log<level::DEBUG>(
        fmt::format("updateInventory() inventoryPath: {}", inventoryPath)
            .c_str());

    if (present)
    {
        // TODO: non-IBM inventory updates?

#if IBM_VPD
        try
        {
            ccin = pmbusIntf->readString(CCIN, Type::HwmonDeviceDebug);
            assetProps.emplace(MODEL_PROP, ccin);
            modelName = ccin;
        }
        catch (const ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure,
            // path...
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
        catch (const ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure,
            // path...
        }

        try
        {
            fn = pmbusIntf->readString(FRU_NUMBER, Type::HwmonDeviceDebug);
            assetProps.emplace(SPARE_PN_PROP, fn);
        }
        catch (const ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure,
            // path...
        }

        try
        {
            header =
                pmbusIntf->readString(SERIAL_HEADER, Type::HwmonDeviceDebug);
            sn = pmbusIntf->readString(SERIAL_NUMBER, Type::HwmonDeviceDebug);
            assetProps.emplace(SN_PROP, sn);
        }
        catch (const ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure,
            // path...
        }

        try
        {
            fwVersion =
                pmbusIntf->readString(FW_VERSION, Type::HwmonDeviceDebug);
            versionProps.emplace(VERSION_PROP, fwVersion);
        }
        catch (const ReadFailure& e)
        {
            // Ignore the read failure, let pmbus code indicate failure,
            // path...
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

        // Populate the VINI Resource Type (RT) keyword
        ipzvpdVINIProps.emplace("RT", std::vector<uint8_t>{'V', 'I', 'N', 'I'});

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

        // Populate the DINF Resource Type (RT) keyword
        ipzvpdDINFProps.emplace("RT", std::vector<uint8_t>{'D', 'I', 'N', 'F'});

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
        catch (const std::exception& e)
        {
            log<level::ERR>(
                std::string(e.what() + std::string(" PATH=") + inventoryPath)
                    .c_str());
        }
#endif
    }
}

void PowerSupply::getInputVoltage(double& actualInputVoltage,
                                  int& inputVoltage) const
{
    using namespace phosphor::pmbus;

    actualInputVoltage = in_input::VIN_VOLTAGE_0;
    inputVoltage = in_input::VIN_VOLTAGE_0;

    if (present)
    {
        try
        {
            // Read input voltage in millivolts
            auto inputVoltageStr = pmbusIntf->readString(READ_VIN, Type::Hwmon);

            // Convert to volts
            actualInputVoltage = std::stod(inputVoltageStr) / 1000;

            // Calculate the voltage based on voltage thresholds
            if (actualInputVoltage < in_input::VIN_VOLTAGE_MIN)
            {
                inputVoltage = in_input::VIN_VOLTAGE_0;
            }
            else if (actualInputVoltage < in_input::VIN_VOLTAGE_110_THRESHOLD)
            {
                inputVoltage = in_input::VIN_VOLTAGE_110;
            }
            else
            {
                inputVoltage = in_input::VIN_VOLTAGE_220;
            }
        }
        catch (const std::exception& e)
        {
            log<level::ERR>(
                fmt::format("READ_VIN read error: {}", e.what()).c_str());
        }
    }
}

void PowerSupply::checkAvailability()
{
    bool origAvailability = available;
    available = present && !hasInputFault() && !hasVINUVFault() &&
                !hasPSKillFault() && !hasIoutOCFault();

    if (origAvailability != available)
    {
        auto invpath = inventoryPath.substr(strlen(INVENTORY_OBJ_PATH));
        phosphor::power::psu::setAvailable(bus, invpath, available);

        // Check if the health rollup needs to change based on the
        // new availability value.
        phosphor::power::psu::handleChassisHealthRollup(bus, inventoryPath,
                                                        !available);
    }
}

} // namespace phosphor::power::psu
