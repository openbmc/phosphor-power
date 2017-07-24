/**
 * Copyright © 2017 IBM Corporation
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
#include <chrono>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <type_traits>
#include "timer.hpp"

namespace witherspoon
{
namespace power
{

using namespace phosphor::logging;
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                            Error::InternalFailure;

Timer::Timer(event::Event& events,
             std::function<void()> callbackFunc) :
    timeEvent(events),
    callback(callbackFunc),
    timeout(0)
{
    sd_event_source* source = nullptr;

    // Start with an infinite expiration time
    auto r = sd_event_add_time(timeEvent.get(),
                               &source,
                               CLOCK_MONOTONIC, // Time base
                               UINT64_MAX,      // Expire time - way long time
                               0,               // Use default event accuracy
                               timeoutHandler,  // Callback handler on timeout
                               this);           // User data
    if (r < 0)
    {
        log<level::ERR>("Timer::Timer failed call to sd_event_add_time",
                        entry("ERROR=%s", strerror(-r)));
        elog<InternalFailure>();
    }

    eventSource.reset(source);

    //Ensure timer isn't running
    setTimer(SD_EVENT_OFF);
}


Timer::~Timer()
{
    setTimer(SD_EVENT_OFF);
}


int Timer::timeoutHandler(sd_event_source* eventSource,
                          uint64_t usec, void* userData)
{
    auto timer = static_cast<Timer*>(userData);

    if (timer->type == TimerType::repeating)
    {
        //Set the next expiration time
        timer->setTimeout();
    }

    timer->callback();

    return 0;
}


std::chrono::microseconds Timer::getTime()
{
    using namespace std::chrono;
    auto now = steady_clock::now().time_since_epoch();
    return duration_cast<microseconds>(now);
}


void Timer::setTimer(int action)
{
    auto r = sd_event_source_set_enabled(eventSource.get(), action);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_source_set_enabled",
                        entry("ERROR=%s", strerror(-r)),
                        entry("ACTION=%d", action));
        elog<InternalFailure>();
    }
}


void Timer::stop()
{
    setTimer(SD_EVENT_OFF);
}


bool Timer::running()
{
    int status = 0;

    //returns SD_EVENT_OFF, SD_EVENT_ON, or SD_EVENT_ONESHOT
    auto r = sd_event_source_get_enabled(eventSource.get(), &status);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_source_get_enabled",
                        entry("ERROR=%s", strerror(-r)));
        elog<InternalFailure>();
    }

    return (status != SD_EVENT_OFF);
}


void Timer::setTimeout()
{
    //Get the current time and add the delta
    static_assert(std::is_same<decltype(getTime()),
            std::chrono::microseconds>::value,
            "Timer::getTime() is the wrong type");
    static_assert(std::is_same<decltype(timeout),
            std::chrono::microseconds>::value,
            "Timer::timeout is the wrong type");

    auto expireTime = getTime() + timeout;

    //Set the time
    auto r = sd_event_source_set_time(eventSource.get(), expireTime.count());
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_source_set_time",
                        entry("ERROR=%s", strerror(-r)));
        elog<InternalFailure>();
    }
}


void Timer::start(std::chrono::microseconds timeValue,
                  TimerType timerType)
{
    type = timerType;

    // Disable the timer
    setTimer(SD_EVENT_OFF);

    //Rearm the timer
    timeout = timeValue;
    setTimeout();

    setTimer((type == TimerType::oneshot) ? SD_EVENT_ONESHOT : SD_EVENT_ON);
}


}
}
