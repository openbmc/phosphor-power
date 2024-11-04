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
#pragma once

#include "i2c_interface.hpp"
#include "version.hpp"

#include <sdbusplus/bus.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

class TestUpdater;

namespace updater
{

namespace fs = std::filesystem;

/**
 * Update PSU firmware
 *
 * @param[in] bus - The sdbusplus DBus bus connection
 * @param[in] psuInventoryPath - The inventory path of the PSU
 * @param[in] imageDir - The directory containing the PSU image
 *
 * @return true if successful, otherwise false
 */
bool update(sdbusplus::bus_t& bus, const std::string& psuInventoryPath,
            const std::string& imageDir);

class Updater
{
  public:
    friend TestUpdater;
    Updater() = delete;
    Updater(const Updater&) = delete;
    Updater& operator=(const Updater&) = delete;
    Updater(Updater&&) = default;
    Updater& operator=(Updater&&) = default;

    /**
     * @brief Constructor
     *
     * @param psuInventoryPath - The PSU inventory path
     * @param devPath - The PSU device path
     * @param imageDir - The update image directory
     */
    Updater(const std::string& psuInventoryPath, const std::string& devPath,
            const std::string& imageDir);

    /** @brief Destructor */
    virtual ~Updater() = default;

    /** @brief Bind or unbind the driver
     *
     * @param doBind - indicate if it's going to bind or unbind the driver
     */
    void bindUnbind(bool doBind);

    /** @brief Set the PSU inventory present property
     *
     * @param present - The present state to set
     */
    void setPresent(bool present);

    /** @brief Check if it's ready to update the PSU
     *
     * @return true if it's ready, otherwise false
     */
    bool isReadyToUpdate();

    /** @brief Do the PSU update
     *
     * @return 0 if success, otherwise non-zero
     */
    virtual int doUpdate();

    /** @brief Create I2C device
     *
     * Creates the I2C device based on the device name.
     * e.g. It opens busId 3, address 0x68 for "3-0068"
     */
    void createI2CDevice();

  protected:
    /** @brief Accessor for PSU inventory path */
    const std::string& getPsuInventoryPath() const
    {
        return psuInventoryPath;
    }

    /** @brief Accessor for device path */
    const std::string& getDevPath() const
    {
        return devPath;
    }

    /** @brief Accessor for device name */
    const std::string& getDevName() const
    {
        return devName;
    }

    /** @brief Accessor for image directory */
    const std::string& getImageDir() const
    {
        return imageDir;
    }

    /** @brief I2C interface accessor */
    i2c::I2CInterface* getI2C()
    {
        return i2c.get();
    }

  private:
    /** @brief The sdbusplus DBus bus connection */
    sdbusplus::bus_t bus;

    /** @brief The PSU inventory path */
    std::string psuInventoryPath;

    /** @brief The PSU device path
     *
     * Usually it is a device in i2c subsystem, e.g.
     *   /sys/bus/i2c/devices/3-0068
     */
    std::string devPath;

    /** @brief The PSU device name
     *
     * Usually it is a i2c device name, e.g.
     *   3-0068
     */
    std::string devName;

    /** @brief The PSU image directory */
    std::string imageDir;

    /** @brief The PSU device driver's path
     *
     * Usually it is the PSU driver, e.g.
     *   /sys/bus/i2c/drivers/ibm-cffps
     */
    fs::path driverPath;

    /** @brief The i2c device interface */
    std::unique_ptr<i2c::I2CInterface> i2c;
};

namespace internal
{

/**
 * @brief Get the device name from the device path
 *
 * @param[in] devPath - PSU path
 *
 * @return device name e.g. 3-0068
 */
std::string getDeviceName(std::string devPath);

/**
 * @brief Function to get device path using DBus bus and PSU
 * inventory Path
 *
 * @param[in] bus - The sdbusplus DBus bus connection
 * @param[in] psuInventoryPath - PSU inventory path
 *
 * @return device path e.g. /sys/bus/i2c/devices/3-0068
 */
std::string getDevicePath(sdbusplus::bus_t& bus,
                          const std::string& psuInventoryPath);

/**
 * @brief Parse the device name to obtain bus and device address
 *
 * @param[in] devName - Device name
 *
 * @return bus and device address
 */
std::pair<uint8_t, uint8_t> parseDeviceName(const std::string& devName);

/**
 * @brief Factory function to create an Updater instance based on PSU model
 * number
 *
 * @param[in] model - PSU model number
 * @param[in] psuInventoryPath - PSU inventory path
 * @param[in] devPath - Device path
 * @param[in] imageDir - Image directory
 *
 * return pointer class based on the device PSU model.
 */
std::unique_ptr<updater::Updater> getClassInstance(
    const std::string& model, const std::string& psuInventoryPath,
    const std::string& devPath, const std::string& imageDir);

/**
 * @brief Retrieve the firmware filename path in the specified directory
 *
 * @param[in] directory - Path to FS directory
 *
 * @retun filename or null
 */
const std::string getFWFilenamePath(const std::string& directory);

/**
 * @brief Calculate CRC-8 for a data vector
 *
 * @param[in] data - Firmware data block
 *
 * @return CRC8
 */
uint8_t calculateCRC8(const std::vector<uint8_t>& data);

/**
 * @brief Delay execution in milliseconds
 *
 * @param[in] milliseconds - Time in milliseconds
 */
void delay(const int& milliseconds);

/**
 * @brief Convert a big-endian value to little-endian
 *
 * @param[in] bigEndianValue - Uint 32 bit value
 *
 * @return vector of little endians.
 */
std::vector<uint8_t> bigEndianToLittleEndian(const uint32_t bigEndianValue);

/**
 * @brief Validate the existence and size of a firmware file.
 *
 * @param[in] fileName - Firmware file name
 *
 * @return true for success or false
 */
bool validateFWFile(const std::string& fileName);

/**
 * @brief Open a firmware file for reading in binary mode.
 *
 * @param[in] fileName - Firmware file name
 *
 * @return pointer to firmware file stream
 */
std::unique_ptr<std::ifstream> openFirmwareFile(const std::string& fileName);

/**
 * @brief Read firmware bytes from file.
 *
 * @param[in] inputFile - Input file stream
 * @param[in] numberOfBytesToRead - Number of bytes to read from firmware file.
 *
 * @return vector of data read
 */
std::vector<uint8_t> readFirmwareBytes(std::ifstream& inputFile,
                                       const size_t numberOfBytesToRead);

/**
 * @brief Wrapper to check existence of PSU JSON file
 *
 * @return true or false (true if using JSON file)
 */
bool usePsuJsonFile();
} // namespace internal
} // namespace updater
