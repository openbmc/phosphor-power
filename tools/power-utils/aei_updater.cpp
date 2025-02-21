/**
 * Copyright © 2024 IBM Corporation
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

#include "config.h"

#include "aei_updater.hpp"

#include "pmbus.hpp"
#include "types.hpp"
#include "updater.hpp"
#include "utility.hpp"
#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fstream>

namespace aeiUpdater
{

constexpr uint8_t MAX_RETRIES = 0x02;    // Constants for retry limits

constexpr int ISP_STATUS_DELAY = 1200;   // Delay for ISP status check (1.2s)
constexpr int MEM_WRITE_DELAY = 5000;    // Memory write delay (5s)
constexpr int MEM_STRETCH_DELAY = 1;     // Delay between writes (1ms)
constexpr int MEM_COMPLETE_DELAY = 2000; // Delay before completion (2s)
constexpr int REBOOT_DELAY = 8000;       // Delay for reboot (8s)

constexpr uint8_t I2C_SMBUS_BLOCK_MAX = 0x20;    // Max Read bytes from PSU
constexpr uint8_t FW_READ_BLOCK_SIZE = 0x20;     // Read bytes from FW file
constexpr uint8_t BLOCK_WRITE_SIZE = 0x25;       // I2C block write size

constexpr uint8_t START_SEQUENCE_INDEX = 0x1;    // Starting sequence index
constexpr uint8_t STATUS_CML_INDEX = 0x4;        // Status CML read index
constexpr uint8_t EXPECTED_MEM_READ_REPLY = 0x5; //  Expected memory read reply
                                                 //  size after write data

// Register addresses for commands.
constexpr uint8_t KEY_REGISTER = 0xF6;        // Key register
constexpr uint8_t STATUS_REGISTER = 0xF7;     // Status register
constexpr uint8_t ISP_MEMORY_REGISTER = 0xF9; // ISP memory register

// Define AEI ISP status register commands
constexpr uint8_t CMD_CLEAR_STATUS = 0x0; // Clear the status register
constexpr uint8_t CMD_RESET_SEQ = 0x01;   // This command will reset ISP OS for
                                          // another attempt of a sequential
                                          // programming operation.
constexpr uint8_t CMD_BOOT_ISP = 0x02; // Boot the In-System Programming System.
constexpr uint8_t CMD_BOOT_PWR = 0x03; // Attempt to boot the Power Management
                                       // OS.

// Define AEI ISP response status bit
constexpr uint8_t B_ISP_MODE = 0x40;             // ISP mode
constexpr uint8_t B_ISP_MODE_CHKSUM_GOOD = 0x41; // ISP mode  & good checksum.
constexpr uint8_t SUCCESSFUL_ISP_REBOOT_STATUS = 0x0; // Successful ISP reboot
                                                      // status
using namespace phosphor::logging;
namespace util = phosphor::power::util;

int AeiUpdater::doUpdate()
{
    i2cInterface = Updater::getI2C();
    enablePelLogging();
    if (i2cInterface == nullptr)
    {
        // Report serviceable error
        std::map<std::string, std::string> additionalData = {
            {"I2C_INTERFACE", "I2C interface is null pointer."}};
        // Callout PSU & I2C
        reportI2CPel(additionalData, "", std::to_string(errno));

        throw std::runtime_error("I2C interface error");
    }
    if (!getFirmwarePath() || !isFirmwareFileValid())
    {
        return 1;                  // No firmware file abort download
    }
    bool downloadFwFailed = false; // Download Firmware status
    int retryProcessTwo(0);
    int retryProcessOne(0);
    disablePelLogging();
    while ((retryProcessTwo < MAX_RETRIES) && (retryProcessOne < MAX_RETRIES))
    {
        // Write AEI PSU ISP key
        if (!writeIspKey())
        {
            lg2::error("Failed to set ISP Key");
            downloadFwFailed = true; // Download Firmware status
            break;
        }

        if (retryProcessTwo == (MAX_RETRIES - 1))
        {
            enablePelLogging();
        }
        retryProcessTwo++;
        while (retryProcessOne < MAX_RETRIES)
        {
            downloadFwFailed = false; // Download Firmware status
            retryProcessOne++;
            // Set ISP mode
            if (!writeIspMode())
            {
                // Write ISP Mode failed MAX_RETRIES times
                retryProcessTwo = MAX_RETRIES;
                downloadFwFailed = true; // Download Firmware Failed
                break;
            }

            // Reset ISP status
            if (writeIspStatusReset())
            {
                // Start PSU frimware download.
                if (downloadPsuFirmware())
                {
                    if (!verifyDownloadFWStatus())
                    {
                        downloadFwFailed = true;
                        continue;
                    }
                }
                else
                {
                  // One of the block write commands failed, retry download
                  // procedure one time starting with re-writing initial ISP
                  // mode. If it fails again, log serviceable error.

                  if (retryProcessOne == MAX_RETRIES)
                  {
                      // Callout PSU failed to update FW
                      std::map<std::string, std::string> additionalData = {
                          {"UPDATE_FAILED", "Download firmware failed"}};

                      reportPSUPel(additionalData);
                      ispReboot(); // Try to set PSU to normal mode
                  }
                    downloadFwFailed = true;
                    continue;
                }
            }
            else
            {
                // ISP Status Reset failed MAX_RETRIES times
                retryProcessTwo = MAX_RETRIES;
                downloadFwFailed = true;
                break;
            }

            ispReboot();

            if (ispReadRebootStatus() && !downloadFwFailed)
            {
                // Download completed successful
                retryProcessTwo = MAX_RETRIES;
                break;
            }
            else
            {
                // Retry the whole download process starting with the key and
                // if fails again then report PEL
                if ((retryProcessOne < (MAX_RETRIES - 1)) &&
                    (retryProcessTwo < (MAX_RETRIES - 1)))
                {
                    downloadFwFailed = false;
                    break;
                }
            }
        }
    }
    if (downloadFwFailed)
    {
        return 1;
    }
    enablePelLogging();
    bindUnbind(true);
    updater::internal::delay(100);
    reportGoodPel();
    return 0; // Update successful
}

bool AeiUpdater::writeIspKey()
{
    // ISP Key to unlock programming mode ( ASCII for "artY").
    constexpr std::array<uint8_t, 4> unlockData = {0x61, 0x72, 0x74,
                                                   0x59}; // ISP Key "artY"
    for (int retry = 0; retry < MAX_RETRIES; ++retry)
    {
        try
        {
            // Send ISP Key to unlock device for firmware update
            i2cInterface->write(KEY_REGISTER, unlockData.size(),
                                unlockData.data());
            disablePelLogging();
            return true;
        }
        catch (const std::exception& e)
        {
            // Log failure if I2C write fails.
            lg2::error("I2C write failed: {ERROR}", "ERROR", e);
            std::map<std::string, std::string> additionalData = {
                {"ISP_KEY", "ISP key failed due to I2C exception"}};
            reportI2CPel(additionalData, e.what(), std::to_string(errno));
            enablePelLogging(); // enable PEL Logging if fail again call out PSU
                                // & I2C
        }
    }
    return false;
}

bool AeiUpdater::writeIspMode()
{
    // Attempt to set device in ISP mode with retries.
    uint8_t ispStatus = 0x0;
    uint8_t i2cFailCount = 0;
    for (int retry = 0; retry < MAX_RETRIES; ++retry)
    {
        try
        {
            // Write command to enter ISP mode.
            i2cInterface->write(STATUS_REGISTER, CMD_BOOT_ISP);
            // Delay to allow status register update.
            updater::internal::delay(ISP_STATUS_DELAY);
            // Read back status register to confirm ISP mode is active.
            i2cInterface->read(STATUS_REGISTER, ispStatus);

            if (ispStatus & B_ISP_MODE)
            {
                lg2::info("Set ISP Mode");
                disablePelLogging();
                return true;
            }
            enablePelLogging();
        }
        catch (const std::exception& e)
        {
            i2cFailCount++;
            // Log I2C error with each retry attempt.
            lg2::error("I2C error during ISP mode write/read: {ERROR}", "ERROR",
                       e);
            if (i2cFailCount == MAX_RETRIES)
            {
                enablePelLogging();
                std::map<std::string, std::string> additionalData = {
                    {"FIRMWARE_I2C_STATUS",
                     "Download firmware failed during writeIspMode due to I2C exception"}};
                // Callout PSU & I2C
                reportI2CPel(additionalData, e.what(), std::to_string(errno));
                return false; // Failed to set ISP Mode
            }
        }
    }

    if (i2cFailCount != MAX_RETRIES)
    {
        // Callout PSU
        std::map<std::string, std::string> additionalData = {
            {"FIRMWARE_STATUS",
             "Download firmware failed during writeIspMode"}};
        reportPSUPel(additionalData);
    }

    lg2::error("Failed to set ISP Mode");
    return false; // Failed to set ISP Mode after retries
}

bool AeiUpdater::writeIspStatusReset()
{
    // Reset ISP status register before firmware download.
    uint8_t ispStatus = 0;
    uint8_t i2cFailCount = 0;
    for (int retry = 0; retry < MAX_RETRIES; retry++)
    {
        try
        {
            i2cInterface->write(STATUS_REGISTER,
                                CMD_RESET_SEQ); // Start reset sequence.
            retry = MAX_RETRIES;
        }
        catch (const std::exception& e)
        {
            i2cFailCount++;
            // Log any errors encountered during reset sequence.
            lg2::error("I2C Write ISP reset failed: {ERROR}", "ERROR", e);
            if (i2cFailCount == MAX_RETRIES)
            {
                enablePelLogging();
                std::map<std::string, std::string> additionalData = {
                    {"ISP_RESET", "I2C exception during ISP status reset"}};
                // Callout PSU & I2C
                reportI2CPel(additionalData, e.what(), std::to_string(errno));
                ispReboot();
                return false;
            }
        }
    }

    i2cFailCount = 0;
    for (int retry = 0; retry < MAX_RETRIES; ++retry)
    {
        try
        {
            i2cInterface->read(STATUS_REGISTER, ispStatus);
            if (ispStatus == B_ISP_MODE)
            {
                lg2::info("write/read ISP reset");
                disablePelLogging();
                return true; // ISP status reset successfully.
            }
            i2cInterface->write(STATUS_REGISTER,
                                CMD_CLEAR_STATUS); // Clear status if
                                                   // not reset.
            lg2::error("Write ISP reset failed");
            enablePelLogging();
        }
        catch (const std::exception& e)
        {
            i2cFailCount++;
            // Log any errors encountered during reset sequence.
            lg2::error(
                "I2C Write/Read or Write error during ISP reset: {ERROR}",
                "ERROR", e);
            if (i2cFailCount == MAX_RETRIES)
            {
                enablePelLogging();
                std::map<std::string, std::string> additionalData = {
                    {"ISP_I2C_READ_STATUS",
                     "I2C exception during read ISP status"}};
                // Callout PSU & I2C
                reportI2CPel(additionalData, e.what(), std::to_string(errno));
            }
        }
    }
    if (i2cFailCount != MAX_RETRIES)
    {
        std::map<std::string, std::string> additionalData = {
            {"ISP_REST_FAILED", "Failed to read ISP expected status"}};
        // Callout PSU PEL
        reportPSUPel(additionalData);
    }
    lg2::error("Failed to reset ISP Status");
    ispReboot();
    return false;
}

bool AeiUpdater::getFirmwarePath()
{
    fspath = updater::internal::getFWFilenamePath(getImageDir());
    if (fspath.empty())
    {
        std::map<std::string, std::string> additionalData = {
            {"FILE_PATH", "Firmware file path is null"}};
        // Callout BMC0001 procedure
        reportSWPel(additionalData);
        lg2::error("Firmware file path not found");
        return false;
    }
    return true;
}

bool AeiUpdater::isFirmwareFileValid()
{
    if (!updater::internal::validateFWFile(fspath))
    {
        std::map<std::string, std::string> additionalData = {
            {"FIRMWARE_VALID",
             "Firmware validation failed, FW file path = " + fspath}};
        // Callout BMC0001 procedure
        reportSWPel(additionalData);
        lg2::error("Firmware validation failed, fspath={PATH}", "PATH", fspath);
        return false;
    }
    return true;
}

std::unique_ptr<std::ifstream> AeiUpdater::openFirmwareFile()
{
    auto inputFile = updater::internal::openFirmwareFile(fspath);
    if (!inputFile)
    {
        std::map<std::string, std::string> additionalData = {
            {"FIRMWARE_OPEN",
             "Firmware file failed to open, FW file path = " + fspath}};
        // Callout BMC0001 procedure
        reportSWPel(additionalData);
        lg2::error("Failed to open firmware file");
    }
    return inputFile;
}

std::vector<uint8_t> AeiUpdater::readFirmwareBlock(std::ifstream& file,
                                                   const size_t& bytesToRead)
{
    auto block = updater::internal::readFirmwareBytes(file, bytesToRead);
    return block;
}

void AeiUpdater::prepareCommandBlock(const std::vector<uint8_t>& dataBlockRead)
{
    cmdBlockWrite.clear(); // Clear cmdBlockWrite before use
    // Assign new values to cmdBlockWrite
    cmdBlockWrite.push_back(ISP_MEMORY_REGISTER);
    cmdBlockWrite.push_back(BLOCK_WRITE_SIZE);
    cmdBlockWrite.insert(cmdBlockWrite.end(), byteSwappedIndex.begin(),
                         byteSwappedIndex.end());
    cmdBlockWrite.insert(cmdBlockWrite.end(), dataBlockRead.begin(),
                         dataBlockRead.end());

    // Resize to ensure it matches BLOCK_WRITE_SIZE + 1 and append CRC
    if (cmdBlockWrite.size() != BLOCK_WRITE_SIZE + 1)
    {
        cmdBlockWrite.resize(BLOCK_WRITE_SIZE + 1, 0xFF);
    }
    cmdBlockWrite.push_back(updater::internal::calculateCRC8(cmdBlockWrite));
    // Remove the F9 and byte count
    cmdBlockWrite.erase(cmdBlockWrite.begin(), cmdBlockWrite.begin() + 2);
}

bool AeiUpdater::downloadPsuFirmware()
{
    // Open firmware file
    auto inputFile = openFirmwareFile();
    if (!inputFile)
    {
        if (isPelLogEnabled())
        {
            // Callout BMC0001 procedure
            std::map<std::string, std::string> additionalData = {
                {"FW_FAILED_TO_OPEN", "Firmware file failed to open"},
                {"FW_FILE_PATH", fspath}};

            reportSWPel(additionalData);
            ispReboot(); // Try to set PSU to normal mode
        }
        lg2::error("Unable to open firmware file {FILE}", "FILE", fspath);
        return false;
    }

    // Read and process firmware file in blocks
    size_t bytesRead = 0;
    const auto fileSize = std::filesystem::file_size(fspath);
    bool downloadFailed = false;
    byteSwappedIndex =
        updater::internal::bigEndianToLittleEndian(START_SEQUENCE_INDEX);
    int writeBlockDelay = MEM_WRITE_DELAY;

    while ((bytesRead < fileSize) && !downloadFailed)
    {
        // Read a block of firmware data
        auto dataRead = readFirmwareBlock(*inputFile, FW_READ_BLOCK_SIZE);
        bytesRead += dataRead.size();

        // Prepare command block with the current index and data
        prepareCommandBlock(dataRead);

        // Perform I2C write/read with retries
        uint8_t readData[I2C_SMBUS_BLOCK_MAX] = {};
        downloadFailed = !performI2cWriteReadWithRetries(
            ISP_MEMORY_REGISTER, EXPECTED_MEM_READ_REPLY, readData, MAX_RETRIES,
            writeBlockDelay);

        // Adjust delay after first write block
        writeBlockDelay = MEM_STRETCH_DELAY;
    }

    inputFile->close();

    // Log final download status
    if (downloadFailed)
    {
        lg2::error(
            "Firmware download failed after retries at FW block {BYTESREAD}",
            "BYTESREAD", bytesRead);
        return false; // Failed
    }
    return true;
}

bool AeiUpdater::performI2cWriteReadWithRetries(
    uint8_t regAddr, const uint8_t expectedReadSize, uint8_t* readData,
    const int retries, const int delayTime)
{
    uint8_t i2cFailCount = 0;
    uint32_t bigEndianValue = 0;
    for (int i = 0; i < retries; ++i)
    {
        uint8_t readReplySize = 0;
        try
        {
            performI2cWriteRead(regAddr, readReplySize, readData, delayTime);
            if ((readData[STATUS_CML_INDEX] == 0 ||
                 // The first firmware data packet sent to the PSU have a
                 // response of 0x80 which indicates firmware update in
                 // progress. If retry to send the first packet again reply will
                 // be 0.
                 (readData[STATUS_CML_INDEX] == 0x80 &&
                  delayTime == MEM_WRITE_DELAY)) &&
                (readReplySize == expectedReadSize) &&
                !std::equal(readData, readData + 4, byteSwappedIndex.begin()))
            {
                std::copy(readData, readData + 4, byteSwappedIndex.begin());
                return true;
            }
            else
            {
                bigEndianValue = (readData[0] << 24) | (readData[1] << 16) |
                                 (readData[2] << 8) | (readData[3]);
                lg2::error("Write/read block {NUM} failed", "NUM",
                           bigEndianValue);
            }
        }
        catch (const std::exception& e)
        {
            i2cFailCount++;
            if (i2cFailCount == MAX_RETRIES)
            {
                std::map<std::string, std::string> additionalData = {
                    {"I2C_WRITE_READ",
                     "I2C exception while flashing the firmware."}};
                // Callout PSU & I2C
                reportI2CPel(additionalData, e.what(), std::to_string(errno));
            }
            lg2::error("I2C write/read block failed: {ERROR}", "ERROR",
                       e.what());
        }
    }
    std::map<std::string, std::string> additionalData = {
        {"WRITE_READ",
         "Download firmware failed block: " + std::to_string(bigEndianValue)}};
    // Callout PSU
    reportPSUPel(additionalData);
    return false;
}

void AeiUpdater::performI2cWriteRead(uint8_t regAddr, uint8_t& readReplySize,
                                     uint8_t* readData, const int& delayTime)
{
    i2cInterface->processCall(regAddr, cmdBlockWrite.size(),
                              cmdBlockWrite.data(), readReplySize, readData);

    if (delayTime != 0)
    {
        updater::internal::delay(delayTime);
    }
}

bool AeiUpdater::verifyDownloadFWStatus()
{
    try
    {
        // Read and verify firmware download status.
        uint8_t status = 0;
        i2cInterface->read(STATUS_REGISTER, status);
        if (status != B_ISP_MODE_CHKSUM_GOOD)
        {
            lg2::error("Firmware download failed - status: {ERR}", "ERR",
                       status);

            return false; // Failed checksum
        }
        return true;
    }
    catch (const std::exception& e)
    {
        lg2::error("I2C read status register failed: {ERROR}", "ERROR", e);
    }
    return false; // Failed
}

void AeiUpdater::ispReboot()
{
    updater::internal::delay(
        MEM_COMPLETE_DELAY); // Delay before starting the reboot process

    try
    {
        // Write reboot command to the status register
        i2cInterface->write(STATUS_REGISTER, CMD_BOOT_PWR);

        updater::internal::delay(
            REBOOT_DELAY); // Add delay after writing reboot command
    }
    catch (const std::exception& e)
    {
        lg2::error("I2C write error during reboot: {ERROR}", "ERROR", e);
    }
}

bool AeiUpdater::ispReadRebootStatus()
{
    for (int retry = 0; retry < MAX_RETRIES; ++retry)
    {
        try
        {
            // Read from the status register to verify reboot
            uint8_t data = 1; // Initialize data to a non-zero value
            i2cInterface->read(STATUS_REGISTER, data);

            // If the reboot was successful, the read data should be 0
            if (data == SUCCESSFUL_ISP_REBOOT_STATUS)
            {
                lg2::info("ISP Status Reboot successful.");
                return true;
            }
        }
        catch (const std::exception& e)
        {
            if (isPelLogEnabled())
            {
                std::map<std::string, std::string> additionalData = {
                    {"I2C_READ_REBOOT",
                     "I2C exception while reading ISP reboot status"}};

                // Callout PSU & I2C
                reportI2CPel(additionalData, e.what(), std::to_string(errno));
            }
            lg2::error("I2C read error during reboot attempt: {ERROR}", "ERROR",
                       e);
        }
        // Reboot the PSU
        ispReboot(); // Try to set PSU to normal mode
    }

    // If we reach here, all retries have failed
    lg2::error("Failed to reboot ISP status after max retries.");
    return false;
}
} // namespace aeiUpdater
