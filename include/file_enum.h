#pragma once

#include "nurl_interact.h"

namespace nurl
{
    template <typename HOST_PROCESSOR, typename FILE_PARAMS>
    class file_enumerator final
    {
        HOST_PROCESSOR& bp_;
        FILE_PARAMS& deq_elem;
        bool should_repost {true};

        void adjust_file_params_deque_cursor()
        {
            bp_.file_params_deque_cursor %= bp_.file_params_deque.size();
        }

        bool all_downloaded() const
        {
            for (const auto& el : bp_.file_params_deque)
            {
                if (!el.bad_stat)
                {
                    if ((el.filesize != el.download) || (el.filesize == 0))
                    {
                        return false;
                    }
                }
            }
            return true;
        }

    public:
        explicit file_enumerator(HOST_PROCESSOR& bp) : bp_(bp), deq_elem(bp.file_params_deque[bp.file_params_deque_cursor++]) {}

        FILE_PARAMS& get_deq_elem() const noexcept
        {
            return deq_elem;
        }

        void cancel_post() noexcept
        {
            should_repost = false;
            interact::details::run_tasks.fetch_sub(1, std::memory_order_relaxed);
        }

        ~file_enumerator() noexcept
        {
            adjust_file_params_deque_cursor();

            if (should_repost)
            {
                if (!all_downloaded())
                {
                    bp_.repost_processor_task();
                }
                else
                {
                    cancel_post(); // "COMPLETED !!"
                }
            }
        }

        file_enumerator(const file_enumerator&) = delete;
        file_enumerator& operator=(const file_enumerator&) = delete;
        file_enumerator(file_enumerator&&) = delete;
        file_enumerator& operator=(file_enumerator&&) = delete;

    };

} //namespace nurl

