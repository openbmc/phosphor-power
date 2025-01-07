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

#include <phosphor-logging/log.hpp>

#include <chrono>
#include <fstream>
#include <thread>
#include <vector>

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
        log<level::ERR>(
            std::format("Firmware file not found: {}", fileName).c_str());
        return false;
    }

    // Check the file size
    auto fileSize = std::filesystem::file_size(fileName);
    if (fileSize == 0)
    {
        log<level::ERR>("Firmware file is empty");
        return false;
    }
    return true;
}

// Open a firmware file for reading in binary mode.
std::unique_ptr<std::ifstream> openFirmwareFile(const std::string& fileName)
{
    if (fileName.empty())
    {
        log<level::ERR>("Firmware file path is not provided");
        return nullptr;
    }
    auto inputFile =
        std::make_unique<std::ifstream>(fileName, std::ios::binary);
    if (!inputFile->is_open())
    {
        log<level::ERR>(
            std::format("Failed to open firmware file: {}", fileName).c_str());
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
        log<level::ERR>(
            std::format("Error reading firmware: {}", e.what()).c_str());
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
        log<level::ERR>("PSU not ready to update",
                        entry("PSU=%s", psuInventoryPath.c_str()));
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
        log<level::ERR>("Failed to get canonical path",
                        entry("DEVPATH=%s", devPath.c_str()),
                        entry("ERROR=%s", e.what()));
        throw;
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
        log<level::ERR>("Failed to set present property",
                        entry("PSU=%s", psuInventoryPath.c_str()),
                        entry("PRESENT=%d", present));
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
        log<level::WARNING>("Unable to update PSU when host is on");
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
            log<level::ERR>("Failed to get present property",
                            entry("PSU=%s", p.c_str()));
        }
        if (!present)
        {
            log<level::WARNING>("PSU not present", entry("PSU=%s", p.c_str()));
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
                log<level::WARNING>(
                    "Unable to update PSU when other PSU has input/ouput fault",
                    entry("PSU=%s", p.c_str()),
                    entry("STATUS_WORD=0x%04x", statusWord),
                    entry("VOUT_BYTE=0x%02x", voutStatus));
                return false;
            }
        }
        catch (const std::exception& ex)
        {
            // If error occurs on accessing the debugfs, it means something went
            // wrong, e.g. PSU is not present, and it's not ready to update.
            log<level::ERR>(ex.what());
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
} // namespace updater
