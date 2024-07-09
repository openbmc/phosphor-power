#include "config.h"

#include "power_supply.hpp"

#include "types.hpp"
#include "util.hpp"

#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <chrono>  // sleep_for()
#include <cmath>
#include <cstdint> // uint8_t...
#include <format>
#include <fstream>
#include <regex>
#include <thread> // sleep_for()

namespace phosphor::power::psu
{
// Amount of time in milliseconds to delay between power supply going from
// missing to present before running the bind command(s).
constexpr auto bindDelay = 1000;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;

PowerSupply::PowerSupply(sdbusplus::bus_t& bus, const std::string& invpath,
                         std::uint8_t i2cbus, std::uint16_t i2caddr,
                         const std::string& driver,
                         const std::string& gpioLineName,
                         std::function<bool()>&& callback) :
    bus(bus),
    inventoryPath(invpath), bindPath("/sys/bus/i2c/drivers/" + driver),
    isPowerOn(std::move(callback)), driverName(driver)
{
    if (inventoryPath.empty())
    {
        throw std::invalid_argument{"Invalid empty inventoryPath"};
    }

    if (gpioLineName.empty())
    {
        throw std::invalid_argument{"Invalid empty gpioLineName"};
    }

    shortName = findShortName(inventoryPath);

    log<level::DEBUG>(
        std::format("{} gpioLineName: {}", shortName, gpioLineName).c_str());
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
        setupSensors();
    }

    setInputVoltageRating();
}

void PowerSupply::bindOrUnbindDriver(bool present)
{
    // Symbolic link to the device will exist if the driver is bound.
    // So exit no action required if both the link and PSU are present
    // or neither is present.
    namespace fs = std::filesystem;
    fs::path path;
    auto action = (present) ? "bind" : "unbind";

    // This case should not happen, if no device driver name return.
    if (driverName.empty())
    {
        log<level::INFO>("No device driver name found");
        return;
    }
    if (bindPath.string().find(driverName) != std::string::npos)
    {
        // bindPath has driver name
        path = bindPath / action;
    }
    else
    {
        // Add driver name to bindPath
        path = bindPath / driverName / action;
        bindPath = bindPath / driverName;
    }

    if ((std::filesystem::exists(bindPath / bindDevice) && present) ||
        (!std::filesystem::exists(bindPath / bindDevice) && !present))
    {
        return;
    }
    if (present)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(bindDelay));
        log<level::INFO>(
            std::format("Binding device driver. path: {} device: {}",
                        path.string(), bindDevice)
                .c_str());
    }
    else
    {
        log<level::INFO>(
            std::format("Unbinding device driver. path: {} device: {}",
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
            std::format("Failed binding or unbinding device. errno={}", err)
                .c_str());
    }
}

