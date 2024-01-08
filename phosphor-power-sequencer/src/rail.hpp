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

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

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
     *             checkStatusVout or compareVoltageToLimits is true.
     * @param checkStatusVout Specifies whether to check the value of the PMBus
     *                        STATUS_VOUT command when determining the pgood
     *                        status of the rail
     * @param compareVoltageToLimits Specifies whether to compare the output
     *                               voltage to the undervoltage and
     *                               overvoltage limits when determining the
     *                               pgood status of the rail
     * @param gpio Optional GPIO to read to determine the pgood status of the
     *             rail
     */
    explicit Rail(const std::string& name,
                  const std::optional<std::string>& presence,
                  const std::optional<uint8_t>& page, bool checkStatusVout,
                  bool compareVoltageToLimits,
                  const std::optional<GPIO>& gpio) :
        name{name},
        presence{presence}, page{page}, checkStatusVout{checkStatusVout},
        compareVoltageToLimits{compareVoltageToLimits}, gpio{gpio}
    {
        // If checking STATUS_VOUT or output voltage, verify PAGE was specified
        if ((checkStatusVout || compareVoltageToLimits) && !page.has_value())
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
     * and overvoltage limits when determining the pgood status of the rail.
     *
     * @return true if output voltage is compared to limits, false otherwise
     */
    bool getCompareVoltageToLimits() const
    {
        return compareVoltageToLimits;
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

  private:
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
     * Specifies whether to check the value of the PMBus STATUS_VOUT command
     * when determining the pgood status of the rail.
     *
     * If one of the error bits is set in STATUS_VOUT, the rail pgood will be
     * considered false.
     */
    bool checkStatusVout{false};

    /**
     * Specifies whether to compare the output voltage to the undervoltage and
     * overvoltage limits when determining the pgood status of the rail.
     *
     * If the output voltage is beyond those limits, the rail pgood will be
     * considered false.
     *
     * Uses the values of the PMBus READ_VOUT, VOUT_UV_FAULT_LIMIT, and
     * VOUT_OV_FAULT_LIMIT commands.
     */
    bool compareVoltageToLimits{false};

    /**
     * GPIO to read to determine the pgood status of the rail.
     */
    std::optional<GPIO> gpio{};
};

} // namespace phosphor::power::sequencer
