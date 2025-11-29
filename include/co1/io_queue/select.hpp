
#pragma once

#include <co1/common.hpp>

#include <fcntl.h>
#include <sys/select.h>

#include <algorithm>
#include <coroutine>
#include <vector>

namespace co1::io_queue
{

class select
{
public:
    struct entry
    {
        io_wait m_io_wait;
        std::error_code* m_error_code;
        std::coroutine_handle<> m_coro;
    };

    static constexpr size_t RESERVED_IOPS_COUNT = 16;

    select() { m_iows.reserve(RESERVED_IOPS_COUNT); }

    void add(const io_wait& iow, std::error_code& error_code, std::coroutine_handle<> coro)
    {
        m_iows.emplace_back(iow, &error_code, coro);
    }

    [[nodiscard]] bool empty() const noexcept { return m_iows.empty(); }

    template <typename Rep, typename Period>
    std::error_code poll(detail::ready_sink_t& ready, std::chrono::duration<Rep, Period> timeout)
    {
        using namespace std::chrono;

        TRACE("Monitoring " << m_iows.size() << " file descriptors, timeout: "
                            << duration_cast<milliseconds>(timeout).count() << " ms");
        while (true)
        {
            bool bad_fd_detected = false;
            auto max_fd = build_fd_set(ready, bad_fd_detected);
            if (bad_fd_detected)
            {
                TRACE("Detected bad file descriptors during fd_set build");
                timeout = duration<Rep, Period>::zero();
            }

            timeval tmval {};
            auto sec = duration_cast<seconds>(timeout);
            auto microsec = duration_cast<microseconds>(timeout - sec);
            tmval.tv_sec = sec.count();
            tmval.tv_usec = microsec.count();

            int sel_cnt = ::select(max_fd + 1, &m_read_fds, &m_write_fds, nullptr, &tmval);
            std::error_code select_error;
            if (sel_cnt < 0)
            {
                select_error = std::error_code { errno, std::generic_category() };
                TRACE("select() returned: " << select_error.message());
            }
            if (!select_error)
            {
                process_success_results(ready);
                return select_error;
            }

            if (select_error == std::errc::bad_file_descriptor)
            {
                process_bad_fd_results(ready);
                timeout = duration<Rep, Period>::zero();
            }
            else if (select_error == std::errc::interrupted ||
                     select_error == std::errc::resource_unavailable_try_again)
            {
                TRACE("select() interrupted, retrying...");
            }
            else
            {
                return select_error;
            }
        }
    }

    int build_fd_set(detail::ready_sink_t& ready, bool& bad_fd_detected)
    {
        int max_fd = -1;

        FD_ZERO(&m_read_fds);
        FD_ZERO(&m_write_fds);
        auto iter = std::remove_if(m_iows.begin(), m_iows.end(), [this, &ready, &max_fd](const auto& entry)
        {
            if (entry.m_io_wait.fd < 0)
            {
                *entry.m_error_code = std::make_error_code(std::errc::invalid_argument);
                ready.push(entry.m_coro);
                return true;
            }
            if (entry.m_io_wait.type == io_type::read)
            {
                FD_SET(entry.m_io_wait.fd, &m_read_fds);
                max_fd = std::max(max_fd, entry.m_io_wait.fd);
                return false;
            }
            if (entry.m_io_wait.type == io_type::write)
            {
                FD_SET(entry.m_io_wait.fd, &m_write_fds);
                max_fd = std::max(max_fd, entry.m_io_wait.fd);
                return false;
            }
            *entry.m_error_code = std::make_error_code(std::errc::invalid_argument);
            ready.push(entry.m_coro);
            return true;
        });

        bad_fd_detected = (iter != m_iows.end());
        m_iows.erase(iter, m_iows.end());
        return max_fd;
    }

    void process_success_results(detail::ready_sink_t& ready)
    {
        auto iter = std::remove_if(m_iows.begin(), m_iows.end(), [this, &ready](const auto& entry)
        {
            bool triggered = (entry.m_io_wait.type == io_type::read &&
                              FD_ISSET(entry.m_io_wait.fd, &m_read_fds)) ||
                             (entry.m_io_wait.type == io_type::write &&
                              FD_ISSET(entry.m_io_wait.fd, &m_write_fds));
            if (triggered)
            {
                *entry.m_error_code = std::error_code{};
                ready.push(entry.m_coro);
            }

            return triggered;
        });

        m_iows.erase(iter, m_iows.end());
    }

    void process_bad_fd_results(detail::ready_sink_t& ready)
    {
        auto iter = std::remove_if(m_iows.begin(), m_iows.end(), [&ready](const auto& entry)
        {
            if (::fcntl(entry.m_io_wait.fd, F_GETFD) == -1 && errno == EBADF)
            {
                TRACE("Invalid file descriptor detected: " << entry.m_io_wait.fd);
                *entry.m_error_code = std::error_code { EBADF, std::generic_category() };
                ready.push(entry.m_coro);
                return true;
            }
            return false;
        });

        m_iows.erase(iter, m_iows.end());
    }

private:
    std::vector<entry> m_iows;
    fd_set m_read_fds {};
    fd_set m_write_fds {};
};

} // namespace co1::io_queue