#include "g13.h"
#include <log4cpp/OstreamAppender.hh>

namespace G13 {

void G13_Manager::start_logging() {
    log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
    appender1->setLayout(new log4cpp::BasicLayout());
    log4cpp::Category &root = log4cpp::Category::getRoot();
    root.addAppender(appender1);

// TODO: this is for later when --log_file is implemented
//    log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", "g13d-output.log");
//    appender2->setLayout(new log4cpp::BasicLayout());
//    log4cpp::Category &sub1 = log4cpp::Category::getInstance(std::string("sub1"));
//    sub1.addAppender(appender2);
}

void G13_Manager::set_log_level(log4cpp::Priority::PriorityLevel lvl) {
    // boost::log::core::get()->set_filter(::boost::log::trivial::severity >= lvl);
    G13_OUT("set log level to " << lvl);
}

void G13_Manager::set_log_level(const std::string& level) {
    /*
    #define CHECK_LEVEL(L)                           \
        if (level == BOOST_PP_STRINGIZE(L)) {        \
            set_log_level(::boost::log::trivial::L); \
            return;                                  \
        }

        CHECK_LEVEL(trace);
        CHECK_LEVEL(debug);
        CHECK_LEVEL(info);
        CHECK_LEVEL(warning);
        CHECK_LEVEL(error);
        CHECK_LEVEL(fatal);
    */
    log4cpp::Category& root = log4cpp::Category::getRoot();
    try {
        auto numLevel = log4cpp::Priority::getPriorityValue(level);
        root.setPriority(numLevel);
    } catch (std::invalid_argument& e) {
        G13_ERR("unknown log level " << level);
    }
}

}  // namespace G13
