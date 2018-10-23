#pragma once

#include <boost/asio/io_service.hpp>
#include <fstream>
#include "log_wrapper.h"
#include <boost/filesystem.hpp>
#include "nurl_interact.h"
#include "contents_proc.h"
#include "file_enum.h"

namespace nurl
{
    class HostProcessor final
    {
        template <class HOST_PROC, class FILE_PARAMS>
        friend class file_enumerator;

        std::string host_addr_;
        boost::asio::io_service& ioService_;
        ContentsProcessor contents_processor;
        size_t conn_times;
        const std::string common_url_path;

        struct file_params
        {
            std::string filename {};
            size_t filesize {};
            std::streamoff download {}; //range
            bool bad_stat {};
        };
        std::deque<file_params> file_params_deque;

        // Clang complaint, but used by friend class file_enumerator
        uint8_t file_params_deque_cursor = 0;

        struct write_function_data_struct
        {
            std::ofstream* ofs = nullptr;
            size_t write_function_bytes {};
        };

        write_function_data_struct write_function_data;

        static size_t write_function(void *buffer, size_t sz, size_t nmemb, void* stream)
        {
            auto realsize = sz * nmemb;
            write_function_data_struct& wfd = *static_cast<write_function_data_struct*>(stream);
            wfd.ofs->write ((const char*)buffer, realsize);
            wfd.write_function_bytes += realsize;
            return realsize;
        }

        void init() const noexcept
        {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }

        void fini() const noexcept
        {
            curl_global_cleanup();
        }

        void sm_step_proto();
        decltype (&HostProcessor::sm_step_proto) sm_step = nullptr;

        void process_contents()
        {
            if (contents_processor.read_contents())
            {
                for (const auto& fn : contents_processor.get_file_queue())
                {
                    file_params_deque.push_back({fn, 0, 0, false});
                }

                if (file_params_deque.empty())
                {
                    nurl_logging::Log_Wrapper ("--Согласно оглавлению нет файлов для скачивания по ", host_addr_);
                    process_failure(conn_times);
                    return;
                }

                sm_step = perform_download;
                repost_processor_task();
            }
            else
            {
                nurl_logging::Log_Wrapper ("--Не читается оглавление по ", host_addr_);
                process_failure(conn_times);
            }
        }

        void repost_processor_task()
        {
            interact::details::platform_guard_lock lk (interact::details::get_ioservice_post_mutex());
            ioService_.post(boost::bind(&HostProcessor::processor_task, this));
        }

        template <typename COUNTER_TO_DEC>
        void process_failure(COUNTER_TO_DEC& counter)
        {
            if (counter--)
            {
                repost_processor_task();
            }
            else
            {
                cancel_next_post();
            }
        }

        void cancel_next_post() noexcept
        {
            interact::details::run_tasks.fetch_sub(1, std::memory_order_relaxed);
        }

        std::string get_local_filename(const std::string& deq_elem_filename) const
        {
            boost::filesystem::path out_path (deq_elem_filename);
            return (out_path.stem().string() + out_path.extension().string());
        }

        std::string compose_url_addr(const std::string& filename) const
        {
            return common_url_path + filename;
        }

        // simple if-algorithm with no state machine
        void perform_download()
        {
            using namespace nurl_logging;

            file_enumerator<HostProcessor, file_params> file_enr(*this);
            auto& deq_elem = file_enr.get_deq_elem();

            if (!deq_elem.bad_stat)
            {
                if (deq_elem.filesize == 0)
                {
                    const auto r = details::curl_get_file_size(compose_url_addr(deq_elem.filename));
                    if (std::get<0>(r))
                    {
                        calibrate_local_file(deq_elem, r);
                    }
                    else
                    {
                        deq_elem.bad_stat = true;
                    }
                } else if (deq_elem.download < deq_elem.filesize)
                {
                    const auto &local_filename = get_local_filename(deq_elem.filename);
                    std::ofstream ofs(local_filename, std::ios_base::app | std::ios_base::binary);

                    download(deq_elem, &ofs, local_filename);

                    check_downloaded(deq_elem, local_filename);
                } else if (deq_elem.download > deq_elem.filesize)
                {
                    Log_Wrapper(get_local_filename(deq_elem.filename), " ", deq_elem.download, " из ", deq_elem.filesize);
                    Log_Wrapper("Вероятно,  (ошибка использования).");
                    file_enr.cancel_post();
                }
            }
        }

