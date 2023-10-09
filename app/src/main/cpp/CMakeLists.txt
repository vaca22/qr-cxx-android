cmake_minimum_required(VERSION 3.18.1)

project("qr_cxx_android")

add_subdirectory (core)
zxing_add_package_stb()
add_library(
        qr_cxx_android
        SHARED
        native-lib.cpp)

find_library(
        log-lib
        log)


target_link_libraries(
        qr_cxx_android
        ZXing::ZXing stb::stb
        ${log-lib})