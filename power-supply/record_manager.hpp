#pragma once

#include <deque>
#include <tuple>
#include <vector>

namespace witherspoon
{
namespace power
{
namespace history
{

static constexpr auto recIDPos = 0;
static constexpr auto recTimePos = 1;
static constexpr auto recAvgPos = 2;
static constexpr auto recMaxPos = 3;
using Record = std::tuple<size_t, int64_t, int64_t, int64_t>;

/**
 * @class RecordManager
 *
 * This class manages the records for the input power history of
 * a power supply.
 *
 * The history is the average and maximum power values across 30s
 * intervals.  Every 30s, a new record will be available from the
 * PS.  This class takes that raw PS data and converts it into
 * something useable by D-Bus.  It ensures the readings are always
 * sorted newest to oldest, and prunes out the oldest entries when
 * necessary.  If there is a problem with the ordering IDs coming
 * from the PS, it will clear out the old records and start over.
 */
class RecordManager
{
    public:

        static constexpr auto LAST_SEQUENCE_ID = 0xFF;

        using DBusRecord = std::tuple<uint64_t, int64_t>;
        using DBusRecordList = std::vector<DBusRecord>;

        RecordManager() = delete;
        ~RecordManager() = default;
        RecordManager(const RecordManager&) = default;
        RecordManager& operator=(const RecordManager&) = default;
        RecordManager(RecordManager&&) = default;
        RecordManager& operator=(RecordManager&&) = default;

        /**
         * @brief Constructor
         *
         * @param[in] maxRec - the maximum number of history
         *                     records to keep at a time
         */
        RecordManager(size_t maxRec) :
                RecordManager(maxRec, LAST_SEQUENCE_ID)
        {
        }

        /**
         * @brief Constructor
         *
         * @param[in] maxRec - the maximum number of history
         *                     records to keep at a time
         * @param[in] lastSequenceID - the last sequence ID the power supply
         *                             will use before starting over
         */
        RecordManager(size_t maxRec, size_t lastSequenceID) :
                maxRecords(maxRec),
                lastSequenceID(lastSequenceID)
        {
        }

        /**
         * @brief Converts a Linear Format power number to an integer
         *
         * The PMBus spec describes a 2 byte Linear Format
         * number that is composed of an exponent and mantissa
         * in two's complement notation.
         *
         * Value = Mantissa * 2**Exponent
         *
         * @return int64_t the converted value
         */
        static int64_t linearToInteger(uint16_t data);

        /**
         * @brief Returns the number of records
         *
         * @return size_t - the number of records
         *
         */
        inline size_t getNumRecords() const
        {
            return records.size();
        }

        /**
         * @brief Deletes all records
         */
        inline void clear()
        {
            records.clear();
        }

    private:

        /**
         * @brief The maximum number of entries to keep in the history.
         *
         * When a new record is added, the oldest one will be removed.
         */
        const size_t maxRecords;

        /**
         * @brief The last ID the power supply returns before rolling over
         *        back to the first ID of 0.
         */
        const size_t lastSequenceID;

        /**
         * @brief The list of timestamp/average/maximum records.
         *        Newer records are added to the front, and older ones
         *        removed from the back.
         */
        std::deque<Record> records;
};

}
}
}
