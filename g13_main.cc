#include "g13.h"
#include <getopt.h>

using namespace G13;

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
//    std::cout << std::left << std::setw(indent) << "--log_file <file>"
//              << "write log to logfile" << std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    G13_Manager manager;
    manager.start_logging();
    manager.set_log_level("INFO");

    // TODO: move out argument parsing
    const char* const short_opts = "l:c:i:o:d:h";
    const option long_opts[] = {
        {"logo", required_argument, nullptr, 'l'},
        {"config", required_argument, nullptr, 'c'},
        {"pipe_in", required_argument, nullptr, 'i'},
        {"pipe_out", required_argument, nullptr, 'o'},
        {"log_level", required_argument, nullptr, 'd'},
        //                                {"log_file", required_argument, nullptr, 'f'},
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
