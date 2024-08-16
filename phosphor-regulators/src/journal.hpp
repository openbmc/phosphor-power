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

#include <systemd/sd-journal.h>

#include <phosphor-logging/log.hpp>

#include <string>
#include <vector>

namespace phosphor::power::regulators
{

/**
 * @class Journal
 *
 * Abstract base class that provides a journal interface.
 *
 * The interface is used to write messages/log entries to the system journal.
 */
class Journal
{
  public:
    // Specify which compiler-generated methods we want
    Journal() = default;
    Journal(const Journal&) = delete;
    Journal(Journal&&) = delete;
    Journal& operator=(const Journal&) = delete;
    Journal& operator=(Journal&&) = delete;
    virtual ~Journal() = default;

    /**
     * Gets the journal messages that have the specified field set to the
     * specified value.
     *
     * The messages in the returned vector are ordered from oldest to newest.
     *
     * @param field journal field name
     * @param fieldValue expected field value
     * @param max Maximum number of messages to return.  Specify 0 to return all
     *            matching messages.
     * @return matching messages from the journal
     */
    virtual std::vector<std::string>
        getMessages(const std::string& field, const std::string& fieldValue,
                    unsigned int max = 0) = 0;

    /**
     * Logs a debug message in the system journal.
     *
     * @param message message to log
     */
    virtual void logDebug(const std::string& message) = 0;

    /**
     * Logs debug messages in the system journal.
     *
     * @param messages messages to log
     */
    virtual void logDebug(const std::vector<std::string>& messages) = 0;

    /**
     * Logs an error message in the system journal.
     *
     * @param message message to log
     */
    virtual void logError(const std::string& message) = 0;

    /**
     * Logs error messages in the system journal.
     *
     * @param messages messages to log
     */
    virtual void logError(const std::vector<std::string>& messages) = 0;

    /**
     * Logs an informational message in the system journal.
     *
     * @param message message to log
     */
    virtual void logInfo(const std::string& message) = 0;

    /**
     * Logs informational messages in the system journal.
     *
     * @param messages messages to log
     */
    virtual void logInfo(const std::vector<std::string>& messages) = 0;
};

/**
 * @class SystemdJournal
 *
 * Implementation of the Journal interface that writes to the systemd journal.
 */
class SystemdJournal : public Journal
{
  public:
    // Specify which compiler-generated methods we want
    SystemdJournal() = default;
    SystemdJournal(const SystemdJournal&) = delete;
    SystemdJournal(SystemdJournal&&) = delete;
    SystemdJournal& operator=(const SystemdJournal&) = delete;
    SystemdJournal& operator=(SystemdJournal&&) = delete;
    virtual ~SystemdJournal() = default;

    /** @copydoc Journal::getMessages() */
    virtual std::vector<std::string>
        getMessages(const std::string& field, const std::string& fieldValue,
                    unsigned int max) override;

    /** @copydoc Journal::logDebug(const std::string&) */
    virtual void logDebug(const std::string& message) override
    {
        using namespace phosphor::logging;
        log<level::DEBUG>(message.c_str());
    }

    /** @copydoc Journal::logDebug(const std::vector<std::string>&) */
    virtual void logDebug(const std::vector<std::string>& messages) override
    {
        for (const std::string& message : messages)
        {
            logDebug(message);
        }
    }

    /** @copydoc Journal::logError(const std::string&) */
    virtual void logError(const std::string& message) override
    {
        using namespace phosphor::logging;
        log<level::ERR>(message.c_str());
    }

    /** @copydoc Journal::logError(const std::vector<std::string>&) */
    virtual void logError(const std::vector<std::string>& messages) override
    {
        for (const std::string& message : messages)
        {
            logError(message);
        }
    }

    /** @copydoc Journal::logInfo(const std::string&) */
    virtual void logInfo(const std::string& message) override
    {
        using namespace phosphor::logging;
        log<level::INFO>(message.c_str());
    }

    /** @copydoc Journal::logInfo(const std::vector<std::string>&) */
    virtual void logInfo(const std::vector<std::string>& messages) override
    {
        for (const std::string& message : messages)
        {
            logInfo(message);
        }
    }

  private:
    /**
     * Gets the value of the specified field for the current journal entry.
     *
     * Returns an empty string if the current journal entry does not have the
     * specified field.
     *
     * @param journal current journal entry
     * @param field journal field name
     * @return field value
     */
    std::string getFieldValue(sd_journal* journal, const std::string& field);

    /**
     * Gets the realtime (wallclock) timestamp for the current journal entry.
     *
     * @param journal current journal entry
     * @return timestamp as a date/time string
     */
    std::string getTimeStamp(sd_journal* journal);
};

} // namespace phosphor::power::regulators
