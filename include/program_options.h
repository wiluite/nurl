#pragma once

#include <vector>
#include <string>

namespace nurl_conf
{
    namespace details
    {
        extern bool POInit(int argc, char* argv[]);
        extern std::vector<std::string> host_addr_vec;
        extern std::string host_user;
        extern std::string host_password;
        extern std::string url_type;
        extern std::string digest_path;
        extern std::string digest_name;
        extern long speed_limit_bytes;
        extern long speed_limit_time;
        extern long connect_timeout;
        extern size_t connects;
        extern size_t hardware_threads;
        extern std::string log_path;
    }
}


