
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

    select() { m_iops.reserve(RESERVED_IOPS_COUNT); }

    void add(const io_op& iop, std::coroutine_handle<> coro) { m_iops.emplace_back(iop, coro); }

    [[nodiscard]] bool empty() const noexcept { return m_iops.empty(); }
    template <typename Rep, typename Period>
    std::error_code poll(detail::ready_sink_t& ready, std::chrono::duration<Rep, Period> timeout)
    {
        using namespace std::chrono;

        TRACE("Monitoring " << m_iops.size() << " file descriptors, timeout: "
                            << duration_cast<milliseconds>(timeout).count() << " ms");
        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        int max_fd = -1;
        for (const auto& [ op, coro ] : m_iops)
        {
            if (op.fd >= 0)
            {
                if (op.type == io_type::read)
                {
                    FD_SET(op.fd, &read_fds);
                }
                else if (op.type == io_type::write)
                {
                    FD_SET(op.fd, &write_fds);
                }
                else
                {
                    return std::make_error_code(std::errc::invalid_argument);
                }
                max_fd = std::max(max_fd, op.fd);
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


        auto iter = std::remove_if(m_iops.begin(), m_iops.end(),
            [&read_fds, &write_fds, &ready](const auto& iop)
            {
                bool triggered = (iop.first.type == io_type::read &&
                                  FD_ISSET(iop.first.fd, &read_fds)) ||
                                 (iop.first.type == io_type::write &&
                                  FD_ISSET(iop.first.fd, &write_fds));
                if (triggered)
                {
                    ready.push(iop.second);
                }
                return triggered;
            });
        m_iops.erase(iter, m_iops.end());

        return std::error_code{};
    }

private:
    std::vector<std::pair<io_op, std::coroutine_handle<>>> m_iops;
};

} // namespace co1::backend::select
