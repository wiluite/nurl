// Experiments with curl downloading of cataloged remote file set

#include "file_enum.h"
#include "host_proc.h"
#include "init_logs.h"
#include "log_wrapper.h"
#include <boost/thread/thread.hpp> // thread_group

#include <windef.h>
#define cons_handler static BOOL WINAPI console_handler(DWORD event)
#define cons_handler_setup SetConsoleCtrlHandler((PHANDLER_ROUTINE)console_handler, TRUE)

cons_handler
{
    nurl_logging::Log_Wrapper ("CTRL+C");
    nurl::interact::details::cons_exit = true;
    return true;
}

std::atomic<int> nurl::interact::details::run_tasks {0};

// CLION: Clang-Tidy: Initialization of 'future' with static storage duration may throw an exception that cannot be caught
// boost::future<int> future;
// Solution:
boost::future<int>& get_complete_future()
{
    static boost::future<int> future;
    return future;
}

auto main(int argc, char* argv[]) -> int
{
    SetConsoleOutputCP(65001);

    using HostProcessorPtr = std::unique_ptr<nurl::HostProcessor>;
    cons_handler_setup;

    try
    {
        if (!nurl_conf::details::POInit(argc, argv)) // without init_log()! (inferior - just check for errors in console mode)
        {
            return 0; //1?
        }

        nurl_logging::init_log();
        nurl_conf::details::POInit(argc, argv); // full-fledged one!

        if (nurl_conf::details::host_addr_vec.empty())
        {
            nurl_logging::Log_Wrapper (" Не найдены адреса хостов.");
            return 0;
        }

        std::vector<HostProcessorPtr> host_processor_vec;
        boost::asio::io_service ioService{};
        for (const auto& el : nurl_conf::details::host_addr_vec)
        {
            using namespace nurl_conf::details;
            host_processor_vec.push_back(std::make_unique<nurl::HostProcessor>(el, host_user, host_password, url_type, ioService));
        }

        boost::thread_group threadpool;
        boost::asio::io_service::work work(ioService);

        const auto thread_num = std::min(host_processor_vec.size(), nurl_conf::details::hardware_threads);

        for (auto i = 0u; i!= thread_num; ++i)
            threadpool.create_thread(
                boost::bind(&boost::asio::io_service::run, &ioService)
            );

        get_complete_future() = nurl::interact::details::get_prom().get_future();
        nurl::interact::details::run_tasks.store(host_processor_vec.size());

        for (const auto& host: host_processor_vec)
        {
            using namespace nurl::interact::details;
            platform_guard_lock lk (get_ioservice_post_mutex());

            ioService.post(boost::bind(&nurl::HostProcessor::processor_task, host.get()));
        }

        get_complete_future().wait();
        ioService.stop();

        threadpool.join_all();
        nurl_logging::Log_Wrapper ("--Завершено.");
    }
    catch (boost::exception & x)
    {
        nurl_logging::Log_Wrapper lw(diagnostic_information(x));
    }

    return 0;
}


