#include <fstream>
#include "g13.h"

//#include <boost/log/attributes.hpp>
//#include <boost/log/core/core.hpp>
//#include <boost/log/expressions.hpp>
//#include <boost/log/expressions/formatters/stream.hpp>
//#include <boost/log/sources/severity_feature.hpp>
//#include <boost/log/sources/severity_logger.hpp>
//#include <boost/log/support/date_time.hpp>
//#include <boost/log/trivial.hpp>
//#include <boost/log/utility/setup.hpp>
//#include <boost/log/utility/setup/console.hpp>

#include <log4cpp/Category.hh>
//#include <log4cpp/Appender.hh>
//#include <log4cpp/FileAppender.hh>
//#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/Layout.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Priority.hh>

using namespace std;

namespace G13 {

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
    } catch(std::invalid_argument &e) {
        G13_ERR("unknown log level" << level);
    }
}

}  // namespace G13
