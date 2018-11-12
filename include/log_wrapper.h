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

    template <typename T>
    static inline void log_impl(std::ostream & log, T && val)
    {
        log << val;
    }

    // overload for float and double
    #define LOG_IMPL(value_type, mess) \
    inline void log_impl(std::ostream & log, value_type val) \
    { \
        log << std::setprecision(15) << val; \
    }

    LOG_IMPL (double &, "DOUBLE LV.")
    LOG_IMPL (double &&, "DOUBLE RV.")
    LOG_IMPL (float &, "FLOAT LV! ")
    LOG_IMPL (float &&, "FLOAT RV! ")
    LOG_IMPL (double const &, "CONST DOUBLE LV! ")
    LOG_IMPL (float const &, "CONST FLOAT LV! ")

    template <class First, class ... Rest>
    static inline void print_to_obuf (std::ostream & log, First && first, Rest && ... rest)
    {
        log_impl(log, std::forward<First>(first));
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
	    print_to_obuf (log, std::forward<T>(args) ...);
            BOOST_LOG_STREAM(my_logger::get()) << log.str();
            {
                boost::lock_guard<boost::mutex> lk (get_cons_mutex());
                std::wcout << std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> {}.from_bytes(log.str()) << "\n";
            }
        }
    };

}

