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

#include <libpldm/instance-id.h>
#include <libpldm/platform.h>
#include <libpldm/state_set.h>
#include <libpldm/transport.h>
#include <libpldm/transport/mctp-demux.h>

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/**
 * @class PldmFetcher
 * @brief Fetches and displays PLDM chassis state sensor data.
 */
class PldmFetcher
{
  public:
    /**
     * @param[in] bus         D-Bus connection for PDR and Inventory queries.
     * @param[in] isVerbose   Enable verbose diagnostic output.
     * @param[in] numChassis  Number of chassis to scan for entity matching.
     * @param[in] oemIbm      Use IBM OEM location codes for entity matching.
     *                        When false (default), serial numbers are used.
     * @param[in] mctpEid     MCTP endpoint ID.
     */
    PldmFetcher(sdbusplus::bus_t& bus, bool isVerbose, int numChassis,
                bool oemIbm = false, uint8_t mctpEid = defaultMctpEid,
                const std::map<std::string, bool>& pldmPropMap = {});
    PldmFetcher(const PldmFetcher&) = delete;
    PldmFetcher& operator=(const PldmFetcher&) = delete;
    PldmFetcher(PldmFetcher&&) = delete;
    PldmFetcher& operator=(PldmFetcher&&) = delete;
    ~PldmFetcher();

    /** Returns true if the MCTP transport was successfully opened. */
    bool isTransportOpen() const
    {
        return transport != nullptr;
    }

    /** Display PLDM-sourced chassis status for the given chassis number.
     *  Only properties enabled in the pldmPropMap passed at construction
     *  are printed. */
    void display(int chassisNumber) const;

    // Default MCTP EID for the host PLDM terminus (matches pldmtool default).
    static constexpr uint8_t defaultMctpEid = 8;

    static constexpr auto smallIndent = "    ";
    static constexpr auto largeIndent = "       ";

  private:
    // PLDM entity type for physical chassis (45)
    static constexpr uint16_t chassisEntityType = 45;

    static constexpr auto pldmService = "xyz.openbmc_project.PLDM";

    // State set 2 — Availability
    inline static const std::map<uint8_t, std::string_view>
        availabilityStateNames = {
            {PLDM_STATE_SET_AVAILABILITY_ENABLED, "Enabled"},
            {PLDM_STATE_SET_AVAILABILITY_DISABLED, "Disabled"},
            {PLDM_STATE_SET_AVAILABILITY_REBOOTING, "Rebooting"},
    };

    // State set 10 — Operational Fault Status
    inline static const std::map<uint8_t, std::string_view>
        operationalFaultStateNames = {
            {PLDM_STATE_SET_OPERATIONAL_FAULT_STATUS_NORMAL, "Normal"},
            {PLDM_STATE_SET_OPERATIONAL_FAULT_STATUS_ERROR, "Fault"},
            {PLDM_STATE_SET_OPERATIONAL_FAULT_STATUS_NON_RECOVERABLE_ERROR,
             "Non-Recoverable Fault"},
    };

    // State set 13 — Presence
    inline static const std::map<uint8_t, std::string_view> presenceStateNames =
        {
            {PLDM_STATE_SET_PRESENCE_PRESENT, "Present"},
            {PLDM_STATE_SET_PRESENCE_NOT_PRESENT, "Not Present"},
    };

    // State set 260 — System Power State
    inline static const std::map<uint8_t, std::string_view>
        systemPowerStateNames = {
            {PLDM_STATE_SET_SYS_POWER_STATE_ON, "On"},
            {PLDM_STATE_SET_SYS_POWER_STATE_HIBERNATE, "Hibernate"},
            {PLDM_STATE_SET_SYS_POWER_STATE_SLEEP_LIGHT, "Sleep (Light)"},
            {PLDM_STATE_SET_SYS_POWER_STATE_SLEEP_DEEP, "Sleep (Deep)"},
            {PLDM_STATE_SET_SYS_POWER_CYCLE_SOFT, "Power Cycle (Soft)"},
            {PLDM_STATE_SET_SYS_POWER_CYCLE_HARD, "Power Cycle (Hard)"},
            {PLDM_STATE_SET_SYS_POWER_CYCLE_OFF_SOFT_GRACEFUL,
             "Power Cycle Off-Soft Graceful"},
            {PLDM_STATE_SET_SYS_POWER_CYCLE_OFF_HARD_GRACEFUL,
             "Power Cycle Off-Hard Graceful"},
            {PLDM_STATE_SET_SYS_POWER_STATE_OFF_SOFT_GRACEFUL,
             "Off-Soft Graceful"},
            {PLDM_STATE_SET_SYS_POWER_STATE_OFF_HARD_GRACEFUL,
             "Off-Hard Graceful"},
            {PLDM_STATE_SET_SYS_POWER_STATE_MASTER_BUS_RESET,
             "Master Bus Reset"},
            {PLDM_STATE_SET_SYS_POWER_STATE_MASTER_BUS_RESET_GRACEFUL,
             "Master Bus Reset Graceful"},
            {PLDM_STATE_SET_SYS_POWER_STATE_NMI, "NMI"},
    };

