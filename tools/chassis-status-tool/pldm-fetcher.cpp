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

#include "pldm-fetcher.hpp"

#include <libpldm/fru.h>
#include <libpldm/platform.h>
#include <libpldm/state_set.h>

#include <format>
#include <print>
#include <vector>

constexpr auto smallIndent = "    ";
constexpr auto largeIndent = "       ";

PldmFetcher::PldmFetcher(sdbusplus::bus_t& bus, bool isVerbose) :
    verbose(isVerbose), bus(bus), tid(hostEID)
{
    // Fetch all PDRs up front via D-Bus
    pdrSets.availability.physical =
        fetchSensorPDRs(bus, PLDM_STATE_SET_AVAILABILITY, chassisEntityType);
    pdrSets.availability.logical = fetchSensorPDRs(
        bus, PLDM_STATE_SET_AVAILABILITY, logicalChassisEntityType);
    pdrSets.presence.physical =
        fetchSensorPDRs(bus, PLDM_STATE_SET_PRESENCE, chassisEntityType);
    pdrSets.presence.logical =
        fetchSensorPDRs(bus, PLDM_STATE_SET_PRESENCE, logicalChassisEntityType);
    pdrSets.powerState.physical = fetchSensorPDRs(
        bus, PLDM_STATE_SET_SYSTEM_POWER_STATE, chassisEntityType);
    pdrSets.powerState.logical = fetchSensorPDRs(
        bus, PLDM_STATE_SET_SYSTEM_POWER_STATE, logicalChassisEntityType);
    pdrSets.operationalFault.physical = fetchSensorPDRs(
        bus, PLDM_STATE_SET_OPERATIONAL_FAULT_STATUS, chassisEntityType);
    pdrSets.operationalFault.logical = fetchSensorPDRs(
        bus, PLDM_STATE_SET_OPERATIONAL_FAULT_STATUS, logicalChassisEntityType);

    // Open MCTP transport
    if (pldm_instance_db_init_default(&instanceDb) != 0)
    {
        if (verbose)
        {
            std::println(stderr, "{}Failed to open PLDM instance ID database",
                         largeIndent);
        }
        instanceDb = nullptr;
        return;
    }

    // Initialize the mctp-demux transport before creating the PLDM transport.
    if (pldm_transport_mctp_demux_init(&demux) != 0)
    {
        if (verbose)
        {
            std::println(stderr, "{}Failed to init mctp-demux transport",
                         largeIndent);
        }
        demux = nullptr;
        return;
    }

    // MCTP bus address assigned to this BMC.
    constexpr uint8_t bmcMctpEid = 8;
    pldm_transport_mctp_demux_map_tid(demux,
                                      static_cast<pldm_tid_t>(bmcMctpEid),
                                      static_cast<uint8_t>(bmcMctpEid));

    // Get the pldm_transport handle that wraps the demux backend.
    transport = pldm_transport_mctp_demux_core(demux);

    fruRsiEntries = fetchFruRecordSetPDRs();

    // One targeted GetFRURecordByOption request per chassis RSI.
    for (const auto& entry : fruRsiEntries)
    {
        auto serial = readPldmSerialNumber(entry.rsi);
        if (serial)
        {
            fruSerialCache[entry.entityInstance] = std::move(*serial);
        }
    }

    // Match PLDM FRU serials against D-Bus Inventory serial numbers to map
    // chassisNumber → entityInstance.
    for (const auto& [entityInstance, pldmSerial] : fruSerialCache)
    {
        for (int i = 0; i < maxChassisProbe; ++i)
        {
            auto invSerial = readInventorySerialNumber(i);
            if (invSerial && !invSerial->empty() && *invSerial == pldmSerial)
            {
                chassisToEntityInstance[i] = entityInstance;
                break;
            }
        }
    }
}

PldmFetcher::~PldmFetcher()
{
    if (demux != nullptr)
    {
        pldm_transport_mctp_demux_destroy(demux);
    }
    if (instanceDb != nullptr)
    {
        pldm_instance_db_destroy(instanceDb);
    }
}

std::optional<pldm_instance_id_t> PldmFetcher::allocateInstanceId(
    std::string_view operation) const
{
    pldm_instance_id_t instanceId{};
    if (pldm_instance_id_alloc(instanceDb, tid, &instanceId) == 0)
    {
        return instanceId;
    }

    if (verbose)
    {
        std::println(stderr, "{}{}: failed to allocate PLDM instance ID",
                     largeIndent, operation);
    }

    return std::nullopt;
}

