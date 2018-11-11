#pragma once
#include <boost/log/common.hpp>
#include <boost/log/sources/logger.hpp>

namespace nurl_logging
{
    void init_log();
    namespace src = boost::log::sources;
    BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, src::logger_mt);
}

