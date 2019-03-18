cmake_minimum_required(VERSION 3.13)
project(g13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-std=c++17")

include_directories(.)

add_executable(g13
        g13.cc
        g13.h
        g13_fonts.cc
        g13_keys.cc
        g13_lcd.cc
        g13_log.cc
        g13_main.cc
        g13_stick.cc
        g13_test.py
        helper.cpp
        helper.hpp
        logo.h)

add_executable(pbm2lpbm
        pbm2lpbm.cc)

add_executable(runtests
        g13.cc
        g13.h
        g13_fonts.cc
        g13_keys.cc
        g13_lcd.cc
        g13_log.cc
        g13_stick.cc
        g13_test.py
        helper.cpp
        helper.hpp
        logo.h
        testKeys.cpp)

target_link_libraries (g13 usb-1.0 log4cpp pthread evdev)
target_link_libraries (runtests usb-1.0 log4cpp pthread evdev gtest gmock)