    struct SensorPDREntry
    {
        uint16_t sensorId;
        uint16_t entityInstance;
    };

    struct FruRSIEntry
    {
        uint16_t entityInstance;
        uint16_t rsi;
    };

    struct PldmPdrSets
    {
        std::vector<SensorPDREntry> availability;
        std::vector<SensorPDREntry> presence;
        std::vector<SensorPDREntry> powerState;
        std::vector<SensorPDREntry> operationalFault;
    };

    // Map of properties to display.
    std::map<std::string, bool> pldmPropMap;

    // Flag for verbose output.
    bool verbose;

    // D-Bus connection used for Inventory and PDR queries.
    sdbusplus::bus_t& bus;

    // Terminus ID
    pldm_tid_t tid{0};

    // MCTP demux transport instance used to send and receive PLDM messages.
    struct pldm_transport_mctp_demux* demux{nullptr};

    // PLDM instance ID database used to allocate per-request instance IDs.
    struct pldm_instance_db* instanceDb{nullptr};

    // Abstract PLDM transport handle wrapping the MCTP demux transport.
    struct pldm_transport* transport{nullptr};

    // Cached PDR entries for each monitored state set, indexed by chassis.
    PldmPdrSets pdrSets;

    // Cached FRU Record Set PDR entries for chassis entities.
    std::vector<FruRSIEntry> fruRsiEntries;

    // Whether to attempt IBM OEM location-code matching.
    bool oemIbm;

    /**
     * IBM OEM FRU location codes keyed by entityInstance.
     * Populated only when oemIbm is true.
     */
    std::map<uint16_t, std::string> fruLocationCache;

    /**
     * Standard FRU serial numbers keyed by entityInstance.
     */
    std::map<uint16_t, std::string> fruSerialCache;

    /**
     * Maps chassisNumber to PLDM entityInstance.
     * Derived by matching FRU location codes or serial numbers
     * against Inventory Manager.
     */
    std::map<int, uint16_t> chassisToEntityInstance;

    /**
     * Queries pldmd via D-Bus for state sensor PDRs.
     *
     * @param[in] stateSetId  PLDM state set to search for.
     * @param[in] entityType  Entity type to filter by.
     *
     * @return Matching sensor PDR entries, or an empty vector if none found.
     */
    std::vector<SensorPDREntry> fetchSensorPDRs(uint16_t stateSetId,
                                                uint16_t entityType) const;

    /**
     * Iterates the PLDM PDR repository via MCTP GetPDR requests and returns
     * all FRU Record Set PDRs whose entity_type matches chassisEntityType.
     *
     * @return Vector of FruRSIEntry with entityInstance and RSI, or an empty
     *         vector on failure.
     */
    std::vector<FruRSIEntry> fetchFruRecordSetPDRs();

    /**
     * Allocates a PLDM instance ID for a request/response transaction.
     *
     * @param[in] operation  Operation name used in verbose error messages.
     *
     * @return Allocated instance ID, or std::nullopt on failure.
     */
    std::optional<pldm_instance_id_t> allocateInstanceId(
        std::string_view operation) const;

    /**
     * Encodes a GetPDR request.
     *
     * @param[in] instanceId    PLDM instance ID for the request.
     * @param[in] recordHandle  PDR record handle to fetch.
     *
     * @return Encoded request bytes, or std::nullopt on failure.
     */
    std::optional<std::vector<uint8_t>> encodeGetPdrRequest(
        pldm_instance_id_t instanceId, uint32_t recordHandle) const;

    /**
     * Decodes a GetPDR response and appends matching FRU Record Set entries.
     *
     * @param[in] recordHandle  PDR record handle used for the request.
     * @param[in] response      Full response buffer (header + payload).
     * @param[out] results      Matching FRU Record Set entries collected so
     * far.
     *
     * @return Next record handle, or std::nullopt on failure.
     */
    std::optional<uint32_t> decodeGetPdrResponse(
        uint32_t recordHandle, std::span<const std::byte> response,
        std::vector<FruRSIEntry>& results) const;

