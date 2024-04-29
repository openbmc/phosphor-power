/**
 * Copyright © 2020 IBM Corporation
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

#include <exception>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class PMBusError
 *
 * An error that occurred while sending PMBus commands or converting data
 * between PMBus formats.
 */
class PMBusError : public std::exception
{
  public:
    // Specify which compiler-generated methods we want
    PMBusError() = delete;
    PMBusError(const PMBusError&) = default;
    PMBusError(PMBusError&&) = default;
    PMBusError& operator=(const PMBusError&) = default;
    PMBusError& operator=(PMBusError&&) = default;
    virtual ~PMBusError() = default;

    /**
     * Constructor.
     *
     * @param error error message
     * @param deviceID unique ID of the device where error occurred
     * @param inventoryPath inventory path of the device where error occurred
     */
    explicit PMBusError(const std::string& error, const std::string& deviceID,
                        const std::string& inventoryPath) :
        error{"PMBusError: " + error}, deviceID{deviceID},
        inventoryPath{inventoryPath}
    {}

    /**
     * Returns the unique ID of the device where the error occurred.
     *
     * @return device ID
     */
    const std::string& getDeviceID() const
    {
        return deviceID;
    }

    /**
     * Returns the inventory path of the device where the error occurred.
     *
     * @return inventory path
     */
    const std::string& getInventoryPath() const
    {
        return inventoryPath;
    }

    /**
     * Returns the description of this error.
     *
     * @return error description
     */
    const char* what() const noexcept override
    {
        return error.c_str();
    }

  private:
    /**
     * Error message.
     */
    const std::string error{};

    /**
     * Unique ID of the device where the error occurred.
     */
    const std::string deviceID{};

    /**
     * Inventory path of the device where the error occurred.
     */
    const std::string inventoryPath{};
};

} // namespace phosphor::power::regulators
