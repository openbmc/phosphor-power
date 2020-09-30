
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

#include <string.h>
#include <time.h>

#include <cstdint>
#include <ctime>
#include <stdexcept>
#include <vector>

using namespace phosphor::logging;

namespace phosphor::power::regulators
{

class JournalCloser
{
  public:
    JournalCloser() = delete;
    JournalCloser(const JournalCloser&) = delete;
    JournalCloser(JournalCloser&&) = delete;
    JournalCloser& operator=(const JournalCloser&) = delete;
    JournalCloser& operator=(JournalCloser&&) = delete;

    JournalCloser(sd_journal* journal) : journal{journal}
    {
    }
    ~JournalCloser()
    {
        sd_journal_close(journal);
    }

  private:
    sd_journal* journal{nullptr};
};

std::string SystemdJournal::getFieldValue(sd_journal* journal,
                                          const char* field) const
{
    const char* data{nullptr};
    size_t length{0};
    size_t prefix{0};

    // Get field data
    int rc = sd_journal_get_data(journal, field, (const void**)&data, &length);
    if (rc < 0)
    {
        throw std::runtime_error{
            std::string{"Failed to read journal entry field: "} +
            strerror(-rc)};
    }

    // Get field value
    const void* eq = memchr(data, '=', length);
    if (eq)
    {
        prefix = (const char*)eq - data + 1;
    }
    else
    {
        prefix = 0;
    }

    std::string value{data + prefix, length - prefix};

    return value;
}

std::vector<std::string>
    SystemdJournal::getMessages(const std::string& field,
                                const std::string& fieldValue, unsigned int max)
{
    sd_journal* journal;
    std::vector<std::string> messages;

    int rc = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    JournalCloser closer{journal};
    if (rc < 0)
    {
        throw std::runtime_error{std::string{"Failed to open journal: "} +
                                 strerror(-rc)};
    }

    SD_JOURNAL_FOREACH_BACKWARDS(journal)
    {
        char dateBuffer[80];
        uint64_t usec{0};

        // Get input field
        std::string value = getFieldValue(journal, field.c_str());

        // Compare field value and read data
        if (value == fieldValue)
        {
            // Get SYSLOG_IDENTIFIER field
            std::string syslog = getFieldValue(journal, "SYSLOG_IDENTIFIER");

            // Get _PID field
            std::string pid = getFieldValue(journal, "_PID");

            // Get MESSAGE field
            std::string message = getFieldValue(journal, "MESSAGE");

            // Get realtime
            rc = sd_journal_get_realtime_usec(journal, &usec);
            if (rc < 0)
            {
                throw std::runtime_error{
                    std::string{"Failed to get journal entry timestamp: "} +
                    strerror(-rc)};
            }

            // Convert realtime microseconds to date format
            std::time_t timeInSecs{0};
            std::string date;
            timeInSecs = usec / 1000000;
            strftime(dateBuffer, sizeof(dateBuffer), "%b %d %H:%M:%S",
                     std::localtime(&timeInSecs));
            date = dateBuffer;

            // Store value to messages
            value = date + " " + syslog + "[" + pid + "]: " + message;
            messages.insert(messages.begin(), value);
        }
        // Set the maximum number of messages
        if (max != 0 && messages.size() >= max)
        {
            break;
        }
    }

    return messages;
}

} // namespace phosphor::power::regulators