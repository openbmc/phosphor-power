/**
 * Copyright © 2026 IBM Corporation
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
#include <stdexcept>
#include <string>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class CompareVPDBytesAction
 *
 * Compares a VPD (Vital Product Data) keyword value to an array of expected
 * byte values, with optional bit masks.
 *
 * Implements the compare_vpd_bytes action in the JSON config file.
 */
class CompareVPDBytesAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    CompareVPDBytesAction() = delete;
    CompareVPDBytesAction(const CompareVPDBytesAction&) = delete;
    CompareVPDBytesAction(CompareVPDBytesAction&&) = delete;
    CompareVPDBytesAction& operator=(const CompareVPDBytesAction&) = delete;
    CompareVPDBytesAction& operator=(CompareVPDBytesAction&&) = delete;
    virtual ~CompareVPDBytesAction() = default;

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param fru Field-Replaceable Unit (FRU). Specify the D-Bus inventory path
     *            of the FRU.
     * @param keyword VPD keyword. Specify one of the following: "CCIN",
     *                "Manufacturer", "Model", "PartNumber", "HW".
     * @param values Zero or more expected byte values.
     */
    explicit CompareVPDBytesAction(const std::string& fru,
                                   const std::string& keyword,
                                   const std::vector<uint8_t>& values) :
        CompareVPDBytesAction(fru, keyword, values,
                              std::vector<uint8_t>(values.size(), 0xFF))
    {}

    /**
     * Constructor.
     *
     * Throws an exception if any of the input parameters are invalid.
     *
     * @param fru Field-Replaceable Unit (FRU). Specify the D-Bus inventory path
     *            of the FRU.
     * @param keyword VPD keyword. Specify one of the following: "CCIN",
     *                "Manufacturer", "Model", "PartNumber", "HW".
     * @param values Zero or more expected byte values.
     * @param masks Zero or more bit masks. The number of bit masks must match
     *              the number of expected byte values. Each mask specifies
     *              which bits should be compared within the corresponding byte
     *              value. Only the bits with a value of 1 in the mask will be
     *              compared.
     */
    explicit CompareVPDBytesAction(
        const std::string& fru, const std::string& keyword,
        const std::vector<uint8_t>& values, const std::vector<uint8_t>& masks) :
        fru{fru}, keyword{keyword}, values{values}, masks{masks}
    {
        // Masks vector must have same size as values vector
        if (masks.size() != values.size())
        {
            throw std::invalid_argument{"Masks vector has invalid size"};
        }
    }

    /**
     * Executes this action.
     *
     * Compares the actual VPD keyword byte values to the expected byte values.
     * The number of actual bytes must equal the number of expected bytes;
     * otherwise the comparison returns false.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return true if the keyword value bytes equal the expected value bytes,
     *         otherwise returns false
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
     * Returns the expected byte values.
     *
     * @return expected values
     */
    const std::vector<uint8_t>& getValues() const
    {
        return values;
    }

    /**
     * Returns the bit masks.
     *
     * Each mask specifies which bits should be compared within the
     * corresponding byte value. Only the bits with a value of 1 in the mask
     * will be compared.
     *
     * @return bit masks
     */
    const std::vector<uint8_t>& getMasks() const
    {
        return masks;
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
     * Expected byte values.
     */
    const std::vector<uint8_t> values{};

    /**
     * Bit masks. Each mask specifies which bits should be compared within the
     * corresponding byte value. Only the bits with a value of 1 in the mask
     * will be compared.
     */
    const std::vector<uint8_t> masks{};
};

} // namespace phosphor::power::regulators
