#include "program_options.h"
#include <boost/program_options.hpp>
#include <fstream>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception_ptr.hpp>
#include "log_wrapper.h"
#include <boost/mpl/for_each.hpp>
#include <boost/thread.hpp>
#include <iostream>

namespace nurl_conf
{
    namespace po = boost::program_options;
    namespace mpl = boost::mpl;

    std::string config_file = {};

    struct error : virtual std::exception, virtual boost::exception { };
    struct illegal_thing : virtual error { };
    using errinfo_illegal_thing = boost::error_info<struct tag_illegal_thing,std::string>;

    std::vector<std::string> details::host_addr_vec;
    std::string details::host_user;
    std::string details::host_password;
    std::string details::url_type;
    std::string details::digest_path;
    std::string details::digest_name;
    long details::speed_limit_bytes;
    decltype(details::speed_limit_time) details::speed_limit_time;
    decltype(details::connect_timeout) details::connect_timeout;
    size_t details::connects;
    size_t details::hardware_threads;
    std::string details::log_path;

    struct program_options_logger
    {
        program_options_logger(boost::any& val, std::string n) : value (&val), name(std::move(n)) {}
        template< typename U > void operator()(U)
        {
            if (auto v = boost::any_cast<U> (value))
                nurl_logging::Log_Wrapper lw (name, std::string(" = "), *v);
            else
                process_vector<U>();
        }
    private:
        template<typename T>
        void process_vector()
        {
            using namespace nurl_logging;
            if (auto v = boost::any_cast<std::vector<T>> (value))
            {
                for (const auto& v2 : *v)
                {
                    Log_Wrapper lw (name, std::string(" = "), v2);
                }
            }
        }
        boost::any* value;
        const std::string name;
    };

    bool details::POInit(int argc, char* argv[])
    {
        try
        {
            using namespace nurl_logging;
            using namespace details;

            po::options_description generic("Generic options");
            generic.add_options()
            ("version,v", "print version string")
            ("help", "produce help message")
            ("config,c", po::value<std::string>(&config_file)->default_value("nurl.cfg"),
             "name of a configuration file.")
            ;

            po::options_description config("Configuration");
            config.add_options()
            ("host-addr", po::value< std::vector<std::string>>(&host_addr_vec),
             "eg. --host-addr=IP1, --host-addr=IP2... --host-addr=IPn")
            ("host-user", po::value< std::string>(&host_user)->default_value("someuser"),
             "eg. --host-user=<user>")
            ("host-password", po::value< std::string>(&host_password)->default_value("somepassword"),
             "eg. --host-password=<password>")


            ("URL-type", po::value< std::string>(&url_type)->default_value("ftp"),
             "eg. --URL-type=sftp | --URL-type=ftp ")
            ("digest-path", po::value< std::string>(&digest_path)->default_value("/home/someuser"),
             "eg. --digest-path=<path>")
            ("digest-name", po::value< std::string>(&digest_name)->default_value("digest"),
             "eg. --digest-name=<name>")
            ("speed-limit-bytes", po::value< long>(&speed_limit_bytes)->default_value(500000),
             "Minimal bytes per second to go on downloading, eg. --speed-limit-bytes=<bytes>")
            ("speed-limit-time", po::value<decltype(speed_limit_time)>(&speed_limit_time)->default_value(5),
             "Maximal time to download at limited speed (see speed-limit-bytes), eg. --speed-limit-time=<seconds>")
            ("connect-timeout", po::value<decltype(connect_timeout)>(&connect_timeout)->default_value(5),
             "Maximal time to connect to any remote host {1-10}, eg. --connect-timeout=<seconds>")
            ("connects", po::value< size_t>(&connects)->default_value(5),
             "Connect attempts (for each host) {1-10}, eg. --connects=<value>")
            ("hardware-threads", po::value< size_t>(&hardware_threads)->default_value(2),
             "Hardware threads, eg. --hardware-threads=<value>")
            ("log-path", po::value< std::string>(&log_path)->default_value("."),
             "eg. --log-path=<path>")


            ;

            po::options_description hidden("Hidden options");
            hidden.add_options()
            ("input-file", po::value< std::vector<std::string> >(), "input file")
            ;


            po::options_description cmdline_options {};
            cmdline_options.add(generic).add(config).add(hidden);

            po::options_description config_file_options;
            config_file_options.add(config).add(hidden);

            po::options_description visible("Allowed options");
            visible.add(generic).add(config);

            po::positional_options_description p;
            p.add("input-file", -1);

            po::variables_map vm;
            store(po::command_line_parser(argc, argv).
                  options(cmdline_options).positional(p).run(), vm);

            notify(vm);

            if (vm.count("help"))
            {
                std::cout << visible << "\n";
                return true;
            }

            if (vm.count("version"))
            {
                std::cout << "NURL: NOT CURL, version 1.0.0\n";
                return true;
            }

            std::ifstream ifs(config_file.c_str());
            if (!ifs)
            {
                std::cout << "Can not open config file: " << config_file << "\n";
                return true;
            }
            else
            {
                store(parse_config_file(ifs, config_file_options), vm);
                notify(vm);
            }

            if (vm.count ("host-addr"))
            {
                // place duplicated to the tail
                std::sort(std::begin(host_addr_vec), std::end(host_addr_vec));
                auto pos = std::unique(std::begin(host_addr_vec), std::end(host_addr_vec));
                // and cut off these
                host_addr_vec.erase(pos, std::end(host_addr_vec));
            }
            if (vm.count ("URL-type"))
            {
                if ((url_type != "sftp") && (url_type != "ftp") )
                {
                    Log_Wrapper ("Неизвестный URL-type: ", url_type);
                    return true;
                }
            }
            if (vm.count ("connect-timeout"))
            {
                if ((connect_timeout ==0) || (connect_timeout > 10))
                {
                    Log_Wrapper ("Неправильный connect-timeout: ", connect_timeout);
                    return true;
                }
            }
            if (vm.count ("connects"))
            {
                if ((connects == 0) || (connects > 10))
                {
                    Log_Wrapper ("Неправильный параметр connects: ", connects);
                    return true;
                }
            }
            if (vm.count ("hardware-threads"))
            {
                if ((hardware_threads == 0) || (hardware_threads > boost::thread::hardware_concurrency()))
                {
                    Log_Wrapper ("Число аппаратных потоков 0 или превышает аппаратный лимит (hardware_concurrency): ", hardware_threads, " > ", boost::thread::hardware_concurrency());
                    return true;
                }
            }
            const std::string& hyphen {"------------------------------"};
            Log_Wrapper(hyphen.c_str());
            for (auto& it : vm)
            {
                using program_options_types = mpl::list<std::string, size_t> ;
                mpl::for_each<program_options_types>( program_options_logger(it.second.value(), it.first) );
            }
            Log_Wrapper(hyphen.c_str());
        }
        catch (boost::exception &x)
        {
            throw;
        }
        catch(std::exception& e)
        {
            BOOST_THROW_EXCEPTION(
                illegal_thing() <<
                boost::errinfo_errno(errno)  <<
                errinfo_illegal_thing(e.what()) );
        }
        return true;
    } // POInit


} //namespace
