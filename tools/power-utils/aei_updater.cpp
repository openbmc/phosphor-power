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
    if (i2cInterface == nullptr)
    {
        throw std::runtime_error("I2C interface error");
    }
    if (!getFirmwarePath() || !isFirmwareFileValid())
    {
        return true;               // No firmware file abort download
    }
    bool downloadFwFailed = false; // Download Firmware status
    int retryProcessTwo(0);
    int retryProcessOne(0);
    int serviceableEvent(0);
    while ((retryProcessTwo < MAX_RETRIES) && (retryProcessOne < MAX_RETRIES))
    {
        retryProcessTwo++;
        // Write AEI PSU ISP key
        if (!writeIspKey())
        {
            serviceableEvent++;
            if (serviceableEvent == (MAX_RETRIES - 1))
            {
                createServiceableError(
                    "createServiceableError: ISP key failed");
            }
            lg2::error("Failed to set ISP Key");
            downloadFwFailed = true; // Download Firmware status
            continue;
        }

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
                    if (retryProcessOne == (MAX_RETRIES - 1))
                    {
                        // no more retries report serviceable error
                        createServiceableError(
                            "serviceableError: Download firmware failed");
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
    return 0; // Update successful
}

bool AeiUpdater::writeIspKey()
{
    // ISP Key to unlock programming mode ( ASCII for "artY").
    constexpr std::array<uint8_t, 4> unlockData = {0x61, 0x72, 0x74,
                                                   0x59}; // ISP Key "artY"
    try
    {
        // Send ISP Key to unlock device for firmware update
        i2cInterface->write(KEY_REGISTER, unlockData.size(), unlockData.data());
        return true;
    }
    catch (const std::exception& e)
    {
        // Log failure if I2C write fails.
        lg2::error("I2C write failed: {ERROR}", "ERROR", e);
        return false;
    }
}

bool AeiUpdater::writeIspMode()
{
    // Attempt to set device in ISP mode with retries.
    uint8_t ispStatus = 0x0;
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
                return true;
            }
        }
        catch (const std::exception& e)
        {
            // Log I2C error with each retry attempt.
            lg2::error("I2C error during ISP mode write/read: {ERROR}", "ERROR",
                       e);
        }
    }
    createServiceableError("createServiceableError: ISP Status failed");
    lg2::error("Failed to set ISP Mode");
    return false; // Failed to set ISP Mode after retries
}

bool AeiUpdater::writeIspStatusReset()
{
    // Reset ISP status register before firmware download.
    uint8_t ispStatus = 0;
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
            // Log any errors encountered during reset sequence.
            lg2::error("I2C Write ISP reset failed: {ERROR}", "ERROR", e);
        }
    }

    for (int retry = 0; retry < MAX_RETRIES; ++retry)
    {
        try
        {
            i2cInterface->read(STATUS_REGISTER, ispStatus);
            if (ispStatus == B_ISP_MODE)
            {
                lg2::info("Read/Write ISP reset");
                return true; // ISP status reset successfully.
            }
            i2cInterface->write(STATUS_REGISTER,
                                CMD_CLEAR_STATUS); // Clear status if
                                                   // not reset.
            lg2::error("Read/Write ISP reset failed");
        }
        catch (const std::exception& e)
        {
            // Log any errors encountered during reset sequence.
            lg2::error("I2C Read/Write error during ISP reset: {ERROR}",
                       "ERROR", e);
        }
    }
    createServiceableError("createServiceableError: Failed to reset ISP");
    lg2::error("Failed to reset ISP Status");
    ispReboot();
    return false;
}

bool AeiUpdater::getFirmwarePath()
{
    fspath = updater::internal::getFWFilenamePath(getImageDir());
    if (fspath.empty())
    {
        lg2::error("Firmware file path not found");
        return false;
    }
    return true;
}

bool AeiUpdater::isFirmwareFileValid()
{
    if (!updater::internal::validateFWFile(fspath))
    {
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
    for (int i = 0; i < retries; ++i)
    {
        uint8_t readReplySize = 0;
        try
        {
            performI2cWriteRead(regAddr, readReplySize, readData, delayTime);
            if ((readData[STATUS_CML_INDEX] == 0 ||
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
                uint32_t littleEndianValue =
                    *reinterpret_cast<int*>(readData + 4);
                uint32_t bigEndianValue =
                    ((littleEndianValue & 0x000000FF) << 24) |
                    ((littleEndianValue & 0x0000FF00) << 8) |
                    ((littleEndianValue & 0x00FF0000) >> 8) |
                    ((littleEndianValue & 0xFF000000) >> 24);
                lg2::error("Write/read block {NUM} failed", "NUM",
                           bigEndianValue);
            }
        }
        catch (const std::exception& e)
        {
            lg2::error("I2C write/read block failed: {ERROR}", "ERROR", e);
        }
    }
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
        lg2::error("I2C read error during reboot attempt: {ERROR}", "ERROR", e);
    }

    // If we reach here, all retries have failed
    lg2::error("Failed to reboot ISP status after max retries.");
    return false;
}

} // namespace aeiUpdater