std::optional<std::vector<uint8_t>> PldmFetcher::encodeGetPdrRequest(
    pldm_instance_id_t instanceId, uint32_t recordHandle) const
{
    std::vector<uint8_t> requestMsg(
        sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REQ_BYTES);
    auto* request = new (requestMsg.data()) pldm_msg;
    const auto rc = encode_get_pdr_req(
        instanceId, recordHandle,
        0,                  // dataTransferHandle
        PLDM_GET_FIRSTPART, // transferOpFlag
        UINT16_MAX,         // requestCount: fetch all
        0,                  // recordChangeNumber
        request, PLDM_GET_PDR_REQ_BYTES);
    if (rc == PLDM_SUCCESS)
    {
        return requestMsg;
    }

    pldm_instance_id_free(instanceDb, tid, instanceId);
    if (verbose)
    {
        std::println(
            stderr, "{}fetchFruRecordSetPDRs: encode_get_pdr_req failed: rc={}",
            largeIndent, rc);
    }

    return std::nullopt;
}

std::optional<std::vector<uint8_t>>
    PldmFetcher::encodeGetStateSensorReadingsRequest(
        pldm_instance_id_t instanceId, uint16_t sensorId) const
{
    std::vector<uint8_t> requestMsg(
        sizeof(pldm_msg_hdr) + PLDM_GET_STATE_SENSOR_READINGS_REQ_BYTES);
    auto* request = new (requestMsg.data()) pldm_msg;
    bitfield8_t rearm{0};
    const auto rc = encode_get_state_sensor_readings_req(instanceId, sensorId,
                                                         rearm, 0, request);
    if (rc == PLDM_SUCCESS)
    {
        return requestMsg;
    }

    pldm_instance_id_free(instanceDb, tid, instanceId);
    if (verbose)
    {
        std::println(stderr,
                     "{}Failed to encode GetStateSensorReadings request "
                     "for sensorID={}: rc={}",
                     largeIndent, sensorId, rc);
    }

    return std::nullopt;
}

std::optional<uint32_t> PldmFetcher::decodeGetPdrResponse(
    uint32_t recordHandle, void* responseMsg, size_t responseSize,
    std::vector<FruRSIEntry>& results) const
{
    uint8_t completionCode{};
    uint32_t nextRecordHandle{};
    uint32_t nextDataTransferHandle{};
    uint8_t transferFlag{};
    uint16_t respCnt{};
    uint8_t transferCRC{};
    std::vector<uint8_t> pdrData(UINT16_MAX);

    const auto payloadLen = responseSize - sizeof(pldm_msg_hdr);
    const auto rc = decode_get_pdr_resp(
        static_cast<pldm_msg*>(responseMsg), payloadLen, &completionCode,
        &nextRecordHandle, &nextDataTransferHandle, &transferFlag, &respCnt,
        pdrData.data(), pdrData.size(), &transferCRC);
    free(responseMsg);

    if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
    {
        if (verbose)
        {
            std::println(stderr,
                         "{}fetchFruRecordSetPDRs: decode_get_pdr_resp failed "
                         "for recordHandle={}: rc={} cc={}",
                         largeIndent, recordHandle, rc,
                         static_cast<unsigned>(completionCode));
        }
        return std::nullopt;
    }

    // Only parse the PDR body when enough bytes arrived for both the common
    // header and the FRU Record Set fixed fields.
    if (respCnt >= sizeof(pldm_pdr_hdr) + sizeof(pldm_pdr_fru_record_set))
    {
        const auto* hdr = reinterpret_cast<const pldm_pdr_hdr*>(pdrData.data());
        if (hdr->type == PLDM_PDR_FRU_RECORD_SET)
        {
            const auto* fruPdr =
                reinterpret_cast<const pldm_pdr_fru_record_set*>(
                    pdrData.data() + sizeof(pldm_pdr_hdr));
            if (fruPdr->entity_type == chassisEntityType)
            {
                results.push_back({fruPdr->entity_instance, fruPdr->fru_rsi});
            }
        }
    }

    return nextRecordHandle;
}

