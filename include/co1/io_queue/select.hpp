
#pragma once

#include <co1/common.hpp>

#include <sys/select.h>

#include <algorithm>
#include <coroutine>
#include <vector>

namespace co1::io_queue
{

class select
{
public:
    select() { m_ops.reserve(16); }

    void add(const io_op& op, std::coroutine_handle<> coro) { m_ops.emplace_back(op, coro); }

    bool empty() const noexcept { return m_ops.empty(); }

    template <typename Rep, typename Period>
    std::error_code poll(detail::ready_sink_t& ready, std::chrono::duration<Rep, Period> timeout)
    {
        using namespace std::chrono;

        TRACE("Monitoring " << m_ops.size() << " file descriptors, timeout: "
                            << duration_cast<milliseconds>(timeout).count() << " ms");
        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        int max_fd = -1;
        for (const auto& [ op, coro ] : m_ops)
        {
            if (op.fd >= 0)
            {
                if (op.type == io_type::read)
                    FD_SET(op.fd, &read_fds);
                else if (op.type == io_type::write)
                    FD_SET(op.fd, &write_fds);
                else
                    return std::make_error_code(std::errc::invalid_argument);

                max_fd = std::max(max_fd, op.fd);
            }
        }

        timeval tv;
        auto sec = duration_cast<seconds>(timeout);
        auto microsec = duration_cast<microseconds>(timeout - sec);
        tv.tv_sec = sec.count();
        tv.tv_usec = microsec.count();

        int n = ::select(max_fd + 1, &read_fds, &write_fds, nullptr, &tv);
        TRACE("select() returned: " << n);
        if (n < 0)
        {
            auto ec = std::error_code { errno, std::generic_category() };
            TRACE("select() failed with error: " << ec.message());
            throw std::system_error(ec);
        }


        auto it = std::remove_if(m_ops.begin(), m_ops.end(), [&read_fds, &write_fds, &ready](const auto& e)
        {
            bool triggered = (e.first.type == io_type::read && FD_ISSET(e.first.fd, &read_fds)) ||
                             (e.first.type == io_type::write && FD_ISSET(e.first.fd, &write_fds));
            if (triggered)
                ready.push(e.second);
            return triggered;
        });
        m_ops.erase(it, m_ops.end());

        return std::error_code{};
    }

private:
    std::vector<std::pair<io_op, std::coroutine_handle<>>> m_ops;
};

} // namespace co1::backend::select
