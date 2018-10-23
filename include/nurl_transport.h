#pragma once

#include <curl/curl.h>
#include "log_wrapper.h"
#include <boost/predef.h> //BOOST_ARCH_X86_64_AVAILABLE
#include <map>
#include <boost/thread/once.hpp>

namespace nurl
{
    namespace details
    {
        static boost::once_flag curl_init_flag{};
        static boost::once_flag curl_fini_flag{};

        boost::mutex& get_mutex()
        {
            static boost::mutex curl_mutex;
            return curl_mutex;
        }

        // is CLION wrong?
        static const auto curl_delete = [](CURL* const p)
        {
            if (p) ::curl_easy_cleanup(p);
        };

        static std::unique_ptr<CURL, decltype(curl_delete)> curl_easy_init_locked()
        {
            boost::lock_guard<boost::mutex> lk (get_mutex());
            return std::unique_ptr<CURL, decltype(curl_delete)> {::curl_easy_init(), curl_delete};
        }

        size_t write_func_prototype(void *buffer, size_t size, size_t nmemb, void* stream);
        using write_func_type = decltype (write_func_prototype);

        std::tuple<CURLcode, double> curl_download_file(const std::string& url_addr, write_func_type write_func, void* write_func_data, long speed_limit, long speed_time, long connect_timeout, std::streamoff pos=0)
        {
            auto curl = curl_easy_init_locked();
            if (curl)
            {
                curl_easy_setopt(curl.get(), CURLOPT_URL,url_addr.c_str());
                curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_func);
                curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, write_func_data);
                curl_easy_setopt(curl.get(), CURLOPT_USE_SSL, CURLUSESSL_NONE);
                curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_LIMIT, speed_limit);
                curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_TIME, speed_time);

                curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, connect_timeout);
                curl_easy_setopt(curl.get(), CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD); // Clion/Clang-Tidy complains
                curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS, CURLPROTO_ALL); // CLion/Clang-Tidy complains
                char r[32] {};
#ifdef BOOST_ARCH_X86_64_AVAILABLE
                sprintf (r, "%zd-", pos);
#else
                sprintf (r, "%lld-", pos);
#endif
                curl_easy_setopt(curl.get(), CURLOPT_RANGE, r);
                auto res = curl_easy_perform(curl.get());
                double medium_speed = 0;
                if (res == CURLE_OPERATION_TIMEDOUT)
                {
                    curl_easy_getinfo(curl.get(), CURLINFO_SPEED_DOWNLOAD, &medium_speed);
                }
                return std::make_tuple(res, medium_speed);
            }
            else
            {
                return std::make_tuple(CURLE_FAILED_INIT, 0);
            }
        }

        std::map<size_t, std::string>& get_Code2DescMap() {

            static std::map<size_t, std::string> Code2DescMap =
                    {
                            {0,  " (Нет ошибки)"},
                            {1,  " (Неподдерживаемый протокол)"},
                            {2,  " (Ошибка инициализации)"},
                            {3,  " (Неправильный формат URL)"},
                            {4,  " (CURLE_NOT_BUILT_IN)"},
                            {5,  " (CURLE_COULDNT_RESOLVE_PROXY)"},
                            {6,  " (CURLE_COULDNT_RESOLVE_HOST)"},
                            {7,  " (Ошибка установки соединения)"},
                            {28, " (ТАЙМАУТ)"},
                            {37, " (CURLE не может применить протокол FILE по каким-то причинам)"},
                            {67, " (CURLE_LOGIN_DENIED)"},
                            {78, " (CURLE_REMOTE_FILE_NOT_FOUND)"},
                            {79, " (CURLE_SSH)"}
                    };
            return Code2DescMap;
        }

        std::tuple<bool, size_t> curl_get_file_size(const std::string& url_addr, long connect_timeout = 5)
        {
            using namespace nurl_logging;
            double filesize = 0.0;

            auto curl = curl_easy_init_locked();
            if(curl)
            {
                curl_easy_setopt(curl.get(), CURLOPT_URL, url_addr.c_str());
                curl_easy_setopt(curl.get(), CURLOPT_NOBODY, 1L);

                curl_write_callback throw_away_cb = [](char* /*ptr*/, std::size_t size, std::size_t nmemb, void* /*data*/) -> std::size_t
                {
                    return (size_t)(size * nmemb);
                };

                curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, throw_away_cb);
                curl_easy_setopt(curl.get(), CURLOPT_HEADER, 1);
                curl_easy_setopt(curl.get(), CURLOPT_IGNORE_CONTENT_LENGTH, 1);
                curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 0L);
                curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, throw_away_cb);
                curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, connect_timeout);
                auto res = curl_easy_perform(curl.get());
                if(CURLE_OK == res)
                {
                    res = curl_easy_getinfo(curl.get(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
                    if((CURLE_OK == res) && (filesize > 0.0))
                    {
                        return std::make_tuple(true, filesize);
                    }
                    else
                    {
                        Log_Wrapper ("CURLINFO_CONTENT_LENGTH_DOWNLOAD error: ", res, get_Code2DescMap()[res], "\n", url_addr);
                        Log_Wrapper ("--Файл в оглавлении, но (вероятно) фактически удален с хоста.");
                    }
                }
                else
                {
                    Log_Wrapper (boost::this_thread::get_id(), " Ошибка установления размера файла: ", res, get_Code2DescMap()[res], "\n", url_addr);
                }
            }
            return std::make_tuple(false, filesize);
        }

        struct CommonUrlPathComposer
        {
            virtual const std::string compose(std::string host_addr, const std::string& user, const std::string& password) const = 0;
            virtual ~CommonUrlPathComposer() = default;
        };

        struct FtpCommonUrlPathComposer : public CommonUrlPathComposer
        {
            const std::string compose(std::string host_addr, const std::string& user, const std::string& password) const override
            {
                return std::string("ftp://")+user+":"+password+"@" + host_addr + "/";
            }
        };

        const static std::map<std::string, std::shared_ptr<CommonUrlPathComposer>> url_path_composer_map = {{"ftp", std::make_shared<FtpCommonUrlPathComposer>()} };
    }
}