    /**
     * Encodes a GetStateSensorReadings request.
     *
     * @param[in] instanceId  PLDM instance ID for the request.
     * @param[in] sensorId    Sensor ID to read.
     *
     * @return Encoded request bytes, or std::nullopt on failure.
     */
    std::optional<std::vector<uint8_t>> encodeGetStateSensorReadingsRequest(
        pldm_instance_id_t instanceId, uint16_t sensorId) const;

    /**
     * Encodes a GetFRURecordByOption request for the chassis FRU record.
     *
     * @param[in] instanceId  PLDM instance ID for the request.
     * @param[in] rsi         FRU Record Set Identifier to query.
     * @param[in] recordType  FRU record type (e.g. PLDM_FRU_RECORD_TYPE_OEM).
     * @param[in] fieldType   FRU field type to filter by.
     * @param[in] operation   Operation name used in verbose error messages.
     *
     * @return Encoded request bytes, or std::nullopt on failure.
     */
    std::optional<std::vector<uint8_t>> encodeGetFruRecordByOptionRequest(
        pldm_instance_id_t instanceId, uint16_t rsi, uint8_t recordType,
        uint8_t fieldType, std::string_view operation) const;

    /**
     * Decodes a GetFRURecordByOption response.
     *
     * @param[in] rsi       FRU Record Set Identifier used for the request.
     * @param[in] response  Full response buffer (header + payload).
     *
     * @return FRU table bytes, or std::nullopt on failure.
     */
    std::optional<std::vector<uint8_t>> decodeGetFruRecordByOptionResponse(
        uint16_t rsi, std::span<const std::byte> response) const;

    /**
     * Finds a field in raw FRU table data by record type and field type.
     *
     * @param[in] fruBytes    Raw FRU table bytes.
     * @param[in] recordType  FRU record type to match.
     * @param[in] fieldType   TLV field type to match.
     *
     * @return Field value string, or std::nullopt if not found.
     */
    std::optional<std::string> findFruField(
        const std::vector<uint8_t>& fruBytes, uint8_t recordType,
        uint8_t fieldType) const;

    /**
     * Fetches a single FRU field for a chassis via a targeted MCTP
     * GetFRURecordByOption request.
     *
     * @param[in] rsi         FRU Record Set Identifier to query.
     * @param[in] recordType  FRU record type to request.
     * @param[in] fieldType   FRU field type to retrieve.
     * @param[in] operation   Operation name used in verbose error messages.
     *
     * @return Field value string, or std::nullopt on failure or if not found.
     */
    std::optional<std::string> readPldmFruField(
        uint16_t rsi, uint8_t recordType, uint8_t fieldType,
        std::string_view operation) const;

    /**
     * Reads a string property from the Inventory Manager for the given chassis.
     *
     * @param[in] chassisNumber  Chassis index (0-based).
     * @param[in] interface      D-Bus interface name.
     * @param[in] property       Property name.
     *
     * @return Property value, or std::nullopt if not found or on error.
     */
    std::optional<std::string> readInventoryProperty(
        int chassisNumber, std::string_view interface,
        std::string_view property) const;

    /**
     * Finds the PDR entry for the given entity instance.
     *
     * @param[in] pdrs            PDR entries to search.
     * @param[in] entityInstance  Entity instance to match.
     *
     * @return Matching entry, or std::nullopt if not found.
     */
    std::optional<SensorPDREntry> findSensorEntry(
        const std::vector<SensorPDREntry>& pdrs, uint16_t entityInstance) const;

    /**
     * Prints the current state of a sensor. Reads the sensor value via MCTP
     * and maps it to a human-readable string using the provided state name map.
     *
     * @param[in] entry       PDR entry for the sensor, or std::nullopt if not
     *                        found.
     * @param[in] stateNames  Map of state values to display strings.
     */
    void printSensorState(
        const std::optional<SensorPDREntry>& entry,
        const std::map<uint8_t, std::string_view>& stateNames) const;

    /**
     * Decodes a GetStateSensorReadings response and returns the current state
     * of the first sensor component.
     *
     * @param[in] response  Full response buffer (header + payload).
     *
     * @return Current sensor state, or std::nullopt on failure.
     */
    std::optional<uint8_t> decodeStateSensorResponse(
        std::span<const std::byte> response) const;

    /**
     * Reads the current state of a sensor via MCTP.
     *
     * @param[in] sensorId  Sensor ID to read.
     *
     * @return Current sensor state, or std::nullopt on failure.
     */
    std::optional<uint8_t> readStateSensor(uint16_t sensorId) const;
};
