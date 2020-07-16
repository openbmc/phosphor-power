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

#include "error_logging.hpp"
#include "journal.hpp"
#include "mock_error_logging.hpp"
#include "mock_journal.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor::power::regulators
{

/**
 * @class MockServices
 *
 * Implementation of the Services interface using mock system services.
 */
class MockServices : public Services
{
  public:
    // Specify which compiler-generated methods we want
    MockServices() = default;
    MockServices(const MockServices&) = delete;
    MockServices(MockServices&&) = delete;
    MockServices& operator=(const MockServices&) = delete;
    MockServices& operator=(MockServices&&) = delete;
    virtual ~MockServices() = default;

    /** @copydoc Services::getBus() */
    virtual sdbusplus::bus::bus& getBus() override
    {
        return bus;
    }

    /** @copydoc Services::getErrorLogging() */
    virtual ErrorLogging& getErrorLogging() override
    {
        return errorLogging;
    }

    /** @copydoc Services::getJournal() */
    virtual Journal& getJournal() override
    {
        return journal;
    }

    /**
     * Returns the MockErrorLogging object that implements the ErrorLogging
     * interface.
     *
     * This allows test cases to use the object in EXPECT_CALL() statements.
     *
     * @return mock error logging object
     */
    virtual MockErrorLogging& getMockErrorLogging() override
    {
        return errorLogging;
    }

    /**
     * Returns the MockJournal object that implements the Journal interface.
     *
     * This allows test cases to use the object in EXPECT_CALL() statements.
     *
     * @return mock journal object
     */
    virtual MockJournal& getMockJournal() override
    {
        return journal;
    }

  private:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus::bus bus{sdbusplus::bus::new_default()};

    /**
     * Mock implementation of the ErrorLogging interface.
     */
    MockErrorLogging errorLogging{};

    /**
     * Mock implementation of the Journal interface.
     */
    MockJournal journal{};
};

} // namespace phosphor::power::regulators
