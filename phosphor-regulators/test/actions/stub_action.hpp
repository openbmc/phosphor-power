/**
 * Copyright © 2019 IBM Corporation
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

#include <cstddef> // for size_t
#include <exception>
#include <memory>
#include <utility>

namespace phosphor
{
namespace power
{
namespace regulators
{

/**
 * @class StubAction
 *
 * Stub action used for unit tests.
 *
 * The action allows you to do one of the following when execute() is called:
 *   - return a predefined return value
 *   - throw an exception
 *
 * The action also tracks the following:
 *   - number of times action has been executed
 *   - number of exceptions thrown
 */
class StubAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    StubAction() = delete;
    StubAction(const StubAction&) = delete;
    StubAction(StubAction&&) = delete;
    StubAction& operator=(const StubAction&) = delete;
    StubAction& operator=(StubAction&&) = delete;
    virtual ~StubAction() = default;

    /**
     * Constructor.
     *
     * @param returnValue value to return when execute() is called
     */
    explicit StubAction(bool returnValue) : returnValue{returnValue}
    {
    }

    /**
     * Executes this action.
     *
     * Throws an exception if setException() was called.  Otherwise returns the
     * return value specified in the constructor or with setReturnValue().
     *
     * @param environment action execution environment
     * @return value of returnValue data member
     */
    virtual bool execute(ActionEnvironment& environment) override
    {
        ++executeCount;

        if (except)
        {
            ++exceptionCount;
            throw *except;
        }

        return returnValue;
    }

    /**
     * Returns the number of exceptions that have been thrown when execute() was
     * called.
     *
     * @return number of exceptions thrown
     */
    size_t getExceptionCount() const
    {
        return exceptionCount;
    }

    /**
     * Returns the number of times execute() has been called.
     *
     * @return number of times execute() called
     */
    size_t getExecuteCount() const
    {
        return executeCount;
    }

    /**
     * Sets the value that will be returned when execute() is called.
     *
     * @param returnValue value to return from execute()
     */
    void setReturnValue(bool returnValue)
    {
        this->returnValue = returnValue;
    }

    /**
     * Sets the exception that will be thrown when execute() is called.
     *
     * @param except exception to throw, or nullptr to throw no exception
     */
    void setException(std::unique_ptr<std::exception> except)
    {
        this->except = std::move(except);
    }

  private:
    /**
     * Value that will be returned when execute() is called.
     */
    bool returnValue{true};

    /**
     * Exception to throw (if any) when execute() is called.
     */
    std::unique_ptr<std::exception> except{nullptr};

    /**
     * Number of times execute() has been called on this action.
     */
    size_t executeCount{0};

    /**
     * Number of times an exception was thrown when execute() was called.
     */
    size_t exceptionCount{0};
};

} // namespace regulators
} // namespace power
} // namespace phosphor