std::vector<PldmFetcher::FruRSIEntry> PldmFetcher::fetchFruRecordSetPDRs()
{
    std::vector<FruRSIEntry> results;
    uint32_t recordHandle = 0;

    do
    {
        const auto instanceId = allocateInstanceId("fetchFruRecordSetPDRs");
        if (!instanceId)
        {
            break;
        }

        const auto requestMsg = encodeGetPdrRequest(*instanceId, recordHandle);
        if (!requestMsg)
        {
            break;
        }

        void* responseMsg = nullptr;
        size_t responseSize{};
        const auto rc = pldm_transport_send_recv_msg(
            transport, tid, requestMsg->data(), requestMsg->size(),
            &responseMsg, &responseSize);
        pldm_instance_id_free(instanceDb, tid, *instanceId);

        if (rc != PLDM_REQUESTER_SUCCESS)
        {
            if (verbose)
            {
                std::println(stderr,
                             "{}fetchFruRecordSetPDRs: GetPDR send/recv failed "
                             "for recordHandle={}: rc={}",
                             largeIndent, recordHandle, static_cast<int>(rc));
            }
            break;
        }

        // Decode the current GetPDR response, append any matching FRU Record
        // Set
        const auto nextRecordHandle = decodeGetPdrResponse(
            recordHandle, responseMsg, responseSize, results);
        if (!nextRecordHandle)
        {
            break;
        }

        recordHandle = *nextRecordHandle;
    } while (recordHandle != 0);

    return results;
}

std::optional<std::vector<uint8_t>>
    PldmFetcher::encodeGetFruRecordByOptionRequest(
        pldm_instance_id_t instanceId, uint16_t rsi) const
{
    const auto payloadLength = sizeof(pldm_get_fru_record_by_option_req);
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) + payloadLength);
    auto* request = new (requestMsg.data()) pldm_msg;
    const auto rc = encode_get_fru_record_by_option_req(
        instanceId,
        0,                            // dataTransferHandle
        0,                            // fruTableHandle
        rsi,                          // recordSetIdentifier
        PLDM_FRU_RECORD_TYPE_GENERAL, // recordType
        PLDM_FRU_FIELD_TYPE_SN,       // fieldType
        PLDM_GET_FIRSTPART, request, payloadLength);
    if (rc == PLDM_SUCCESS)
    {
        return requestMsg;
    }

    pldm_instance_id_free(instanceDb, tid, instanceId);
    if (verbose)
    {
        std::println(stderr,
                     "{}readPldmSerialNumber: encode request failed for "
                     "rsi={}: rc={}",
                     largeIndent, rsi, rc);
    }

    return std::nullopt;
}

std::optional<std::vector<uint8_t>>
    PldmFetcher::decodeGetFruRecordByOptionResponse(
        uint16_t rsi, void* responseMsg, size_t responseSize) const
{
    uint8_t completionCode{};
    uint32_t nextTransferHandle{};
    uint8_t transferFlag{};
    variable_field fruData{};
    const auto payloadLen = responseSize - sizeof(pldm_msg_hdr);
    const auto rc = decode_get_fru_record_by_option_resp(
        static_cast<pldm_msg*>(responseMsg), payloadLen, &completionCode,
        &nextTransferHandle, &transferFlag, &fruData);

    std::vector<uint8_t> fruBytes;
    if (rc == PLDM_SUCCESS && completionCode == PLDM_SUCCESS &&
        fruData.ptr != nullptr && fruData.length > 0)
    {
        fruBytes.assign(fruData.ptr, fruData.ptr + fruData.length);
    }
    free(responseMsg);

    if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
    {
        if (verbose)
        {
            std::println(stderr,
                         "{}readPldmSerialNumber: decode failed for rsi={}: "
                         "rc={} cc={}",
                         largeIndent, rsi, rc,
                         static_cast<unsigned>(completionCode));
        }
        return std::nullopt;
    }

    return fruBytes;
}

std::optional<std::string> PldmFetcher::findFruSerialNumber(
    const std::vector<uint8_t>& fruBytes) const
{
    const auto* p = fruBytes.data();
    const auto* end = p + fruBytes.size();

    // Walk the raw FRU table
    while (
        p + sizeof(pldm_fru_record_data_format) - sizeof(pldm_fru_record_tlv) <=
        end)
    {
        const auto* rec =
            reinterpret_cast<const pldm_fru_record_data_format*>(p);
        p += sizeof(pldm_fru_record_data_format) - sizeof(pldm_fru_record_tlv);

        for (uint8_t field = 0; field < rec->num_fru_fields; ++field)
        {
            if (p + sizeof(pldm_fru_record_tlv) - 1 > end)
            {
                return std::nullopt;
            }

            const auto* tlv = reinterpret_cast<const pldm_fru_record_tlv*>(p);
            // stride = fixed TLV header (minus flex array byte) + value bytes
            const auto stride = sizeof(pldm_fru_record_tlv) - 1 + tlv->length;
            if (p + stride > end)
            {
                return std::nullopt;
            }

            if (rec->record_type == PLDM_FRU_RECORD_TYPE_GENERAL &&
                tlv->type == PLDM_FRU_FIELD_TYPE_SN)
            {
                return std::string(reinterpret_cast<const char*>(tlv->value),
                                   tlv->length);
            }

            p += stride;
        }
    }

    return std::nullopt;
}

