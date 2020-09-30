
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

#include <phosphor-logging/log.hpp>

#include <stdexcept>

using namespace phosphor::logging;

namespace phosphor::power::regulators
{

std::string getFieldValue(sd_journal* journal, const char* field)
{
    const char* data;
    const void* eq;
    size_t length;
    size_t prefix;
    int rc;
    std::string value;

    // Get field data
    rc = sd_journal_get_data(journal, field, (const void**)&data, &length);
    if (rc < 0)
    {
        throw std::runtime_error{std::string{"Failed to read field: "} +
                                 strerror(-rc)};
    }

    // Get field value
    eq = memchr(data, '=', length);
    if (eq)
    {
        prefix = (char*)eq - data + 1;
    }
    else
    {
        prefix = 0;
    }

    value = strndup(data + prefix, length - prefix);

    return value;
}

std::vector<std::string> getMessages(const std::string& field,
                                     const std::string& fieldValue,
                                     unsigned int max = 0)
{
    sd_journal* journal;
    std::vector<std::string> messages;
    int rc;
    rc = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    if (rc < 0)
    {
        throw std::runtime_error{std::string{"Failed to open journal: "} +
                                 strerror(-rc)};
    }

    SD_JOURNAL_FOREACH(journal)
    {
        std::string date;
        char time[80];
        std::string message;
        std::string pid;
        std::string syslog;
        std::time_t t;
        uint64_t usec;
        std::string value;

        // Get input field
        try
        {
            value = getFieldValue(journal, field.c_str());
        }
        catch (const std::runtime_error& r_error)
        {
            continue;
        }

        // Compare field value and read data
        if (value == fieldValue)
        {
            try
            {
                // Get SYSLOG_IDENTIFIER field
                syslog = getFieldValue(journal, "SYSLOG_IDENTIFIER");

                // Get _PID field
                pid = getFieldValue(journal, "_PID");

                // Get MESSAGE field
                message = getFieldValue(journal, "MESSAGE");

                // Get realtime
                rc = sd_journal_get_realtime_usec(journal, &usec);
                if (rc < 0)
                {
                    throw std::runtime_error{
                        std::string{"Failed to determine timestamp: "} +
                        strerror(-rc)};
                }

                // Set realtime microseconds to date format
                t = usec / 1000000;
                strftime(time, sizeof(time), "%b %d %H:%M:%S", std::gmtime(&t));
                date = time;
            }
            catch (const std::runtime_error& r_error)
            {
                continue;
            }

            // Store to messages
            value = date + " " + syslog + "[" + pid + "]: " + message + "\n";
            messages.push_back(value);
        }
    }
    sd_journal_close(journal);

    if (max != 0 && max < messages.size())
    {
        messages.erase(messages.begin(),
                       messages.begin() + messages.size() - max);
    }
    return messages;
}

} // namespace phosphor::power::regulators