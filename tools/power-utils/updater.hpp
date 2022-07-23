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

#include <sdbusplus/bus.hpp>

#include <filesystem>
#include <string>

class TestUpdater;

namespace updater
{

namespace fs = std::filesystem;

/**
 * Update PSU firmware
 *
 * @param[in] psuInventoryPath - The inventory path of the PSU
 * @param[in] imageDir - The directory containing the PSU image
 *
 * @return true if successful, otherwise false
 */
bool update(const std::string& psuInventoryPath, const std::string& imageDir);

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
    ~Updater() = default;

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
    int doUpdate();

    /** @brief Create I2C device
     *
     * Creates the I2C device based on the device name.
     * e.g. It opens busId 3, address 0x68 for "3-0068"
     */
    void createI2CDevice();

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

} // namespace updater
