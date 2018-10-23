#include "init_logs.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/support/date_time.hpp>

#include "program_options.h"

namespace nurl_logging
{

    namespace logging = boost::log;
    namespace attrs = boost::log::attributes;
    namespace src = boost::log::sources;
    namespace sinks = boost::log::sinks;
    namespace expr = boost::log::expressions;
    namespace keywords = boost::log::keywords;


    using boost::shared_ptr;

    void init_log()
    {
        // Create a text file sink
        typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink;
        shared_ptr< file_sink > sink(new file_sink(
                                         keywords::file_name = nurl_conf::details::log_path + "/LOGS/LOGNURL%3N.TXT",
                                         keywords::rotation_size = 1 * 1024 * 1024   // rotation size, in characters
                                     ));

        // Set up where the rotated files will be stored
        sink->locked_backend()->set_file_collector(sinks::file::make_collector(
                    keywords::target = nurl_conf::details::log_path + "/LOGS",                          // where to store rotated files
                    keywords::max_size = 2 * 1024 * 1024,              // maximum total size of the stored files, in bytes
                    keywords::min_free_space = 1000 * 1024 * 1024        // minimum free space on the drive, in bytes
                ));

        sink->locked_backend()->auto_flush(true);
        // Upon restart, scan the target directory for files matching the file_name pattern
        sink->locked_backend()->scan_for_files();


        // Add it to the core
        logging::core::get()->add_sink(sink);
        logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
        //logging::core::get()->add_global_attribute("RecordID", attrs::counter< unsigned int >());

        sink->set_formatter(
            expr::format("%1% %2%")
            //% expr::attr< unsigned int >("RecordID")
            % expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
            % expr::smessage);

    }
}
