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
     * @return Status code 0 for success or 1 for failure.
     */
    int doUpdate() override;

  private:
    /**
     * @brief Writes an ISP (In-System Programming) key to initiate the update.
     *
     * @return True if successful, false otherwise.
     */
    bool writeIspKey();

    /**
     * @brief Writes the mode required for ISP to start firmware programming.
     *
     * @return True if successful, false otherwise.
     */
    bool writeIspMode();

    /**
     * @brief Resets the ISP status to prepare for a firmware update.
     *
     * @return True if successful, false otherwise.
     */
    bool writeIspStatusReset();

    /**
     * @brief Retrieves the path to the firmware file.
     *
     * @return The file path of the firmware.
     */
    std::string getFirmwarePath();

    /**
     * @brief Validates the firmware file.
     *
     * @param fspath The file path to validate.
     * @return True if the file is valid, false otherwise.
     */
    bool isFirmwareFileValid(const std::string& fspath);

    /**
     * @brief Opens a firmware file in binary mode.
     *
     * @param fspath The path to the firmware file.
     * @return A file stream to read the firmware
     * file.
     */
    std::unique_ptr<std::ifstream> openFirmwareFile(const std::string& fspath);

    /**
     * @brief Reads a block of firmware data from the file.
     *
     * @param file The input file stream from which to read data.
     * @param bytesToRead The number of bytes to read.
     * @return A vector containing the firmware block.
     */
    std::vector<uint8_t> readFirmwareBlock(std::ifstream& file,
                                           const size_t& bytesToRead);

    /**
     * @brief Prepares an ISP_MEMORY command block by processing the firmware
     * data block.
     *
     * @param dataBlockRead The firmware data block read from the file.
     */
    void prepareCommandBlock(const std::vector<uint8_t>& dataBlockRead);

    /**
     * @brief Performs firmware update for the power supply unit (PSU)
     *
     * This function retrieves the firmware file from appropriate path,
     * validate existence of the file and initiate the update process.
     * The process includes processing the data into blocks, and writes
     * these blocks to the PSU.
     *
     * @return True if firmware download was successful, false otherwise.
     */
    bool downloadPsuFirmware();

    /**
     * @brief Performs an I2C write and read with retry logic.
     *
     * This function attempts to write a command block to PSU register
     * and read next block sequence and CML write status. If the block
     * sequence number the same as written block, then retry to write
     * same block again.
     *
     * @param regAddr The register address to write to.
     * @param expectedReadSize The size of data read from i2c device.
     * @param readData The buffer to store read data.
     * @param retries The number of retry attempts allowed.
     * @param delayTime The delay time between retries.
     * @return True if the operation is successful, false otherwise.
     */
    bool performI2cWriteReadWithRetries(
        uint8_t regAddr, const uint8_t expectedReadSize, uint8_t* readData,
        const int retries, const int delayTime);

    /**
     * @brief Performs a single I2C write and read without retry logic.
     *
     * @param regAddr The register address to write to.
     * @param readReplySize The size of data read from i2c device.
     * @param readData The buffer to store read data.
     * @param delayTime The delay time between write and read operations.
     */
    void performI2cWriteRead(uint8_t regAddr, uint8_t& readReplySize,
                             uint8_t* readData, const int& delayTime);
    /**
     * @brief Verifies the status of the firmware download.
     *
     * @return True if the download status is verified as successful, false
     * otherwise.
     */
    bool verifyDownloadFWStatus();

    /**
     * @brief Initiates a reboot of the ISP to apply new firmware.
     */
    void ispReboot();

    /**
     * @brief Reads the reboot status from the ISP.
     *
     * @return True if the reboot status indicates success, false
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

    /**
     * @brief Command block used for writing data to the device
     */
    std::vector<uint8_t> cmdBlockWrite;
};
} // namespace aeiUpdater
