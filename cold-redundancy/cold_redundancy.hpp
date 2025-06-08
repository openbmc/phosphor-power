/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <boost/asio.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <util.hpp>

/**
 * @class ColdRedundancy
 *
 */
class ColdRedundancy
{
  public:
    /**
     * Constructor
     *
     * @param[in] io - boost asio context
     * @param[in] dbusConnection - D-Bus connection
     */
    ColdRedundancy(
        boost::asio::io_context& io,
        std::shared_ptr<sdbusplus::asio::connection>& dbusConnection);

    /**
     * Checking PSU information, adding matches, starting rotation
     * and creating PSU objects
     *
     * @param[in] dbusConnection - D-Bus connection
     */
    void createPSU(
        std::shared_ptr<sdbusplus::asio::connection>& dbusConnection);

  private:
    /**
     * @brief Indicates the count of PSUs
     *
     * @details Indicates how many PSUs are there on the system.
     */
    uint8_t numberOfPSU = 0;

    /**
     * @brief Indicates the delay timer
     *
     * @details Each time this daemon start, need a delay to avoid
     *          different PSUs calling createPSU function for
     *          several times at same time
     */
    boost::asio::steady_timer filterTimer;

    /**
     * @brief Indicates the dbus connction
     */
    [[maybe_unused]] std::shared_ptr<sdbusplus::asio::connection>& systemBus;

    /**
     * @brief Indicates the D-Bus matches
     *
     * @details This matches contain all matches in this daemon such
     *          as PSU event match, PSU information match. The target
     *          D-Bus properties change will trigger callback function
     *          by these matches
     */
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> matches;
};

/**
 * @class PowerSupply
 * Represents a power supply device.
 */
class PowerSupply
{
  public:
    /**
     * Constructor
     *
     * @param[in] name - the device name
     * @param[in] bus - smbus number
     * @param[in] address - device address on smbus
     * @param[in] order - ranking order of redundancy
     * @param[in] dbusConnection - D-Bus connection
     */
    PowerSupply(
        std::string& name, uint8_t bus, uint8_t address, uint8_t order,
        const std::shared_ptr<sdbusplus::asio::connection>& dbusConnection);
    ~PowerSupply() = default;

    /**
     * @brief Indicates the name of the device
     *
     * @details The PSU name such as PSU1
     */
    std::string name;

    /**
     * @brief Indicates the smbus number
     *
     * @details The smbus number on the system
     */
    uint8_t bus;

    /**
     * @brief Indicates the smbus address of the device
     *
     * @details The 7-bit smbus address of the PSU on smbus
     */
    uint8_t address;

    /**
     * @brief Indicates the ranking order
     *
     * @details The order indicates the sequence entering standby mode.
     *          the PSU with lower order will enter standby mode first.
     */
    uint8_t order = 0;

    /**
     * @brief Indicates the status of the PSU
     *
     * @details If the PSU has no any problem, the status of it will be
     *          normal otherwise acLost.
     */
    CR::PSUState state = CR::PSUState::normal;
};
