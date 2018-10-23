#pragma once

#include <boost/algorithm/string.hpp>
#include "program_options.h"
#include "nurl_transport.h"

namespace nurl
{
    class ContentsProcessor final
    {
        const std::string impulse_log_url;
        const std::string host_addr_;
        const std::string host_user_;
        const std::string host_password_;

        mutable std::stringstream oss;

        static size_t wr_func(void *buffer, size_t sz, size_t nmemb, void* stream)
        {
            const auto realsize = sz * nmemb;
            static_cast<std::stringstream*>(stream)->write((const char*)buffer, realsize);
            return realsize;
        }
    public:
        ContentsProcessor (const std::string& host_addr, const std::string& host_user, const std::string& host_password, const std::string& url_type) :
            impulse_log_url(details::url_path_composer_map.at(url_type)->compose(host_addr, host_user, host_password) + nurl_conf::details::digest_path + "/" + nurl_conf::details::digest_name)
            , host_addr_(host_addr)
            , host_user_(host_user)
            , host_password_(host_password)
            {}

        bool read_contents() const
        {
            using namespace nurl_conf::details;

            CURLcode err_code;
            std::tie (err_code, std::ignore) = details::curl_download_file(impulse_log_url, wr_func, &oss, speed_limit_bytes, speed_limit_time, connect_timeout);
            if (CURLE_OK != err_code)
            {
                nurl_logging::Log_Wrapper ("--Код ошибки ", err_code, details::get_Code2DescMap()[err_code], " по хосту: ", host_addr_);
                return false;
            }
            return true;
        }

        std::deque <std::string> get_file_queue() const
        {
            // Clion bug/feature: Function 'get_file_queue()' "recurses" infinitely
            decltype (get_file_queue()) deq;
            static_assert (std::is_same<std::deque <std::string>, decltype (get_file_queue())>::value, "");

            for (std::string buf; getline (oss, buf); )
            {
                std::replace (std::begin(buf), std::end(buf), '\\', '/');
                boost::algorithm::replace_all(buf, "C:", "");
                buf = boost::algorithm::trim_right_copy(buf);
                if (!buf.empty())
                {
                    deq.push_back(buf);
                }
            }
            return deq;
        }

        ContentsProcessor(const ContentsProcessor&) = delete;
        ContentsProcessor& operator=(const ContentsProcessor&) = delete;
        ContentsProcessor(ContentsProcessor&&) = delete;
        ContentsProcessor& operator=(ContentsProcessor&&) = delete;

    };

}
