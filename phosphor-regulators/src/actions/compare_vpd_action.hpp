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

#include "action.hpp"
#include "action_environment.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class CompareVPDAction
 *
 * Compares a VPD (Vital Product Data) keyword value to an expected value.
 *
 * Implements the compare_vpd action in the JSON config file.
 */
class CompareVPDAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    CompareVPDAction() = delete;
    CompareVPDAction(const CompareVPDAction&) = delete;
    CompareVPDAction(CompareVPDAction&&) = delete;
    CompareVPDAction& operator=(const CompareVPDAction&) = delete;
    CompareVPDAction& operator=(CompareVPDAction&&) = delete;
    virtual ~CompareVPDAction() = default;

    /**
     * Constructor.
     *
     * @param fru Field-Replaceable Unit (FRU). Specify the D-Bus inventory path
     *            of the FRU.
     * @param keyword VPD keyword. Specify one of the following: "CCIN",
     *                "Manufacturer", "Model", "PartNumber".
     * @param value Expected value
     */
    explicit CompareVPDAction(const std::string& fru,
                              const std::string& keyword,
                              const std::vector<uint8_t>& value) :
        fru{fru}, keyword{keyword}, value{value}
    {}

    /**
     * Executes this action.
     *
     * Compares the actual VPD keyword value to the expected value.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return true if the keyword value equals the expected value, otherwise
     *         returns false
     */
    virtual bool execute(ActionEnvironment& environment) override;

    /**
     * Returns the Field-Replaceable Unit (FRU).
     *
     * @return FRU
     */
    const std::string& getFRU() const
    {
        return fru;
    }

    /**
     * Returns the VPD keyword.
     *
     * @return keyword
     */
    const std::string& getKeyword() const
    {
        return keyword;
    }

    /**
     * Returns the expected value.
     *
     * @return value
     */
    const std::vector<uint8_t>& getValue() const
    {
        return value;
    }

    /**
     * Returns a string description of this action.
     *
     * @return description of action
     */
    virtual std::string toString() const override;

  private:
    /**
     * Field-Replaceable Unit (FRU) for this action.
     *
     * The D-Bus inventory path of the FRU.
     */
    const std::string fru{};

    /**
     * VPD keyword.
     */
    const std::string keyword{};

    /**
     * Expected value.
     */
    const std::vector<uint8_t> value{};
};

} // namespace phosphor::power::regulators
