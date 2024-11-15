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

#include "aeiUpdater.hpp"

#include "pmbus.hpp"
#include "types.hpp"
#include "updater.hpp"
#include "utility.hpp"

#include <sys/stat.h>

#include <phosphor-logging/log.hpp>

#include <chrono>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

constexpr uint8_t MAX_RETRIES = 0x02;    // Constants for retry limits

constexpr int ISP_STATUS_DELAY = 1200;   // Delay for ISP status check (1.2s)
constexpr int MEM_WRITE_DELAY = 5000;    // Memory write delay (5s)
constexpr int MEM_STRETCH_DELAY = 60;    // Delay between writes (60ms)
constexpr int MEM_COMPLETE_DELAY = 2000; // Delay before completion (2s)
constexpr int REBOOT_DELAY = 8000;       // Delay for reboot (8s)

constexpr uint8_t I2C_SMBUS_BLOCK_MAX = 0x20; // Max Read bytes from PSU
constexpr uint8_t FW_READ_BLOCK_SIZE = 0x20;  // Read bytes from FW file
constexpr uint8_t BLOCK_WRITE_SIZE = 0x25;    // I2C block write size
constexpr uint8_t READ_SEQ_ST_CML_SIZE = 0x6; // Read sequence and status CML
                                              // size
constexpr uint8_t START_SEQUENCE_INDEX = 0x1; // Starting sequence index
constexpr uint8_t STATUS_CML_INDEX = 0x5;     // Status CML read index

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
constexpr uint8_t B_CHKSUM_ERR = 0x0; // The checksum verification unsuccessful
constexpr uint8_t B_CHKSUM_SUCCESS = 0x1; // The checksum verification
                                          // successful.
constexpr uint8_t B_MEM_ERR = 0x2;        // Memory boundry error indication.
constexpr uint8_t B_ALIGN_ERR = 0x4;      // Adress error indication.
constexpr uint8_t B_KEY_ERR = 0x8;        // Invalid Key
constexpr uint8_t B_START_ERR = 0x10;     // Error indicator set at startup.
constexpr uint8_t B_IMG_MISSMATCH_ERR = 0x20; // Firmware image does not match
                                              // PSU
constexpr uint8_t B_ISP_MODE = 0x40;          // ISP mode
constexpr uint8_t B_ISP_MODE_CHKSUM_GOOD = 0x41; // ISP mode  & good checksum.
constexpr uint8_t B_PRGM_BUSY = 0x80;            // Write operation in progress.

using namespace phosphor::logging;
namespace util = phosphor::power::util;

namespace aeiUpdater
{

// Main function to perform the PSU firmware update process.
int AeiUpdater::doUpdate()
{
    i2cInterface = Updater::getI2C();
    bool cleanFailedIspMode = false; //

    // Set ISP mode by writing necessary keys and resetting ISP status
    if (!writeIspKey() || !writeIspMode() || !writeIspStatusReset())
    {
        log<level::ERR>("Failed to set ISP key or mode or reset ISP status");
        cleanFailedIspMode = true;
    }

    if (cleanFailedIspMode)
    {
        return 1;
    }
    return 0; // Update successful.
}

// Write ISP key to the PSU to allow firmware update.
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
        log<level::ERR>(std::format("I2C write failed: {}", e.what()).c_str());
        return false;
    }
}

// Set the PSU into ISP mode for firmware update.
bool AeiUpdater::writeIspMode()
{
    // Attempt to set device in ISP mode with retries.
    uint8_t data = 0x0;
    for (int retry = 0; retry < MAX_RETRIES; ++retry)
    {
        try
        {
            // Write command to enter ISP mode.
            i2cInterface->write(STATUS_REGISTER, CMD_BOOT_ISP);
            // Delay to allow status register update.
            updater::internal::delay(ISP_STATUS_DELAY);
            // Read back status register to confirm ISP mode is active.
            i2cInterface->read(STATUS_REGISTER, data);

            if (data & B_ISP_MODE)
            {
                return true;
            }
        }
        catch (const std::exception& e)
        {
            // Log I2C error with each retry attempt.
            log<level::ERR>(
                std::format("I2C error during ISP mode write/read: {}",
                            e.what())
                    .c_str());
        }
    }
    log<level::ERR>("Failed to set ISP Mode");
    return false; // Failed to set ISP Mode after retries
}

// Reset ISP status to clear any previous error states
bool AeiUpdater::writeIspStatusReset()
{
    // Reset ISP status register before firmware download.
    uint8_t data = 0;
    try
    {
        i2cInterface->write(STATUS_REGISTER,
                            CMD_RESET_SEQ); // Start reset sequence.
        for (int retry = 0; retry < MAX_RETRIES; ++retry)
        {
            i2cInterface->read(STATUS_REGISTER, data);
            if (data == B_ISP_MODE)
            {
                return true; // ISP status reset successfully.
            }
            i2cInterface->write(STATUS_REGISTER,
                                CMD_CLEAR_STATUS); // Clear status if
                                                   // not reset.
        }
    }
    catch (const std::exception& e)
    {
        // Log any errors encountered during reset sequence.
        log<level::ERR>(
            std::format("I2C Read/Write error during ISP reset: {}", e.what())
                .c_str());
    }
    log<level::ERR>("Failed to reset ISP Status");
    return false;
}

} // namespace aeiUpdater