void PowerSupply::updatePresence()
{
    try
    {
        present = getPresence(bus, inventoryPath);
    }
    catch (const sdbusplus::exception_t& e)
    {
        // Relying on property change or interface added to retry.
        // Log an informational trace to the journal.
        log<level::INFO>(
            std::format("D-Bus property {} access failure exception",
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
            std::format("presenceGPIO read fail: {}", e.what()).c_str());
        throw;
    }

    if (presentOld != present)
    {
        log<level::DEBUG>(std::format("{} presentOld: {} present: {}",
                                      shortName, presentOld, present)
                              .c_str());

        auto invpath = inventoryPath.substr(strlen(INVENTORY_OBJ_PATH));

        bindOrUnbindDriver(present);
        if (present)
        {
            // If the power supply was present, then missing, and present again,
            // the hwmon path may have changed. We will need the correct/updated
            // path before any reads or writes are attempted.
            pmbusIntf->findHwmonDir();
        }

        setPresence(bus, invpath, present, shortName);
        setupSensors();
        updateInventory();

        // Need Functional to already be correct before calling this.
        checkAvailability();

        if (present)
        {
            onOffConfig(phosphor::pmbus::ON_OFF_CONFIG_CONTROL_PIN_ONLY);
            clearFaults();
            // Indicate that the input history data and timestamps between all
            // the power supplies that are present in the system need to be
            // synchronized.
            syncHistoryRequired = true;
        }
        else
        {
            setSensorsNotAvailable();
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
                log<level::ERR>(
                    std::format("{} CML fault: STATUS_WORD = {:#06x}, "
                                "STATUS_CML = {:#02x}",
                                shortName, statusWord, statusCML)
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
                    std::format("{} INPUT fault: STATUS_WORD = {:#06x}, "
                                "STATUS_MFR_SPECIFIC = {:#04x}, "
                                "STATUS_INPUT = {:#04x}",
                                shortName, statusWord, statusMFR, statusInput)
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
            std::format("{} INPUT fault cleared: STATUS_WORD = {:#06x}, "
                        "STATUS_MFR_SPECIFIC = {:#04x}, "
                        "STATUS_INPUT = {:#04x}",
                        shortName, statusWord, statusMFR, statusInput)
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
                    std::format(
                        "{} VOUT_OV_FAULT fault: STATUS_WORD = {:#06x}, "
                        "STATUS_MFR_SPECIFIC = {:#04x}, "
                        "STATUS_VOUT = {:#02x}",
                        shortName, statusWord, statusMFR, statusVout)
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
                    std::format("{} IOUT fault: STATUS_WORD = {:#06x}, "
                                "STATUS_MFR_SPECIFIC = {:#04x}, "
                                "STATUS_IOUT = {:#04x}",
                                shortName, statusWord, statusMFR, statusIout)
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
                    std::format(
                        "{} VOUT_UV_FAULT fault: STATUS_WORD = {:#06x}, "
                        "STATUS_MFR_SPECIFIC = {:#04x}, "
                        "STATUS_VOUT = {:#04x}",
                        shortName, statusWord, statusMFR, statusVout)
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
                log<level::ERR>(std::format("{} FANS fault/warning: "
                                            "STATUS_WORD = {:#06x}, "
                                            "STATUS_MFR_SPECIFIC = {:#04x}, "
                                            "STATUS_FANS_1_2 = {:#04x}",
                                            shortName, statusWord, statusMFR,
                                            statusFans12)
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
                log<level::ERR>(std::format("{} TEMPERATURE fault/warning: "
                                            "STATUS_WORD = {:#06x}, "
                                            "STATUS_MFR_SPECIFIC = {:#04x}, "
                                            "STATUS_TEMPERATURE = {:#04x}",
                                            shortName, statusWord, statusMFR,
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
        if (pgoodFault < PGOOD_DEGLITCH_LIMIT)
        {
            if (statusWord != statusWordOld)
            {
                log<level::ERR>(std::format("{} PGOOD fault: "
                                            "STATUS_WORD = {:#06x}, "
                                            "STATUS_MFR_SPECIFIC = {:#04x}",
                                            shortName, statusWord, statusMFR)
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
    if (bindPath.string().find(IBMCFFPS_DD_NAME) != std::string::npos)
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
                log<level::ERR>(std::format("{} MFR fault: "
                                            "STATUS_WORD = {:#06x} "
                                            "STATUS_MFR_SPECIFIC = {:#04x}",
                                            shortName, statusWord, statusMFR)
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
                    std::format("{} VIN_UV fault: STATUS_WORD = {:#06x}, "
                                "STATUS_MFR_SPECIFIC = {:#04x}, "
                                "STATUS_INPUT = {:#04x}",
                                shortName, statusWord, statusMFR, statusInput)
                        .c_str());
            }
            vinUVFault++;
        }
        // Remember that this PSU has seen an AC fault
        acFault = AC_FAULT_LIMIT;
    }
    else
    {
        if (vinUVFault != 0)
        {
            log<level::INFO>(
                std::format("{} VIN_UV fault cleared: STATUS_WORD = {:#06x}, "
                            "STATUS_MFR_SPECIFIC = {:#04x}, "
                            "STATUS_INPUT = {:#04x}",
                            shortName, statusWord, statusMFR, statusInput)
                    .c_str());
            vinUVFault = 0;
        }
        // No AC fail, decrement counter
        if (acFault != 0)
        {
            --acFault;
        }
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
                if (bindPath.string().find(IBMCFFPS_DD_NAME) !=
                    std::string::npos)
                {
                    statusMFR = pmbusIntf->read(STATUS_MFR, Type::Debug);
                }
                statusCML = pmbusIntf->read(STATUS_CML, Type::Debug);
                auto status0Vout = pmbusIntf->insertPageNum(STATUS_VOUT, 0);
                statusVout = pmbusIntf->read(status0Vout, Type::Debug);
                statusIout = pmbusIntf->read(STATUS_IOUT, Type::Debug);
                statusFans12 = pmbusIntf->read(STATUS_FANS_1_2, Type::Debug);
                statusTemperature = pmbusIntf->read(STATUS_TEMPERATURE,
                                                    Type::Debug);

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
                    log<level::INFO>(std::format("{} STATUS_WORD = {:#06x}",
                                                 shortName, statusWord)
                                         .c_str());
                }

                // if INPUT/VIN_UV fault was on, it cleared, trace it.
                if (inputFault)
                {
                    log<level::INFO>(
                        std::format(
                            "{} INPUT fault cleared: STATUS_WORD = {:#06x}",
                            shortName, statusWord)
                            .c_str());
                }

                if (vinUVFault)
                {
                    log<level::INFO>(
                        std::format("{} VIN_UV cleared: STATUS_WORD = {:#06x}",
                                    shortName, statusWord)
                            .c_str());
                }

                if (pgoodFault > 0)
                {
                    log<level::INFO>(
                        std::format("{} pgoodFault cleared", shortName)
                            .c_str());
                }

                clearFaultFlags();
                // No AC fail, decrement counter
                if (acFault != 0)
                {
                    --acFault;
                }
            }

            // Save off old inputVoltage value.
            // Get latest inputVoltage.
            // If voltage went from below minimum, and now is not, clear faults.
            // Note: getInputVoltage() has its own try/catch.
            int inputVoltageOld = inputVoltage;
            double actualInputVoltageOld = actualInputVoltage;
            getInputVoltage(actualInputVoltage, inputVoltage);
            if ((inputVoltageOld == in_input::VIN_VOLTAGE_0) &&
                (inputVoltage != in_input::VIN_VOLTAGE_0))
            {
                log<level::INFO>(
                    std::format(
                        "{} READ_VIN back in range: actualInputVoltageOld = {} "
                        "actualInputVoltage = {}",
                        shortName, actualInputVoltageOld, actualInputVoltage)
                        .c_str());
                clearVinUVFault();
            }
            else if (vinUVFault && (inputVoltage != in_input::VIN_VOLTAGE_0))
            {
                log<level::INFO>(
                    std::format(
                        "{} CLEAR_FAULTS: vinUVFault {} actualInputVoltage {}",
                        shortName, vinUVFault, actualInputVoltage)
                        .c_str());
                // Do we have a VIN_UV fault latched that can now be cleared
                // due to voltage back in range? Attempt to clear the
                // fault(s), re-check faults on next call.
                clearVinUVFault();
            }
            else if (std::abs(actualInputVoltageOld - actualInputVoltage) >
                     10.0)
            {
                log<level::INFO>(
                    std::format(
                        "{} actualInputVoltageOld = {} actualInputVoltage = {}",
                        shortName, actualInputVoltageOld, actualInputVoltage)
                        .c_str());
            }

            monitorSensors();

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

    if (present && driverName != ACBEL_FSG032_DD_NAME)
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

void PowerSupply::clearVinUVFault()
{
    // Read in1_lcrit_alarm to clear bits 3 and 4 of STATUS_INPUT.
    // The fault bits in STAUTS_INPUT roll-up to STATUS_WORD. Clearing those
    // bits in STATUS_INPUT should result in the corresponding STATUS_WORD bits
    // also clearing.
    //
    // Do not care about return value. Should be 1 if active, 0 if not.
    if (driverName != ACBEL_FSG032_DD_NAME)
    {
        static_cast<void>(
            pmbusIntf->read("in1_lcrit_alarm", phosphor::pmbus::Type::Hwmon));
    }
    else
    {
        static_cast<void>(
            pmbusIntf->read("curr1_crit_alarm", phosphor::pmbus::Type::Hwmon));
    }
    vinUVFault = 0;
}

void PowerSupply::clearFaults()
{
    log<level::DEBUG>(
        std::format("clearFaults() inventoryPath: {}", inventoryPath).c_str());
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
            clearVinUVFault();
            static_cast<void>(
                pmbusIntf->read("in1_input", phosphor::pmbus::Type::Hwmon));
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

void PowerSupply::inventoryChanged(sdbusplus::message_t& msg)
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

void PowerSupply::inventoryAdded(sdbusplus::message_t& msg)
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

                log<level::INFO>(std::format("Power Supply {} Present {}",
                                             inventoryPath, present)
                                     .c_str());

                updateInventory();
                checkAvailability();
            }
        }
    }
}

auto PowerSupply::readVPDValue(const std::string& vpdName,
                               const phosphor::pmbus::Type& type,
                               const std::size_t& vpdSize)
{
    std::string vpdValue;
    const std::regex illegalVPDRegex = std::regex("[^[:alnum:]]",
                                                  std::regex::basic);

    try
    {
        vpdValue = pmbusIntf->readString(vpdName, type);
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

    if (vpdValue.size() != vpdSize)
    {
        log<level::INFO>(std::format("{} {} resize needed. size: {}", shortName,
                                     vpdName, vpdValue.size())
                             .c_str());
        vpdValue.resize(vpdSize, ' ');
    }

    // Replace any illegal values with space(s).
    std::regex_replace(vpdValue.begin(), vpdValue.begin(), vpdValue.end(),
                       illegalVPDRegex, " ");

    return vpdValue;
}

void PowerSupply::updateInventory()
{
    using namespace phosphor::pmbus;

#if IBM_VPD
    std::string pn;
    std::string fn;
    std::string header;
    std::string sn;
    // The IBM power supply splits the full serial number into two parts.
    // Each part is 6 bytes long, which should match up with SN_KW_SIZE.
    const auto HEADER_SIZE = 6;
    const auto SERIAL_SIZE = 6;
    // The IBM PSU firmware version size is a bit complicated. It was originally
    // 1-byte, per command. It was later expanded to 2-bytes per command, then
    // up to 8-bytes per command. The device driver only reads up to 2 bytes per
    // command, but combines all three of the 2-byte reads, or all 4 of the
    // 1-byte reads into one string. So, the maximum size expected is 6 bytes.
    // However, it is formatted by the driver as a hex string with two ASCII
    // characters per byte.  So the maximum ASCII string size is 12.
    const auto IBMCFFPS_FW_VERSION_SIZE = 12;
    const auto ACBEL_FSG032_FW_VERSION_SIZE = 6;

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
        std::format("updateInventory() inventoryPath: {}", inventoryPath)
            .c_str());

    if (present)
    {
        // TODO: non-IBM inventory updates?

#if IBM_VPD
        if (driverName == ACBEL_FSG032_DD_NAME)
        {
            getPsuVpdFromDbus("CC", modelName);
            getPsuVpdFromDbus("PN", pn);
            getPsuVpdFromDbus("FN", fn);
            getPsuVpdFromDbus("SN", sn);
            assetProps.emplace(SN_PROP, sn);
            fwVersion = readVPDValue(FW_VERSION, Type::Debug,
                                     ACBEL_FSG032_FW_VERSION_SIZE);
            versionProps.emplace(VERSION_PROP, fwVersion);
        }
        else
        {
            modelName = readVPDValue(CCIN, Type::HwmonDeviceDebug, CC_KW_SIZE);
            pn = readVPDValue(PART_NUMBER, Type::Debug, PN_KW_SIZE);
            fn = readVPDValue(FRU_NUMBER, Type::Debug, FN_KW_SIZE);

            header = readVPDValue(SERIAL_HEADER, Type::Debug, HEADER_SIZE);
            sn = readVPDValue(SERIAL_NUMBER, Type::Debug, SERIAL_SIZE);
            assetProps.emplace(SN_PROP, header + sn);
            fwVersion = readVPDValue(FW_VERSION, Type::HwmonDeviceDebug,
                                     IBMCFFPS_FW_VERSION_SIZE);
            versionProps.emplace(VERSION_PROP, fwVersion);
        }

        assetProps.emplace(MODEL_PROP, modelName);
        assetProps.emplace(PN_PROP, pn);
        assetProps.emplace(SPARE_PN_PROP, fn);

        ipzvpdVINIProps.emplace(
            "CC", std::vector<uint8_t>(modelName.begin(), modelName.end()));
        ipzvpdVINIProps.emplace("PN",
                                std::vector<uint8_t>(pn.begin(), pn.end()));
        ipzvpdVINIProps.emplace("FN",
                                std::vector<uint8_t>(fn.begin(), fn.end()));
        std::string header_sn = header + sn;
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
            auto service = util::getService(INVENTORY_OBJ_PATH,
                                            INVENTORY_MGR_IFACE, bus);

            if (service.empty())
            {
                log<level::ERR>("Unable to get inventory manager service");
                return;
            }

            auto method = bus.new_method_call(service.c_str(),
                                              INVENTORY_OBJ_PATH,
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

auto PowerSupply::getMaxPowerOut() const
{
    using namespace phosphor::pmbus;

    auto maxPowerOut = 0;

    if (present)
    {
        try
        {
            // Read max_power_out, should be direct format
            auto maxPowerOutStr = pmbusIntf->readString(MFR_POUT_MAX,
                                                        Type::HwmonDeviceDebug);
            log<level::INFO>(std::format("{} MFR_POUT_MAX read {}", shortName,
                                         maxPowerOutStr)
                                 .c_str());
            maxPowerOut = std::stod(maxPowerOutStr);
        }
        catch (const std::exception& e)
        {
            log<level::ERR>(std::format("{} MFR_POUT_MAX read error: {}",
                                        shortName, e.what())
                                .c_str());
        }
    }

    return maxPowerOut;
}

void PowerSupply::setupSensors()
{
    setupInputPowerPeakSensor();
}

void PowerSupply::setupInputPowerPeakSensor()
{
    if (peakInputPowerSensor || !present ||
        (bindPath.string().find(IBMCFFPS_DD_NAME) == std::string::npos))
    {
        return;
    }

    // This PSU has problems with the input_history command
    if (getMaxPowerOut() == phosphor::pmbus::IBM_CFFPS_1400W)
    {
        return;
    }

    auto sensorPath =
        std::format("/xyz/openbmc_project/sensors/power/ps{}_input_power_peak",
                    shortName.back());

    peakInputPowerSensor = std::make_unique<PowerSensorObject>(
        bus, sensorPath.c_str(), PowerSensorObject::action::defer_emit);

    // The others can remain at the defaults.
    peakInputPowerSensor->functional(true, true);
    peakInputPowerSensor->available(true, true);
    peakInputPowerSensor->value(0, true);
    peakInputPowerSensor->unit(
        sdbusplus::xyz::openbmc_project::Sensor::server::Value::Unit::Watts,
        true);

    auto associations = getSensorAssociations();
    peakInputPowerSensor->associations(associations, true);

    peakInputPowerSensor->emit_object_added();
}

void PowerSupply::setSensorsNotAvailable()
{
    if (peakInputPowerSensor)
    {
        peakInputPowerSensor->value(std::numeric_limits<double>::quiet_NaN());
        peakInputPowerSensor->available(false);
    }
}

void PowerSupply::monitorSensors()
{
    monitorPeakInputPowerSensor();
}

void PowerSupply::monitorPeakInputPowerSensor()
{
    if (!peakInputPowerSensor)
    {
        return;
    }

    constexpr size_t recordSize = 5;
    std::vector<uint8_t> data;

    // Get the peak input power with input history command.
    // New data only shows up every 30s, but just try to read it every 1s
    // anyway so we always have the most up to date value.
    try
    {
        data = pmbusIntf->readBinary(INPUT_HISTORY,
                                     pmbus::Type::HwmonDeviceDebug, recordSize);
    }
    catch (const ReadFailure& e)
    {
        peakInputPowerSensor->value(std::numeric_limits<double>::quiet_NaN());
        peakInputPowerSensor->functional(false);
        throw;
    }

    if (data.size() != recordSize)
    {
        log<level::DEBUG>(
            std::format("Input history command returned {} bytes instead of 5",
                        data.size())
                .c_str());
        peakInputPowerSensor->value(std::numeric_limits<double>::quiet_NaN());
        peakInputPowerSensor->functional(false);
        return;
    }

    // The format is SSAAAAPPPP:
    //   SS = packet sequence number
    //   AAAA = average power (linear format, little endian)
    //   PPPP = peak power (linear format, little endian)
    auto peak = static_cast<uint16_t>(data[4]) << 8 | data[3];
    auto peakPower = linearToInteger(peak);

    peakInputPowerSensor->value(peakPower);
    peakInputPowerSensor->functional(true);
    peakInputPowerSensor->available(true);
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
                std::format("{} READ_VIN read error: {}", shortName, e.what())
                    .c_str());
        }
    }
}

void PowerSupply::checkAvailability()
{
    bool origAvailability = available;
    bool faulted = isPowerOn() && (hasPSKillFault() || hasIoutOCFault());
    available = present && !hasInputFault() && !hasVINUVFault() && !faulted;

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

void PowerSupply::setInputVoltageRating()
{
    if (!present)
    {
        if (inputVoltageRatingIface)
        {
            inputVoltageRatingIface->value(0);
            inputVoltageRatingIface.reset();
        }
        return;
    }

    double inputVoltageValue{};
    int inputVoltageRating{};
    getInputVoltage(inputVoltageValue, inputVoltageRating);

    if (!inputVoltageRatingIface)
    {
        auto path = std::format(
            "/xyz/openbmc_project/sensors/voltage/ps{}_input_voltage_rating",
            shortName.back());

        inputVoltageRatingIface = std::make_unique<SensorObject>(
            bus, path.c_str(), SensorObject::action::defer_emit);

        // Leave other properties at their defaults
        inputVoltageRatingIface->unit(SensorInterface::Unit::Volts, true);
        inputVoltageRatingIface->value(static_cast<double>(inputVoltageRating),
                                       true);

        inputVoltageRatingIface->emit_object_added();
    }
    else
    {
        inputVoltageRatingIface->value(static_cast<double>(inputVoltageRating));
    }
}

void PowerSupply::getPsuVpdFromDbus(const std::string& keyword,
                                    std::string& vpdStr)
{
    try
    {
        std::vector<uint8_t> value;
        vpdStr.clear();
        util::getProperty(VINI_IFACE, keyword, inventoryPath,
                          INVENTORY_MGR_IFACE, bus, value);
        for (char c : value)
        {
            vpdStr += c;
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        log<level::ERR>(
            std::format("Failed getProperty error: {}", e.what()).c_str());
    }
}

double PowerSupply::linearToInteger(uint16_t data)
{
    // The exponent is the first 5 bits, followed by 11 bits of mantissa.
    int8_t exponent = (data & 0xF800) >> 11;
    int16_t mantissa = (data & 0x07FF);

    // If exponent's MSB on, then it's negative.
    // Convert from two's complement.
    if (exponent & 0x10)
    {
        exponent = (~exponent) & 0x1F;
        exponent = (exponent + 1) * -1;
    }

    // If mantissa's MSB on, then it's negative.
    // Convert from two's complement.
    if (mantissa & 0x400)
    {
        mantissa = (~mantissa) & 0x07FF;
        mantissa = (mantissa + 1) * -1;
    }

    auto value = static_cast<double>(mantissa) * pow(2, exponent);
    return value;
}

std::vector<AssociationTuple> PowerSupply::getSensorAssociations()
{
    std::vector<AssociationTuple> associations;

    associations.emplace_back("inventory", "sensors", inventoryPath);

    auto chassis = getChassis(bus, inventoryPath);
    associations.emplace_back("chassis", "all_sensors", std::move(chassis));

    return associations;
}

} // namespace phosphor::power::psu
