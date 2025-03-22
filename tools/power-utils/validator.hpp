/**
 * Copyright © 2025 IBM Corporation
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

#include "utility.hpp"

namespace validator
{
using namespace phosphor::power::util;
/**
 * @class PSUUpdateValidator
 * @brief This class validates PSU configurations in OpenBMC.
 */
class PSUUpdateValidator
{
  public:
    /**
     * @brief Constructor - Initializes D-Bus connection.
     */
    // PSUUpdateValidator(sdbusplus::bus_t& bus);
    PSUUpdateValidator(sdbusplus::bus_t& bus, const std::string& psuPath) :
        bus(bus), psuPath(psuPath)
    {}

    /**
     * @brief Checks if all PSUs are of the same model.
     *
     * @param psuPath Path of the reference PSU.
     * @return true if all PSUs have the same model.
     * @return false if any PSU has a different model.
     */
    // bool areAllPsuSameModel(const std::string& psuPath);
    bool areAllPsuSameModel();

    /**
     * @brief Counts the number of PSUs that are physically present.
     *
     * @return true if successfully counted present PSUs.
     * @return false on failure.
     */
    bool countPresentPsus();

    /**
     * @brief Retrieves the required number of PSUs for redundancy.
     *
     * @return true if successfully retrieved redundancy count.
     * @return false on failure.
     */
    bool getRequiredPsus();

    bool validToUpdate();

  private:
    sdbusplus::bus::bus& bus;          ///< D-Bus connection instance
    std::vector<std::string> psuPaths; ///< List of PSU object paths
    std::string targetPsuModel;        ///< Model name of the reference PSU
    std::string psuPath;               ///< Model name of the reference PSU
    DbusSubtree supportedObjects;      ///< D-Bus PSU supported objects
    DbusPropertyMap properties;        ///< D-Bus PSU properties
    uint64_t presentPsuCount = 0;      ///< Count of physically present PSUs
    uint64_t redundantCount = 0;       ///< Number of redundant PSUs
};

} // namespace validator
