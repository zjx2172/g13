#include <getopt.h>
#include "g13.h"

//#include <boost/program_options.hpp>
//#if 0
//#include <boost/log/attributes.hpp>
//#include <boost/log/core/core.hpp>
//#include <boost/log/expressions.hpp>
//#include <boost/log/expressions/formatters/stream.hpp>
//#include <boost/log/support/date_time.hpp>
//#include <boost/log/trivial.hpp>
//#include <boost/log/utility/setup.hpp>
//#include <boost/log/utility/setup/console.hpp>
//#endif

// using namespace std;
using namespace G13;
// namespace po = boost::program_options;

void printHelp() {
    const auto indent = 24;
    std::cout << "Allowed options" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --help"
              << "produce help message" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --logo <file>"
              << "set logo from file" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --config <file>"
              << "load config commands from file" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --pipe_in <name>"
              << "specify name for input pipe" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --pipe_out <name>"
              << "specify name for output pipe" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --log_level <level>"
              << "logging level" << std::endl;
    // std::cout << std::left << std::setw(indent) << "logfile" << "write log to logfile" <<
    // std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    G13_Manager manager;
    manager.set_log_level("info");

    /*    // Declare the supported options.
        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message");
        std::vector<std::string> sopt_names;
        auto add_string_option = [&sopt_names, &desc](const char* name, const char* description) {
            desc.add_options()(name, po::value<std::string>(), description);
            sopt_names.push_back(name);
        };
        add_string_option("logo", "set logo from file");
        add_string_option("config", "load config commands from file");
        add_string_option("pipe_in", "specify name for input pipe");
        add_string_option("pipe_out", "specify name for output pipe");
        add_string_option("log_level", "logging level");
        // add_string_option( "logfile", "write log to logfile" );

        po::positional_options_description p;
        p.add("logo", -1);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << argv[0] << " : user space G13 driver" << endl;
            cout << desc << "\n";
            return 1;
        }

        BOOST_FOREACH (const std::string& tag, sopt_names) {
            if (vm.count(tag)) {
                manager.set_string_config_value(tag, vm[tag].as<std::string>());
            }
        }

        if (vm.count("logo")) {
            manager.set_logo(vm["logo"].as<std::string>());
        }

        if (vm.count("log_level")) {
            manager.set_log_level(manager.string_config_value("log_level"));
        }*/

    const char* const short_opts = "l:c:i:o:d:h";
    const option long_opts[] = {{"logo", required_argument, nullptr, 'l'},
                                {"config", required_argument, nullptr, 'c'},
                                {"pipe_in", required_argument, nullptr, 'i'},
                                {"pipe_out", required_argument, nullptr, 'o'},
                                {"log_level", required_argument, nullptr, 'd'},
                                {"help", no_argument, nullptr, 'h'},
                                {nullptr, no_argument, nullptr, 0}};
    while (true) {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

        if (-1 == opt) {
            break;
        }

        switch (opt) {
            case 'l':
                manager.set_string_config_value("logo", std::string(optarg));
                manager.set_logo(std::string(optarg));
                break;

            case 'c':
                manager.set_string_config_value("config", std::string(optarg));
                break;

            case 'i':
                manager.set_string_config_value("pipe_in", std::string(optarg));
                break;

            case 'o':
                manager.set_string_config_value("pipe_out", std::string(optarg));
                break;

            case 'd':
                manager.set_string_config_value("log_level", std::string(optarg));
                manager.set_log_level(manager.string_config_value("log_level"));
                break;

            case 'h':  // -h or --help
            case '?':  // Unrecognized option
            default:
                printHelp();
                break;
        }
    }
    return manager.run();
}
