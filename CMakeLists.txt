cmake_minimum_required(VERSION 3.13)
project(g13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-std=c++17")

include_directories(.)

add_executable(g13
        g13.cpp
        g13.hpp
        g13_fonts.cpp
        g13_keys.cpp
        g13_lcd.cpp
        g13_log.cpp
        g13_main.cpp
        g13_stick.cpp
        g13_test.py
        helper.cpp
        helper.hpp
        logo.hpp)

set_source_files_properties(version PROPERTIES GENERATED true)
add_custom_target(version
        COMMAND sh GIT-VERSION-GEN
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_dependencies(g13 version)
ADD_CUSTOM_COMMAND(
        COMMENT "version files clean"
        COMMAND rm
        ARGS    -f GIT-VERSION-FILE GIT-VERSION.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        TARGET  g13
)
add_executable(pbm2lpbm
        pbm2lpbm.cpp)

add_executable(runtests
        g13.cpp
        g13.hpp
        g13_fonts.cpp
        g13_keys.cpp
        g13_lcd.cpp
        g13_log.cpp
        g13_stick.cpp
        g13_test.py
        helper.cpp
        helper.hpp
        logo.hpp
        testKeys.cpp)

target_link_libraries (g13 usb-1.0 log4cpp evdev)
target_link_libraries (runtests usb-1.0 log4cpp evdev gtest gmock)