/**
 * Copyright © 2019 IBM Corporation
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
#include "updater.hpp"

#include "aei_updater.hpp"
#include "pmbus.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "utils.hpp"
#include "version.hpp"

#include <phosphor-logging/lg2.hpp>

#include <chrono>
#include <fstream>
#include <thread>
#include <vector>
// #include <sdbusplus/bus.hpp>
// #include <utility.hpp>
#include <xyz/openbmc_project/Logging/Create/server.hpp>

#include <iostream>
#include <map>
#include <string>

using namespace phosphor::logging;
namespace util = phosphor::power::util;

namespace updater
{

namespace internal
{

// Define the CRC-8 polynomial (CRC-8-CCITT)
constexpr uint8_t CRC8_POLYNOMIAL = 0x07;
constexpr uint8_t CRC8_INITIAL = 0x00;

// Get the appropriate Updater class instance based PSU model number
std::unique_ptr<updater::Updater> getClassInstance(
    const std::string& model, const std::string& psuInventoryPath,
    const std::string& devPath, const std::string& imageDir)
{
    if (model == "51E9" || model == "51DA")
    {
        return std::make_unique<aeiUpdater::AeiUpdater>(psuInventoryPath,
                                                        devPath, imageDir);
    }
    return std::make_unique<Updater>(psuInventoryPath, devPath, imageDir);
}

// Function to locate FW file with model and extension bin or hex
const std::string getFWFilenamePath(const std::string& directory)
{
    namespace fs = std::filesystem;
    // Get the last part of the directory name (model number)
    std::string model = fs::path(directory).filename().string();
    for (const auto& entry : fs::directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();

            if ((filename.rfind(model, 0) == 0) && (filename.ends_with(".bin")))
            {
                return directory + "/" + filename;
            }
        }
    }
    return "";
}

// Compute CRC-8 checksum for a vector of bytes
uint8_t calculateCRC8(const std::vector<uint8_t>& data)
{
    uint8_t crc = CRC8_INITIAL;

    for (const auto& byte : data)
    {
        crc ^= byte;
        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// Delay execution for a specified number of milliseconds
void delay(const int& milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Convert big endian (32 bit integer) to a vector of little endian.
std::vector<uint8_t> bigEndianToLittleEndian(const uint32_t bigEndianValue)
{
    std::vector<uint8_t> littleEndianBytes(4);

    littleEndianBytes[3] = (bigEndianValue >> 24) & 0xFF;
    littleEndianBytes[2] = (bigEndianValue >> 16) & 0xFF;
    littleEndianBytes[1] = (bigEndianValue >> 8) & 0xFF;
    littleEndianBytes[0] = bigEndianValue & 0xFF;
    return littleEndianBytes;
}

// Validate the existence and size of a firmware file.
bool validateFWFile(const std::string& fileName)
{
    // Ensure the file exists and get the file size.
    if (!std::filesystem::exists(fileName))
    {
        lg2::error("Firmware file not found: {FILE}", "FILE", fileName);
        return false;
    }

    // Check the file size
    auto fileSize = std::filesystem::file_size(fileName);
    if (fileSize == 0)
    {
        lg2::error("Firmware {FILE} is empty", "FILE", fileName);
        return false;
    }
    return true;
}

// Open a firmware file for reading in binary mode.
std::unique_ptr<std::ifstream> openFirmwareFile(const std::string& fileName)
{
    if (fileName.empty())
    {
        lg2::error("Firmware file path is not provided");
        return nullptr;
    }
    auto inputFile =
        std::make_unique<std::ifstream>(fileName, std::ios::binary);
    if (!inputFile->is_open())
    {
        lg2::error("Failed to open firmware file: {FILE}", "FILE", fileName);
        return nullptr;
    }
    return inputFile;
}

// Read firmware bytes from input stream.
std::vector<uint8_t> readFirmwareBytes(std::ifstream& inputFile,
                                       const size_t numberOfBytesToRead)
{
    std::vector<uint8_t> readDataBytes(numberOfBytesToRead, 0xFF);
    try
    {
        // Enable exceptions for failbit and badbit
        inputFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        inputFile.read(reinterpret_cast<char*>(readDataBytes.data()),
                       numberOfBytesToRead);
        size_t bytesRead = inputFile.gcount();
        if (bytesRead != numberOfBytesToRead)
        {
            readDataBytes.resize(bytesRead);
        }
    }
    catch (const std::ios_base::failure& e)
    {
        lg2::error("Error reading firmware: {ERROR}", "ERROR", e);
        readDataBytes.clear();
    }
    return readDataBytes;
}

} // namespace internal

bool update(sdbusplus::bus_t& bus, const std::string& psuInventoryPath,
            const std::string& imageDir)
{
    auto devPath = utils::getDevicePath(bus, psuInventoryPath);

    if (devPath.empty())
    {
        return false;
    }

    std::filesystem::path fsPath(imageDir);

    std::unique_ptr<updater::Updater> updaterPtr = internal::getClassInstance(
        fsPath.filename().string(), psuInventoryPath, devPath, imageDir);

    if (!updaterPtr->isReadyToUpdate())
    {
        lg2::error("PSU not ready to update PSU = {PATH}", "PATH",
                   psuInventoryPath);
        return false;
    }

    updaterPtr->bindUnbind(false);
    updaterPtr->createI2CDevice();
    int ret = updaterPtr->doUpdate();
    updaterPtr->bindUnbind(true);
    return ret == 0;
}

Updater::Updater(const std::string& psuInventoryPath,
                 const std::string& devPath, const std::string& imageDir) :
    bus(sdbusplus::bus::new_default()), psuInventoryPath(psuInventoryPath),
    devPath(devPath), devName(utils::getDeviceName(devPath)), imageDir(imageDir)
{
    fs::path p = fs::path(devPath) / "driver";
    try
    {
        driverPath =
            fs::canonical(p); // Get the path that points to the driver dir
    }
    catch (const fs::filesystem_error& e)
    {
        lg2::error("Failed to get canonical path DEVPATH= {PATH}, ERROR= {ERR}",
                   "PATH", devPath, "ERR", e);
    }
}

// During PSU update, it needs to access the PSU i2c device directly, so it
// needs to unbind the driver during the update, and re-bind after it's done.
// After unbind, the hwmon sysfs will be gone, and the psu-monitor will report
// errors. So set the PSU inventory's Present property to false so that
// psu-monitor will not report any errors.
void Updater::bindUnbind(bool doBind)
{
    if (!doBind)
    {
        // Set non-present before unbind the driver
        setPresent(doBind);
    }
    auto p = driverPath;
    p /= doBind ? "bind" : "unbind";
    std::ofstream out(p.string());
    out << devName;
    if (doBind)
    {
        internal::delay(500);
    }
    out.close();

    if (doBind)
    {
        // Set to present after bind the driver
        setPresent(doBind);
    }
}

void Updater::setPresent(bool present)
{
    try
    {
        auto service = util::getService(psuInventoryPath, INVENTORY_IFACE, bus);
        util::setProperty(INVENTORY_IFACE, PRESENT_PROP, psuInventoryPath,
                          service, bus, present);
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to set present property PSU= {PATH}, PRESENT= {PRESENT}",
            "PATH", psuInventoryPath, "PRESENT", present);
    }
}

bool Updater::isReadyToUpdate()
{
    using namespace phosphor::pmbus;

    // Pre-condition for updating PSU:
    // * Host is powered off
    // * At least one other PSU is present
    // * All other PSUs that are present are having AC input and DC standby
    //   output

    if (util::isPoweredOn(bus, true))
    {
        lg2::warning("Unable to update PSU when host is on");
        return false;
    }

    bool hasOtherPresent = false;
    auto paths = util::getPSUInventoryPaths(bus);
    for (const auto& p : paths)
    {
        if (p == psuInventoryPath)
        {
            // Skip check for itself
            continue;
        }

        // Check PSU present
        bool present = false;
        try
        {
            auto service = util::getService(p, INVENTORY_IFACE, bus);
            util::getProperty(INVENTORY_IFACE, PRESENT_PROP, psuInventoryPath,
                              service, bus, present);
        }
        catch (const std::exception& e)
        {
            lg2::error("Failed to get present property PSU={PSU}", "PSU", p);
        }
        if (!present)
        {
            lg2::warning("PSU not present PSU={PSU}", "PSU", p);
            continue;
        }
        hasOtherPresent = true;

        // Typically the driver is still bound here, so it is possible to
        // directly read the debugfs to get the status.
        try
        {
            auto path = utils::getDevicePath(bus, p);
            PMBus pmbus(path);
            uint16_t statusWord = pmbus.read(STATUS_WORD, Type::Debug);
            auto status0Vout = pmbus.insertPageNum(STATUS_VOUT, 0);
            uint8_t voutStatus = pmbus.read(status0Vout, Type::Debug);
            if ((statusWord & status_word::VOUT_FAULT) ||
                (statusWord & status_word::INPUT_FAULT_WARN) ||
                (statusWord & status_word::VIN_UV_FAULT) ||
                // For ibm-cffps PSUs, the MFR (0x80)'s OV (bit 2) and VAUX
                // (bit 6) fault map to OV_FAULT, and UV (bit 3) fault maps to
                // UV_FAULT in vout status.
                (voutStatus & status_vout::UV_FAULT) ||
                (voutStatus & status_vout::OV_FAULT))
            {
                lg2::warning(
                    "Unable to update PSU when other PSU has input/ouput fault PSU={PSU}, STATUS_WORD={STATUS}, VOUT_BYTE={VOUT}",
                    "PSU", p, "STATUS", lg2::hex, statusWord, "VOUT", lg2::hex,
                    voutStatus);
                return false;
            }
        }
        catch (const std::exception& ex)
        {
            // If error occurs on accessing the debugfs, it means something went
            // wrong, e.g. PSU is not present, and it's not ready to update.
            lg2::error("{EX}", "EX", ex.what());
            return false;
        }
    }
    return hasOtherPresent;
}

int Updater::doUpdate()
{
    using namespace std::chrono;

    uint8_t data;
    uint8_t unlockData[12] = {0x45, 0x43, 0x44, 0x31, 0x36, 0x30,
                              0x33, 0x30, 0x30, 0x30, 0x34, 0x01};
    uint8_t bootFlag = 0x01;
    static_assert(sizeof(unlockData) == 12);

    i2c->write(0xf0, sizeof(unlockData), unlockData);
    printf("Unlock PSU\n");

    std::this_thread::sleep_for(milliseconds(5));

    i2c->write(0xf1, bootFlag);
    printf("Set boot flag ret\n");

    std::this_thread::sleep_for(seconds(3));

    i2c->read(0xf1, data);
    printf("Read of 0x%02x, 0x%02x\n", 0xf1, data);
    return 0;
}

void Updater::createI2CDevice()
{
    auto [id, addr] = utils::parseDeviceName(devName);
    i2c = i2c::create(id, addr);
}

void Updater::createServiceablePel(
    const std::string& errorName, const std::string& severity,
    const std::map<std::string, std::string>& additionalData)
{
    using namespace sdbusplus::xyz::openbmc_project;
    constexpr auto loggingObjectPath = "/xyz/openbmc_project/logging"; // Object
                                                                       // path
    constexpr auto loggingCreateInterface =
        "xyz.openbmc_project.Logging.Create"; // Interface
    try
    {
        auto service = phosphor::power::util::getService(
            loggingObjectPath, loggingCreateInterface, bus);
        if (service.empty())
        {
            lg2::error("Unable to get logging manager service");
            return;
        }

        auto method = bus.new_method_call(service.c_str(), loggingObjectPath,
                                          loggingCreateInterface, "Create");

        method.append(errorName, severity, additionalData);

        bus.call(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::error("Failed creating event log for fault {FILE}, error {ERR}",
                   "FILE", errorName, "ERR", e);
    }
}

std::map<std::string, std::string> Updater::getI2CAdditionalData()
{
    std::map<std::string, std::string> additionalData;
    auto [id, addr] = utils::parseDeviceName(getDevName());
    std::string hexIdString = std::format("0x{:x}", id);
    std::string hexAddrString = std::format("0x{:x}", addr);

    additionalData["CALLOUT_IIC_BUS"] = hexIdString;
    additionalData["CALLOUT_IIC_ADDR"] = hexAddrString;
    return additionalData;
}

void Updater::reportI2CPel(
    std::map<std::string, std::string> extraAdditionalData,
    const std::string& exceptionString, const std::string& errnoString)
{
    std::map<std::string, std::string> additionalData = {
        {"CALLOUT_INVENTORY_PATH", getPsuInventoryPath()}};
    additionalData.merge(extraAdditionalData);
    additionalData.merge(getI2CAdditionalData());
    additionalData["CALLOUT_ERRNO"] = errnoString;
    if (!exceptionString.empty())
    {
        additionalData["Exception:"] = exceptionString;
    }
    createServiceablePel(FW_UPDATE_FAILED_MSG, ERROR_SEVERITY, additionalData);
}

void Updater::reportPSUPel(
    std::map<std::string, std::string> extraAdditionalData)
{
    std::map<std::string, std::string> additionalData = {
        {"CALLOUT_INVENTORY_PATH", getPsuInventoryPath()}};
    additionalData.merge(extraAdditionalData);
    createServiceablePel(updater::FW_UPDATE_FAILED_MSG, updater::ERROR_SEVERITY,
                         additionalData);
}

void Updater::reportSWPel(std::map<std::string, std::string> additionalData)
{
    createServiceablePel(updater::PSU_FW_FILE_ISSUE_MSG,
                         updater::ERROR_SEVERITY, additionalData);
}

void Updater::reportGoodPel()
{
    std::map<std::string, std::string> additionalData = {
        {"Successful PSU Update:", getPsuInventoryPath()},
        {"Firmware Version:", version::getVersion(bus, getPsuInventoryPath())}};
    createServiceablePel(updater::FW_UPDATE_SUCCESS_MSG,
                         updater::INFORMATIONAL_SEVERITY, additionalData);
}
} // namespace updater
