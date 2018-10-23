#pragma once

#include <boost/thread/future.hpp>
#include <atomic>

namespace nurl
{
    namespace interact
    {
        namespace details
        {
            // CLION: Clang-Tidy: Initialization of 'prom' with static storage duration may throw an exception that cannot be caught
            // boost::promise<int> prom;
            // Solution:
            boost::promise<int>& get_prom()
            {
                static boost::promise<int> prom;
                return prom;
            }

            std::atomic<bool> cons_exit{false};

            // CLION: Clang-Tidy: Initialization of 'ioservice_post_mutex' with static storage duration may throw an exception that cannot be caught
            // boost::mutex ioservice_post_mutex;
            // Solution:
            boost::mutex& get_ioservice_post_mutex()
            {
                static boost::mutex ioservice_post_mutex;
                return ioservice_post_mutex;
            }
            extern std::atomic<int> run_tasks;
            using platform_guard_lock = boost::lock_guard<boost::mutex>;
        }
    }
}
