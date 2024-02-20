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

#include "power_sequencer_device.hpp"
#include "services.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace phosphor::power::sequencer
{

/**
 * @struct GPIO
 *
 * General Purpose Input/Output (GPIO) that can be read to obtain the pgood
 * status of a voltage rail.
 */
struct GPIO
{
    /**
     * The libgpiod line offset of the GPIO.
     */
    unsigned int line{0};

    /**
     * Specifies whether the GPIO is active low.
     *
     * If true, the GPIO value 0 indicates a true pgood status. If false, the
     * GPIO value 1 indicates a true pgood status.
     */
    bool activeLow{false};
};

/**
 * @class Rail
 *
 * A voltage rail that is enabled or monitored by the power sequencer device.
 */
class Rail
{
  public:
    // Specify which compiler-generated methods we want
    Rail() = delete;
    Rail(const Rail&) = delete;
    Rail(Rail&&) = delete;
    Rail& operator=(const Rail&) = delete;
    Rail& operator=(Rail&&) = delete;
    ~Rail() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param name Unique name for the rail
     * @param presence Optional D-Bus inventory path of a system component which
     *                 must be present in order for the rail to be present
     * @param page Optional PMBus PAGE number of the rail.  Required if
     *             checkStatusVout or compareVoltageToLimit is true.
     * @param isPowerSupplyRail Specifies whether the rail is produced by a
     *                          power supply
     * @param checkStatusVout Specifies whether to check the value of the PMBus
     *                        STATUS_VOUT command when determining the pgood
     *                        status of the rail
     * @param compareVoltageToLimit Specifies whether to compare the output
     *                              voltage to the undervoltage fault limit when
     *                              determining the pgood status of the rail
     * @param gpio Optional GPIO to read to determine the pgood status of the
     *             rail
     */
    explicit Rail(const std::string& name,
                  const std::optional<std::string>& presence,
                  const std::optional<uint8_t>& page, bool isPowerSupplyRail,
                  bool checkStatusVout, bool compareVoltageToLimit,
                  const std::optional<GPIO>& gpio) :
        name{name},
        presence{presence}, page{page}, isPsuRail{isPowerSupplyRail},
        checkStatusVout{checkStatusVout},
        compareVoltageToLimit{compareVoltageToLimit}, gpio{gpio}
    {
        // If checking STATUS_VOUT or output voltage, verify PAGE was specified
        if ((checkStatusVout || compareVoltageToLimit) && !page)
        {
            throw std::invalid_argument{"PMBus PAGE is required"};
        }
    }

    /**
     * Returns the unique name for the rail.
     *
     * @return rail name
     */
    const std::string& getName() const
    {
        return name;
    }

    /**
     * Returns the D-Bus inventory path of a system component which must be
     * present in order for the rail to be present.
     *
     * @return inventory path for presence detection
     */
    const std::optional<std::string>& getPresence() const
    {
        return presence;
    }

    /**
     * Returns the PMBus PAGE number of the rail.
     *
     * @return PAGE number for rail
     */
    const std::optional<uint8_t>& getPage() const
    {
        return page;
    }

    /**
     * Returns whether the rail is produced by a power supply.
     *
     * @return true if rail is produced by a power supply, false otherwise
     */
    bool isPowerSupplyRail() const
    {
        return isPsuRail;
    }

    /**
     * Returns whether the value of the PMBus STATUS_VOUT command is checked
     * when determining the pgood status of the rail.
     *
     * @return true if STATUS_VOUT is checked, false otherwise
     */
    bool getCheckStatusVout() const
    {
        return checkStatusVout;
    }

    /**
     * Returns whether the output voltage should be compared to the undervoltage
     * fault limit when determining the pgood status of the rail.
     *
     * @return true if output voltage is compared to limit, false otherwise
     */
    bool getCompareVoltageToLimit() const
    {
        return compareVoltageToLimit;
    }

    /**
     * Returns the GPIO to read to determine the pgood status of the rail.
     *
     * @return GPIO
     */
    const std::optional<GPIO>& getGPIO() const
    {
        return gpio;
    }

    /**
     * Returns whether the rail is present.
     *
     * Returns true if no inventory path was specified for presence detection.
     *
     * @param services System services like hardware presence and the journal
     * @return true if rail is present, false otherwise
     */
    bool isPresent(Services& services);

    /**
     * Returns the value of the PMBus STATUS_WORD command for the rail.
     *
     * Reads the value from the specified device.  The returned value is in
     * host-endian order.
     *
     * Throws an exception if the value could not be obtained.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @return STATUS_WORD value
     */
    uint16_t getStatusWord(PowerSequencerDevice& device);

    /**
     * Returns the value of the PMBus STATUS_VOUT command for the rail.
     *
     * Reads the value from the specified device.
     *
     * Throws an exception if the value could not be obtained.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @return STATUS_VOUT value
     */
    uint8_t getStatusVout(PowerSequencerDevice& device);

    /**
     * Returns the value of the PMBus READ_VOUT command for the rail.
     *
     * Reads the value from the specified device.  The returned value is in
     * volts.
     *
     * Throws an exception if the value could not be obtained.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @return READ_VOUT value in volts
     */
    double getReadVout(PowerSequencerDevice& device);

    /**
     * Returns the value of the PMBus VOUT_UV_FAULT_LIMIT command for the rail.
     *
     * Reads the value from the specified device.  The returned value is in
     * volts.
     *
     * Throws an exception if the value could not be obtained.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @return VOUT_UV_FAULT_LIMIT value in volts
     */
    double getVoutUVFaultLimit(PowerSequencerDevice& device);

    /**
     * Returns whether a pgood (power good) fault has occurred on the rail.
     *
     * Throws an exception if an error occurs while trying to obtain the rail
     * status.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @param services System services like hardware presence and the journal
     * @param gpioValues GPIO values obtained from the device (if any)
     * @param additionalData Additional data to include in an error log if this
     *                       method returns true
     * @return true if a pgood fault was found on the rail, false otherwise
     */
    bool hasPgoodFault(PowerSequencerDevice& device, Services& services,
                       const std::vector<int>& gpioValues,
                       std::map<std::string, std::string>& additionalData);

  private:
    /**
     * Verifies that a PMBus PAGE number is defined for the rail.
     *
     * Throws an exception if a PAGE number is not defined.
     */
    void verifyHasPage();

    /**
     * Returns whether the PMBus STATUS_VOUT command indicates a pgood fault
     * has occurred on the rail.
     *
     * Throws an exception if an error occurs while trying to obtain the rail
     * status.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @param services System services like hardware presence and the journal
     * @param additionalData Additional data to include in an error log if this
     *                       method returns true
     * @return true if a pgood fault was found on the rail, false otherwise
     */
    bool hasPgoodFaultStatusVout(
        PowerSequencerDevice& device, Services& services,
        std::map<std::string, std::string>& additionalData);

    /**
     * Returns whether a GPIO value indicates a pgood fault has occurred on the
     * rail.
     *
     * Throws an exception if an error occurs while trying to obtain the rail
     * status.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @param gpioValues GPIO values obtained from the device (if any)
     * @param additionalData Additional data to include in an error log if this
     *                       method returns true
     * @return true if a pgood fault was found on the rail, false otherwise
     */
    bool hasPgoodFaultGPIO(Services& services,
                           const std::vector<int>& gpioValues,
                           std::map<std::string, std::string>& additionalData);

    /**
     * Returns whether the output voltage is below the undervoltage limit
     * indicating a pgood fault has occurred on the rail.
     *
     * Throws an exception if an error occurs while trying to obtain the rail
     * status.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @param services System services like hardware presence and the journal
     * @param additionalData Additional data to include in an error log if this
     *                       method returns true
     * @return true if a pgood fault was found on the rail, false otherwise
     */
    bool hasPgoodFaultOutputVoltage(
        PowerSequencerDevice& device, Services& services,
        std::map<std::string, std::string>& additionalData);

    /**
     * Store pgood fault debug data in the specified additional data map.
     *
     * Stores data that is relevant regardless of which method was used to
     * detect the pgood fault.
     *
     * @param device Power sequencer device that enables and monitors the rail
     * @param services System services like hardware presence and the journal
     * @param additionalData Additional data to include in an error log
     */
    void storePgoodFaultDebugData(
        PowerSequencerDevice& device, Services& services,
        std::map<std::string, std::string>& additionalData);

    /**
     * Unique name for the rail.
     */
    std::string name{};

    /**
     * D-Bus inventory path of a system component which must be present in order
     * for the rail to be present.
     *
     * If not specified, the rail is assumed to always be present.
     */
    std::optional<std::string> presence{};

    /**
     * PMBus PAGE number of the rail.
     */
    std::optional<uint8_t> page{};

    /**
     * Specifies whether the rail is produced by a power supply.
     */
    bool isPsuRail{false};

    /**
     * Specifies whether to check the value of the PMBus STATUS_VOUT command
     * when determining the pgood status of the rail.
     *
     * If one of the error bits is set in STATUS_VOUT, the rail pgood will be
     * considered false.
     */
    bool checkStatusVout{false};

    /**
     * Specifies whether to compare the output voltage to the undervoltage fault
     * limit when determining the pgood status of the rail.
     *
     * If the output voltage is below this limit, the rail pgood will be
     * considered false.
     *
     * Uses the values of the PMBus READ_VOUT and VOUT_UV_FAULT_LIMIT commands.
     */
    bool compareVoltageToLimit{false};

    /**
     * GPIO to read to determine the pgood status of the rail.
     */
    std::optional<GPIO> gpio{};
};

} // namespace phosphor::power::sequencer
