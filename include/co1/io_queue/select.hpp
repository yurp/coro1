
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
    static constexpr size_t RESERVED_IOPS_COUNT = 16;

    select() { m_iows.reserve(RESERVED_IOPS_COUNT); }

    void add(const io_wait& iow, std::coroutine_handle<> coro) { m_iows.emplace_back(iow, coro); }

    [[nodiscard]] bool empty() const noexcept { return m_iows.empty(); }
    template <typename Rep, typename Period>
    std::error_code poll(detail::ready_sink_t& ready, std::chrono::duration<Rep, Period> timeout)
    {
        using namespace std::chrono;

        TRACE("Monitoring " << m_iows.size() << " file descriptors, timeout: "
                            << duration_cast<milliseconds>(timeout).count() << " ms");
        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        int max_fd = -1;
        for (const auto& [ iow, coro ] : m_iows)
        {
            if (iow.fd >= 0)
            {
                if (iow.type == io_type::read)
                {
                    FD_SET(iow.fd, &read_fds);
                }
                else if (iow.type == io_type::write)
                {
                    FD_SET(iow.fd, &write_fds);
                }
                else
                {
                    return std::make_error_code(std::errc::invalid_argument);
                }
                max_fd = std::max(max_fd, iow.fd);
            }
        }

        timeval tmval {};
        auto sec = duration_cast<seconds>(timeout);
        auto microsec = duration_cast<microseconds>(timeout - sec);
        tmval.tv_sec = sec.count();
        tmval.tv_usec = microsec.count();
        int sel_cnt = ::select(max_fd + 1, &read_fds, &write_fds, nullptr, &tmval);
        TRACE("select() returned: " << sel_cnt);
        if (sel_cnt < 0)
        {
            auto ecode = std::error_code { errno, std::generic_category() };
            TRACE("select() failed with error: " << ecode.message());
            throw std::system_error(ecode);
        }


        auto iter = std::remove_if(m_iows.begin(), m_iows.end(),
            [&read_fds, &write_fds, &ready](const auto& iow)
            {
                bool triggered = (iow.first.type == io_type::read &&
                                  FD_ISSET(iow.first.fd, &read_fds)) ||
                                 (iow.first.type == io_type::write &&
                                  FD_ISSET(iow.first.fd, &write_fds));
                if (triggered)
                {
                    ready.push(iow.second);
                }
                return triggered;
            });
        m_iows.erase(iter, m_iows.end());
        return std::error_code{};
    }

private:
    std::vector<std::pair<io_wait, std::coroutine_handle<>>> m_iows;
};

} // namespace co1::io_queue