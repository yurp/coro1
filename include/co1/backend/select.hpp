
#pragma once

#include <co1/common.hpp>

#include <sys/select.h>

#include <algorithm>
#include <coroutine>
#include <vector>

namespace co1::backend
{

class select
{
public:
    select()
    {
        m_ops.reserve(16);
    }

    void add(const io_op& op, std::coroutine_handle<> coro)
    {
        m_ops.emplace_back(op, coro);
    }

    template <typename Rep, typename Period>
    std::error_code poll(ready_sink& ready, std::chrono::duration<Rep, Period> timeout)
    {
        using namespace std::chrono;

        TRACE("Calling select() with timeout: " << duration_cast<milliseconds>(timeout).count() << " ms");
        TRACE("Monitoring " << m_ops.size() << " file descriptors");

        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        int max_fd = -1;
        for (const auto& [it, coro] : m_ops)
        {
            if (it.fd >= 0)
            {
                if (it.type == io_type::read)
                    FD_SET(it.fd, &read_fds);
                else if (it.type == io_type::write)
                    FD_SET(it.fd, &write_fds);
                else
                    return std::make_error_code(std::errc::invalid_argument);

                if (it.fd > max_fd)
                    max_fd = it.fd;
            }
        }

        timeval tv;
        auto sec = duration_cast<seconds>(timeout);
        auto microsec = duration_cast<microseconds>(timeout - sec);
        tv.tv_sec = sec.count();
        tv.tv_usec = microsec.count();

        int n = ::select(max_fd + 1, &read_fds, &write_fds, nullptr, &tv);
        if (n < 0)
        {
            TRACE("select() error: " << n);
            auto ec = std::error_code { errno, std::generic_category() };
            TRACE("select() failed with error: " << ec.message());
            throw std::system_error(ec);
            return ec;
        }

        TRACE("select() returned: " << n);
        auto it = std::remove_if(m_ops.begin(), m_ops.end(), [this, &read_fds, &write_fds, &ready](const auto& e)
        {
            if ((e.first.type == io_type::read && FD_ISSET(e.first.fd, &read_fds)) ||
                (e.first.type == io_type::write && FD_ISSET(e.first.fd, &write_fds)))
            {
                ready.push(e.second);
                return true;
            }
            return false;
        });
        m_ops.erase(it, m_ops.end());

        return std::error_code{};
    }

private:
    std::vector<std::pair<io_op, std::coroutine_handle<>>> m_ops;
};

} // namespace co1::backend::select
