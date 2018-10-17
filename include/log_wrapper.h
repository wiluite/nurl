#pragma once

#include <sstream>
#include "init_logs.h"
#include <boost/thread/mutex.hpp>
#include <iomanip>

namespace nurl_logging
{

//Stroustrup 4, 28.6.1

    static inline void print_to_obuf(std::ostream & ostream_buf, std::ostringstream & sstream_buf)
    {
    }

    template <class V, class ... Args>
    static inline void print_to_obuf (std::ostream & ostream_buf, std::ostringstream & sstream_buf, const V& value, const Args& ... args)
    {
        ostream_buf << std::setprecision(15) << value;
        sstream_buf << std::setprecision(15) << value;
        print_to_obuf(ostream_buf, sstream_buf, args...);
    }

    static boost::mutex cout_mutex;

    class Log_Wrapper
    {
    public:
        template <typename ...T> inline Log_Wrapper(const T& ... args)
        {
            using platform_guard_lock = boost::lock_guard<boost::mutex>;
            std::ostringstream ostream_buf {};
            std::ostringstream sstream_buf {};
            print_to_obuf (ostream_buf, sstream_buf, args...);
            BOOST_LOG_STREAM(my_logger::get()) << ostream_buf.str();
            platform_guard_lock lk (cout_mutex);
            std::cout << sstream_buf.str() << "\n";

        }
    };

}

