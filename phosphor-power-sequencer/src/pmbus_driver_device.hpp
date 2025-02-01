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

#include "pmbus.hpp"
#include "rail.hpp"
#include "services.hpp"
#include "standard_device.hpp"

#include <stddef.h> // for size_t

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @class PMBusDriverDevice
 *
 * StandardDevice sub-class for power sequencer devices that are bound to a
 * PMBus device driver.
 */
class PMBusDriverDevice : public StandardDevice
{
  public:
    // Specify which compiler-generated methods we want
    PMBusDriverDevice() = delete;
    PMBusDriverDevice(const PMBusDriverDevice&) = delete;
    PMBusDriverDevice(PMBusDriverDevice&&) = delete;
    PMBusDriverDevice& operator=(const PMBusDriverDevice&) = delete;
    PMBusDriverDevice& operator=(PMBusDriverDevice&&) = delete;
    virtual ~PMBusDriverDevice() = default;

    /**
     * Constructor.
     *
     * @param name Device name
     * @param rails Voltage rails that are enabled and monitored by this device
     * @param services System services like hardware presence and the journal
     * @param bus I2C bus for the device
     * @param address I2C address for the device
     * @param driverName Device driver name
     * @param instance Chip instance number
     */
    explicit PMBusDriverDevice(
        const std::string& name, std::vector<std::unique_ptr<Rail>> rails,
        Services& services, uint8_t bus, uint16_t address,
        const std::string& driverName = "", size_t instance = 0) :
        StandardDevice(name, std::move(rails)), bus{bus}, address{address},
        driverName{driverName}, instance{instance}
    {
        pmbusInterface =
            services.createPMBus(bus, address, driverName, instance);
    }

    /**
     * Returns the I2C bus for the device.
     *
     * @return I2C bus
     */
    uint8_t getBus() const
    {
        return bus;
    }

    /**
     * Returns the I2C address for the device.
     *
     * @return I2C address
     */
    uint16_t getAddress() const
    {
        return address;
    }

    /**
     * Returns the device driver name.
     *
     * @return driver name
     */
    const std::string& getDriverName() const
    {
        return driverName;
    }

    /**
     * Returns the chip instance number.
     *
     * @return chip instance
     */
    size_t getInstance() const
    {
        return instance;
    }

    /**
     * Returns interface to the PMBus information that is provided by the device
     * driver in sysfs.
     *
     * @return PMBus interface object
     */
    pmbus::PMBusBase& getPMBusInterface()
    {
        return *pmbusInterface;
    }

    /** @copydoc PowerSequencerDevice::getGPIOValues() */
    virtual std::vector<int> getGPIOValues(Services& services) override;

    /** @copydoc PowerSequencerDevice::getStatusWord() */
    virtual uint16_t getStatusWord(uint8_t page) override;

    /** @copydoc PowerSequencerDevice::getStatusVout() */
    virtual uint8_t getStatusVout(uint8_t page) override;

    /** @copydoc PowerSequencerDevice::getReadVout() */
    virtual double getReadVout(uint8_t page) override;

    /** @copydoc PowerSequencerDevice::getVoutUVFaultLimit() */
    virtual double getVoutUVFaultLimit(uint8_t page) override;

    /**
     * Returns map from PMBus PAGE numbers to sysfs hwmon file numbers.
     *
     * Throws an exception if an error occurs trying to build the map.
     *
     * @return page to file number map
     */
    const std::map<uint8_t, unsigned int>& getPageToFileNumberMap()
    {
        if (pageToFileNumber.empty())
        {
            buildPageToFileNumberMap();
        }
        return pageToFileNumber;
    }

    /**
     * Returns the hwmon file number that corresponds to the specified PMBus
     * PAGE number.
     *
     * Throws an exception if a file number was not found for the specified PAGE
     * number.
     *
     * @param page PMBus PAGE number
     * @return hwmon file number
     */
    unsigned int getFileNumber(uint8_t page);

  protected:
    /** @copydoc StandardDevice::prepareForPgoodFaultDetection() */
    virtual void prepareForPgoodFaultDetection(Services& services) override
    {
        // Rebuild PMBus PAGE to hwmon file number map
        buildPageToFileNumberMap();

        // Call parent class method to do any actions defined there
        StandardDevice::prepareForPgoodFaultDetection(services);
    }

    /**
     * Build mapping from PMBus PAGE numbers to the hwmon file numbers in
     * sysfs.
     *
     * hwmon file names have the format:
     *   <type><number>_<item>
     *
     * The <number> is not the PMBus PAGE number.  The PMBus PAGE is determined
     * by reading the contents of the <type><number>_label file.
     *
     * If the map is not empty, it is cleared and rebuilt.  This is necessary
     * over time because power devices may have been added or removed.
     *
     * Throws an exception if an error occurs trying to build the map.
     */
    virtual void buildPageToFileNumberMap();

    /**
     * Returns whether the specified sysfs hwmon file is a voltage label file.
     *
     * If it is a label file, the hwmon file number is obtained from the file
     * name and returned.
     *
     * @param fileName file within the sysfs hwmon directory
     * @param fileNumber the hwmon file number is stored in this output
     *                   parameter if this is a label file
     * @return true if specified file is a voltage label file, false otherwise
     */
    virtual bool isLabelFile(const std::string& fileName,
                             unsigned int& fileNumber);

    /**
     * Reads the specified voltage label file to obtain the associated PMBus
     * PAGE number.
     *
     * The returned optional variable will have no value if the PMBus PAGE
     * number could not be obtained due to an error.
     *
     * @param fileName voltage label file within the sysfs hwmon directory
     * @return PMBus page number
     */
    virtual std::optional<uint8_t> readPageFromLabelFile(
        const std::string& fileName);

    /**
     * I2C bus for the device.
     */
    uint8_t bus;

    /**
     * I2C address for the device.
     */
    uint16_t address;

    /**
     * Device driver name.
     */
    std::string driverName;

    /**
     * Chip instance number.
     */
    size_t instance;

    /**
     * Interface to the PMBus information that is provided by the device driver
     * in sysfs.
     */
    std::unique_ptr<pmbus::PMBusBase> pmbusInterface;

    /**
     * Map from PMBus PAGE numbers to sysfs hwmon file numbers.
     */
    std::map<uint8_t, unsigned int> pageToFileNumber;
};

} // namespace phosphor::power::sequencer