std::optional<std::string> PldmFetcher::readPldmSerialNumber(uint16_t rsi) const
{
    const auto instanceId = allocateInstanceId("readPldmSerialNumber");
    if (!instanceId)
    {
        return std::nullopt;
    }

    const auto requestMsg = encodeGetFruRecordByOptionRequest(*instanceId, rsi);
    if (!requestMsg)
    {
        return std::nullopt;
    }

    void* responseMsg = nullptr;
    size_t responseSize{};
    const auto rc = pldm_transport_send_recv_msg(
        transport, tid, requestMsg->data(), requestMsg->size(), &responseMsg,
        &responseSize);
    pldm_instance_id_free(instanceDb, tid, *instanceId);

    if (rc != PLDM_REQUESTER_SUCCESS)
    {
        if (verbose)
        {
            std::println(stderr,
                         "{}readPldmSerialNumber: MCTP send/recv failed "
                         "for rsi={}: rc={}",
                         largeIndent, rsi, static_cast<int>(rc));
        }
        return std::nullopt;
    }

    const auto fruBytes =
        decodeGetFruRecordByOptionResponse(rsi, responseMsg, responseSize);
    if (!fruBytes)
    {
        return std::nullopt;
    }

    return findFruSerialNumber(*fruBytes);
}

auto PldmFetcher::readInventorySerialNumber(int chassisNumber) const
    -> std::optional<std::string>
{
    try
    {
        auto method = bus.new_method_call(
            "xyz.openbmc_project.Inventory.Manager",
            std::format("/xyz/openbmc_project/inventory/system/chassis{}",
                        chassisNumber)
                .c_str(),
            "org.freedesktop.DBus.Properties", "Get");
        method.append("xyz.openbmc_project.Inventory.Decorator.Asset",
                      "SerialNumber");
        auto reply = bus.call(method);
        std::variant<std::string> val;
        reply.read(val);
        return std::get<std::string>(val);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

auto PldmFetcher::fetchSensorPDRs(sdbusplus::bus_t&, uint16_t stateSetId,
                                  uint16_t entityType) const
    -> std::vector<SensorPDREntry>
{
    std::vector<SensorPDREntry> results;
    try
    {
        auto method = bus.new_method_call(
            pldmService, "/xyz/openbmc_project/pldm",
            "xyz.openbmc_project.PLDM.PDR", "FindStateSensorPDR");
        method.append(static_cast<uint8_t>(0),
                      static_cast<uint16_t>(entityType),
                      static_cast<uint16_t>(stateSetId));
        auto reply = bus.call(method);

        std::vector<std::vector<uint8_t>> pdrs;
        reply.read(pdrs);

        for (const auto& pdr : pdrs)
        {
            // Minimum size: fixed fields of pldm_state_sensor_pdr excluding
            // the variable-length possible_states tail
            if (pdr.size() < sizeof(pldm_state_sensor_pdr) - sizeof(uint8_t))
            {
                continue;
            }
            const auto* p =
                reinterpret_cast<const pldm_state_sensor_pdr*>(pdr.data());
            results.push_back({p->sensor_id, p->entity_instance});
        }
    }
    catch (const std::exception& e)
    {
        if (verbose)
        {
            std::println(stderr,
                         "{}FindStateSensorPDR(stateSetId={}, entityType={:#x})"
                         " failed: {}",
                         largeIndent, stateSetId, entityType, e.what());
        }
    }
    return results;
}

auto PldmFetcher::findSensorEntry(const std::vector<SensorPDREntry>& pdrs,
                                  uint16_t entityInstance) const
    -> std::optional<SensorPDREntry>
{
    for (const auto& entry : pdrs)
    {
        if (entry.entityInstance == entityInstance)
        {
            return entry;
        }
    }
    return std::nullopt;
}

std::optional<uint8_t> PldmFetcher::decodeStateSensorResponse(
    void* responseMsg, size_t payloadLen) const
{
    // An error response contains only the completion code (1 byte payload).
    if (payloadLen == 1)
    {
        return std::nullopt;
    }

    uint8_t completionCode{};
    uint8_t compCount{};
    std::array<get_sensor_state_field, 8> stateField{};
    auto rc = decode_get_state_sensor_readings_resp(
        static_cast<pldm_msg*>(responseMsg), payloadLen, &completionCode,
        &compCount, stateField.data());

    if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS || compCount == 0)
    {
        return std::nullopt;
    }

    return stateField[0].present_state;
}

std::optional<uint8_t> PldmFetcher::readStateSensor(uint16_t sensorId) const
{
    const auto instanceId = allocateInstanceId("readStateSensor");
    if (!instanceId)
    {
        return std::nullopt;
    }

    auto requestMsg =
        encodeGetStateSensorReadingsRequest(*instanceId, sensorId);
    if (!requestMsg)
    {
        return std::nullopt;
    }

    void* responseMsg = nullptr;
    size_t responseSize{};
    // Send the encoded request, receive the response buffer, then release
    // the reserved instance ID.
    auto rc = pldm_transport_send_recv_msg(
        transport, tid, requestMsg->data(), requestMsg->size(), &responseMsg,
        &responseSize);
    pldm_instance_id_free(instanceDb, tid, *instanceId);

    if (rc != PLDM_REQUESTER_SUCCESS)
    {
        if (verbose)
        {
            std::println(stderr,
                         "{}GetStateSensorReadings MCTP send/recv failed "
                         "for sensorID={}: rc={}",
                         largeIndent, sensorId, static_cast<int>(rc));
        }
        return std::nullopt;
    }

    auto state = decodeStateSensorResponse(responseMsg,
                                           responseSize - sizeof(pldm_msg_hdr));
    if (!state && verbose)
    {
        std::println(stderr,
                     "{}GetStateSensorReadings decode failed for sensorID={}",
                     largeIndent, sensorId);
    }
    free(responseMsg);
    return state;
}

void PldmFetcher::printSensorState(
    const std::optional<SensorPDREntry>& entry,
    const std::map<uint8_t, std::string_view>& stateNames) const
{
    if (!entry)
    {
        std::println("Unknown (no PDR found)");
        return;
    }

    auto state = readStateSensor(entry->sensorId);
    if (!state)
    {
        std::println("Unknown (MCTP read failed)");
        return;
    }

    auto found = stateNames.find(*state);
    std::println("{}", found != stateNames.end() ? found->second
                                                 : std::string_view{"Unknown"});

    if (verbose)
    {
        std::println("{}sensorID: {}  TID: {}", largeIndent, entry->sensorId,
                     static_cast<unsigned>(tid));
    }
}

void PldmFetcher::display(int chassisNumber) const
{
    std::println("PLDM");

    // Use the entity instance derived from FRU serial matching, or UINIT16_MAx
    // to "throw away" the pldm data that does not map to a chassis
    const uint16_t entityInstance =
        chassisToEntityInstance.contains(chassisNumber)
            ? chassisToEntityInstance.at(chassisNumber)
            : UINT16_MAX;

    std::print("{}Availability: ", smallIndent);
    printSensorState(
        findSensorEntry(pdrSets.availability.forChassis(chassisNumber),
                        entityInstance),
        availabilityStateNames);

    std::print("{}Present: ", smallIndent);
    printSensorState(findSensorEntry(pdrSets.presence.forChassis(chassisNumber),
                                     entityInstance),
                     presenceStateNames);

    std::print("{}Power State: ", smallIndent);
    printSensorState(
        findSensorEntry(pdrSets.powerState.forChassis(chassisNumber),
                        entityInstance),
        systemPowerStateNames);

    std::print("{}Operational Fault Status: ", smallIndent);
    printSensorState(
        findSensorEntry(pdrSets.operationalFault.forChassis(chassisNumber),
                        entityInstance),
        operationalFaultStateNames);

    if (!verbose)
    {
        return;
    }

    auto invOpt = readInventorySerialNumber(chassisNumber);
    if (invOpt && invOpt->empty())
    {
        invOpt = std::nullopt;
    }

    if (fruSerialCache.empty() && !invOpt)
    {
        return;
    }

    const auto& invSerial = invOpt.value_or("Unknown");

    std::string pldmSerial = "Unknown";
    for (const auto& [cachedInstance, cachedSerial] : fruSerialCache)
    {
        if (cachedSerial == invSerial)
        {
            pldmSerial = cachedSerial;
            break;
        }
    }

    std::println("{}FRU Serial (PLDM):      {}", largeIndent, pldmSerial);
    std::println("{}FRU Serial (Inventory): {}", largeIndent, invSerial);
}
