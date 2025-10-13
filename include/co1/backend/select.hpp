
#pragma once

#include <co1/backend.hpp>
#include <co1/utils.hpp>

#include <sys/select.h>

#include <algorithm>
#include <optional>
#include <vector>

namespace co1::backend
{

class select
{
public:
    select()
    {
        m_events.reserve(16);
        m_ready_events.reserve(16);
    }

    std::error_code register_event(const event& e)
    {
        m_events.push_back(e);
        return std::error_code{};
    }

    size_t unregister_event(const event& e)
    {
        auto it = std::remove(m_events.begin(), m_events.end(), e);
        size_t count = std::distance(it, m_events.end());
        m_events.erase(it, m_events.end());
        return count;
    }

    std::optional<event> consume_event()
    {
        if (m_ready_events.empty())
            return std::nullopt;
        auto e = m_ready_events.back();
        m_ready_events.pop_back();
        return e;
    }

    template <typename Rep, typename Period>
    std::error_code poll(std::chrono::duration<Rep, Period> timeout)
    {
        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        int max_fd = -1;
        for (const auto& it : m_events)
        {
            if (it.fd >= 0)
            {
                if (it.type == event_type::fd_read)
                    FD_SET(it.fd, &read_fds);
                else if (it.type == event_type::fd_write)
                    FD_SET(it.fd, &write_fds);
                else
                    return std::make_error_code(std::errc::invalid_argument);

                if (it.fd > max_fd)
                    max_fd = it.fd;
            }
        }

        timeval tv;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout - seconds);
        tv.tv_sec = seconds.count();
        tv.tv_usec = microseconds.count();

        int n = ::select(max_fd + 1, &read_fds, &write_fds, nullptr, &tv);
        if (n < 0)
        {
            TRACE("select() error: " << n);
            return std::error_code { errno, std::generic_category() };
        }
        TRACE("select() returned: " << n);
        m_ready_events.reserve(m_ready_events.size() + n);
        std::remove_if(m_events.begin(), m_events.end(), [this, &read_fds, &write_fds](const event& e)
        {
            if ((e.type == event_type::fd_read && FD_ISSET(e.fd, &read_fds)) ||
                (e.type == event_type::fd_write && FD_ISSET(e.fd, &write_fds)))
            {
                m_ready_events.push_back(e);
                return true;
            }
            return false;
        });

        return std::error_code{};
    }

private:
    std::vector<event> m_events;
    std::vector<event> m_ready_events;
};

} // namespace co1::backend::select