        void check_downloaded(file_params &deq_elem, const std::string &local_filename) const
        {
            if ((deq_elem.download += write_function_data.write_function_bytes) == deq_elem.filesize)
            {
                nurl_logging::Log_Wrapper (local_filename, " ", deq_elem.download, " ЗАВЕРШЕН");
            }
        }

        void download(file_params &deq_elem, std::ofstream* ofs, const std::string& local_filename)
        {
            using namespace nurl_conf::details;
            using namespace details;
            using namespace nurl_logging;

            write_function_data.ofs = ofs;
            write_function_data.write_function_bytes = 0;

            CURLcode err_code;
            double medium_speed;
            std::tie(err_code, medium_speed) = curl_download_file(compose_url_addr(deq_elem.filename), write_function, &write_function_data, speed_limit_bytes, speed_limit_time, connect_timeout, deq_elem.download);
            if (CURLE_OK != err_code)
            {
                if (CURLE_OPERATION_TIMEDOUT == err_code)
                {
                    Log_Wrapper (local_filename, " ", deq_elem.download + write_function_data.write_function_bytes, " байтов, прерван по лимиту на скорость (Б/с): ", medium_speed, "/", speed_limit_bytes);
                }
                else
                {
                    Log_Wrapper (local_filename, " ", deq_elem.download + write_function_data.write_function_bytes, " байтов, прерван по коду ошибки ", err_code, " ", get_Code2DescMap()[err_code]);
                    if (CURLE_REMOTE_FILE_NOT_FOUND == err_code)
                    {
                        Log_Wrapper (local_filename, " вероятно, не хватает права доступа на чтение. Пропуск...");
                        deq_elem.bad_stat = true;
                    }
                }
            }
        }

        void calibrate_local_file(file_params &deq_elem, const std::tuple<bool, size_t> &r) const
        {
            deq_elem.filesize = std::get<1>(r);
            const auto& local_filename = get_local_filename(deq_elem.filename);
            nurl_logging::Log_Wrapper (local_filename, " РАЗМЕР ", deq_elem.filesize, " по ", host_addr_);
            if (boost::filesystem::exists(local_filename))
            {
                std::ifstream ifs (local_filename, std::ios_base::ate | std::ios_base::binary);
                BOOST_ASSERT (deq_elem.download == 0);
                deq_elem.download  = ifs.tellg();
                nurl_logging::Log_Wrapper (local_filename, " ", deq_elem.download, " НАЙДЕН");
            }
        }

    public:
        // CLION: Constructor 'HostProcessor' is never used
        // TODO: todo
        HostProcessor(const std::string& host_addr, const std::string& host_user, const std::string& host_password, const std::string& url_type, boost::asio::io_service& ioService):
            host_addr_(host_addr)
            , ioService_(ioService)
            , contents_processor(host_addr, host_user, host_password, url_type), conn_times(nurl_conf::details::connects - 1)
            , common_url_path(details::url_path_composer_map.at(url_type)->compose(host_addr, host_user, host_password))
            , sm_step(process_contents)
        {
            boost::call_once(details::curl_init_flag,boost::bind(&HostProcessor::init,this));
        }

        ~HostProcessor()
        {
            boost::call_once(details::curl_fini_flag,boost::bind(&HostProcessor::fini,this));
        }

        void processor_task()
        {
            static const auto set_prom = [] () -> void { try { interact::details::get_prom().set_value(0); } catch (...) {} };

            if (interact::details::cons_exit)
            {
                interact::details::run_tasks.fetch_sub(1, std::memory_order_relaxed);
                set_prom();
                return;
            }

            (this->*sm_step)();

            if (!interact::details::run_tasks.fetch_sub(0, std::memory_order_relaxed))
            {
                set_prom();
            }
        }

        HostProcessor(const HostProcessor&) = delete;
        HostProcessor& operator=(const HostProcessor&) = delete;
        HostProcessor(HostProcessor&&) = delete;
        HostProcessor& operator=(HostProcessor&&) = delete;
    };

} // namespace nurl
