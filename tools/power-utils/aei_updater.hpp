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
#pragma once

#include "updater.hpp"

namespace aeiUpdater
{
/**
 * @class AeiUpdater
 * @brief A class to handle firmware updates for the AEI PSUs.
 *
 * This class provides methods to update firmware by writing ISP keys,
 * validating firmware files, and performing I2C communications. It includes
 * functions to manage the update process, including downloading firmware blocks
 * and verifying the update status.
 */
class AeiUpdater : public updater::Updater
{
  public:
    AeiUpdater() = delete;
    AeiUpdater(const AeiUpdater&) = delete;
    AeiUpdater(AeiUpdater&&) = delete;
    AeiUpdater& operator=(const AeiUpdater&) = delete;
    AeiUpdater& operator=(AeiUpdater&&) = delete;
    ~AeiUpdater() = default;

    /**
     * @brief Constructor for the AeiUpdater class.
     *
     * @param psuInventoryPath The path to the PSU's inventory in the system.
     * @param devPath The device path for the PSU.
     * @param imageDir The directory containing the firmware image files.
     */
    AeiUpdater(const std::string& psuInventoryPath, const std::string& devPath,
               const std::string& imageDir) :
        Updater(psuInventoryPath, devPath, imageDir)
    {}

    /**
     * @brief Initiates the firmware update process.
     *
     * @return int Status code 0 for success or 1 for failure.
     */
    int doUpdate() override;

  private:
    /**
     * @brief Writes an ISP (In-System Programming) key to initiate the update.
     *
     * @return bool True if successful, false otherwise.
     */
    bool writeIspKey();

    /**
     * @brief Writes the mode required for ISP to start firmware programming.
     *
     * @return bool True if successful, false otherwise.
     */
    bool writeIspMode();

    /**
     * @brief Resets the ISP status to prepare for a firmware update.
     *
     * @return bool True if successful, false otherwise.
     */
    bool writeIspStatusReset();

    /**
     * @brief Retrieves the path to the firmware file.
     *
     * @return std::string The file path of the firmware.
     */
    std::string getFirmwarePath();

    /**
     * @brief Validates the firmware file.
     *
     * @param fspath The file path to validate.
     * @return bool True if the file is valid, false otherwise.
     */
    bool isFirmwareFileValid(const std::string& fspath);

    /**
     * @brief Opens a firmware file in binary mode.
     *
     * @param fspath The path to the firmware file.
     * @return std::unique_ptr<std::ifstream> A file stream to read the firmware
     * file.
     */
    std::unique_ptr<std::ifstream> openFirmwareFile(const std::string& fspath);

    /**
     * @brief Reads a block of firmware data from the file.
     *
     * @param file The input file stream from which to read data.
     * @param bytesToRead The number of bytes to read.
     * @return std::vector<uint8_t> A vector containing the firmware block.
     */
    std::vector<uint8_t> readFirmwareBlock(std::ifstream& file,
                                           const size_t& bytesToRead);

    /**
     * @brief Prepares a command block by processing the firmware data block.
     *
     * @param dataBlockRead The firmware data block read from the file.
     * @return std::vector<uint8_t> The prepared command block.
     */
    std::vector<uint8_t>
        prepareCommandBlock(const std::vector<uint8_t>& dataBlockRead);

    /**
     * @brief Initiates a reboot of the ISP to apply new firmware.
     */
    void ispReboot();

    /**
     * @brief Reads the reboot status from the ISP.
     *
     * @return bool True if the reboot status indicates success, false
     * otherwise.
     */
    bool ispReadRebootStatus();

    /**
     * @brief Pointer to the I2C interface for communication
     *
     * This pointer is not owned by the class. The caller is responsible for
     * ensuring that 'i2cInterface'  remains valid for the lifetime of this
     * object. Ensure to check this pointer for null before use.
     */
    i2c::I2CInterface* i2cInterface;

    /**
     * @brief Stores byte-swapped indices for command processing
     */
    std::vector<uint8_t> byteSwappedIndex;
};
} // namespace aeiUpdater
