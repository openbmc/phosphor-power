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

#include "journal.hpp"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <chrono>
#include <stdexcept>
#include <thread>

namespace phosphor::power::regulators
{

/**
 * @class JournalCloser
 *
 * Automatically closes the journal when the object goes out of scope.
 */
class JournalCloser
{
  public:
    // Specify which compiler-generated methods we want
    JournalCloser() = delete;
    JournalCloser(const JournalCloser&) = delete;
    JournalCloser(JournalCloser&&) = delete;
    JournalCloser& operator=(const JournalCloser&) = delete;
    JournalCloser& operator=(JournalCloser&&) = delete;

    explicit JournalCloser(sd_journal* journal) : journal{journal} {}

    ~JournalCloser()
    {
        sd_journal_close(journal);
    }

  private:
    sd_journal* journal{nullptr};
};

std::vector<std::string> SystemdJournal::getMessages(
    const std::string& field, const std::string& fieldValue, unsigned int max)
{
    // Sleep 100ms; otherwise recent journal entries sometimes not available
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);

    // Open the journal
    sd_journal* journal;
    int rc = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    if (rc < 0)
    {
        throw std::runtime_error{
            std::string{"Failed to open journal: "} + strerror(-rc)};
    }

    // Create object to automatically close journal
    JournalCloser closer{journal};

    // Add match so we only loop over entries with specified field value
    std::string match{field + '=' + fieldValue};
    rc = sd_journal_add_match(journal, match.c_str(), 0);
    if (rc < 0)
    {
        throw std::runtime_error{
            std::string{"Failed to add journal match: "} + strerror(-rc)};
    }

    // Loop through matching entries from newest to oldest
    std::vector<std::string> messages;
    messages.reserve((max != 0) ? max : 10);
    std::string syslogID, pid, message, timeStamp, line;
    SD_JOURNAL_FOREACH_BACKWARDS(journal)
    {
        // Get relevant journal entry fields
        timeStamp = getTimeStamp(journal);
        syslogID = getFieldValue(journal, "SYSLOG_IDENTIFIER");
        pid = getFieldValue(journal, "_PID");
        message = getFieldValue(journal, "MESSAGE");

        // Build one line string containing field values
        line = timeStamp + " " + syslogID + "[" + pid + "]: " + message;
        messages.emplace(messages.begin(), line);

        // Stop looping if a max was specified and we have reached it
        if ((max != 0) && (messages.size() >= max))
        {
            break;
        }
    }

    return messages;
}

std::string SystemdJournal::getFieldValue(sd_journal* journal,
                                          const std::string& field)
{
    std::string value{};

    // Get field data from current journal entry
    const void* data{nullptr};
    size_t length{0};
    int rc = sd_journal_get_data(journal, field.c_str(), &data, &length);
    if (rc < 0)
    {
        if (-rc == ENOENT)
        {
            // Current entry does not include this field; return empty value
            return value;
        }
        else
        {
            throw std::runtime_error{
                std::string{"Failed to read journal entry field: "} +
                strerror(-rc)};
        }
    }

    // Get value from field data.  Field data in format "FIELD=value".
    std::string dataString{static_cast<const char*>(data), length};
    std::string::size_type pos = dataString.find('=');
    if ((pos != std::string::npos) && ((pos + 1) < dataString.size()))
    {
        // Value is substring after the '='
        value = dataString.substr(pos + 1);
    }

    return value;
}

std::string SystemdJournal::getTimeStamp(sd_journal* journal)
{
    // Get realtime (wallclock) timestamp of current journal entry.  The
    // timestamp is in microseconds since the epoch.
    uint64_t usec{0};
    int rc = sd_journal_get_realtime_usec(journal, &usec);
    if (rc < 0)
    {
        throw std::runtime_error{
            std::string{"Failed to get journal entry timestamp: "} +
            strerror(-rc)};
    }

    // Convert to number of seconds since the epoch
    time_t secs = usec / 1000000;

    // Convert seconds to tm struct required by strftime()
    struct tm* timeStruct = localtime(&secs);
    if (timeStruct == nullptr)
    {
        throw std::runtime_error{
            std::string{"Invalid journal entry timestamp: "} + strerror(errno)};
    }

    // Convert tm struct into a date/time string
    char timeStamp[80];
    strftime(timeStamp, sizeof(timeStamp), "%b %d %H:%M:%S", timeStruct);

    return timeStamp;
}

} // namespace phosphor::power::regulators
