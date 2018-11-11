#pragma once

#include <sstream>
#include "init_logs.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <iomanip>
#include <iostream>
#include <codecvt>

namespace nurl_logging
{

//Stroustrup 4, 28.6.1

    static inline void print_to_obuf(std::ostream & log) noexcept
    {}

    template <class First, class ... Rest>
    static inline void print_to_obuf (std::ostream & log, First && first, Rest && ... rest)
    {
        log << std::setprecision(15) << first;
        print_to_obuf(log, std::forward<Rest>(rest) ...);
    }

    inline boost::mutex& get_cons_mutex()
    {
        static boost::mutex cons_mutex;
        return cons_mutex;
    };

    class Log_Wrapper
    {
    public:
        template <typename ... T>
        explicit Log_Wrapper(T && ... args)
        {
            std::stringstream log {};
            print_to_obuf (log, args...);
            BOOST_LOG_STREAM(my_logger::get()) << log.str();
            {
                boost::lock_guard<boost::mutex> lk (get_cons_mutex());
                std::wcout << std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> {}.from_bytes(log.str()) << "\n";
            }
        }
    };

}

