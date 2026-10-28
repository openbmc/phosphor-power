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
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class CompareVPDByteAction
 *
 * Compares a byte within a VPD (Vital Product Data) keyword value to an
 * expected byte value, with an optional bit mask.
 *
 * Implements the compare_vpd_byte action in the JSON config file.
 */
class CompareVPDByteAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    CompareVPDByteAction() = delete;
    CompareVPDByteAction(const CompareVPDByteAction&) = delete;
    CompareVPDByteAction(CompareVPDByteAction&&) = delete;
    CompareVPDByteAction& operator=(const CompareVPDByteAction&) = delete;
    CompareVPDByteAction& operator=(CompareVPDByteAction&&) = delete;
    virtual ~CompareVPDByteAction() = default;

    /**
     * Constructor.
     *
     * @param fru Field-Replaceable Unit (FRU). Specify the D-Bus inventory path
     *            of the FRU.
     * @param keyword VPD keyword. Specify one of the following: "CCIN",
     *                "Manufacturer", "Model", "PartNumber", "HW".
     * @param position Zero-based byte position within the VPD keyword value.
     *                 Position 0 is the first byte.
     * @param value Expected byte value.
     * @param mask Bit mask. Specifies which bits should be compared within the
     *             byte value. Only the bits with a value of 1 in the mask will
     *             be compared. If not specified, defaults to 0xFF which means
     *             that all bits will be compared.
     */
    explicit CompareVPDByteAction(
        const std::string& fru, const std::string& keyword,
        unsigned int position, uint8_t value, uint8_t mask = 0xFF) :
        fru{fru}, keyword{keyword}, position{position}, value{value}, mask{mask}
    {}

    /**
     * Executes this action.
     *
     * Compares the actual byte at the specified position within the VPD keyword
     * value to the expected byte value.
     *
     * If the byte position is past the end of the actual VPD byte array, the
     * comparison returns false.
     *
     * Throws an exception if an error occurs.
     *
     * @param environment action execution environment
     * @return true if the keyword value byte equals the expected byte value,
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
     * Returns the byte position within the VPD keyword value.
     *
     * @return byte position
     */
    unsigned int getPosition() const
    {
        return position;
    }

    /**
     * Returns the expected byte value.
     *
     * @return expected value
     */
    uint8_t getValue() const
    {
        return value;
    }

    /**
     * Returns the bit mask.
     *
     * Specifies which bits should be compared within the byte value. Only the
     * bits with a value of 1 in the mask will be compared.
     *
     * @return bit mask
     */
    uint8_t getMask() const
    {
        return mask;
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
     * Byte position within the VPD keyword value.
     */
    const unsigned int position{0};

    /**
     * Expected byte value.
     */
    const uint8_t value{0x00};

    /**
     * Bit mask. Specifies which bits should be compared within the byte value.
     * Only the bits with a value of 1 in the mask will be compared.
     */
    const uint8_t mask{0xFF};
};

} // namespace phosphor::power::regulators
