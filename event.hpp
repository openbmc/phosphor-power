#pragma once

#include <memory>
#include <systemd/sd-event.h>

namespace witherspoon
{
namespace power
{
namespace event
{

/**
 * Custom deleter for sd_event_source
 */
struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        sd_event_source_unref(eventSource);
    }
};

using EventSource = std::unique_ptr<sd_event_source, EventSourceDeleter>;

/**
 * Customer deleter for sd_event
 */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        sd_event_unref(event);
    }
};

using Event = std::unique_ptr<sd_event, EventDeleter>;

}
}
}
