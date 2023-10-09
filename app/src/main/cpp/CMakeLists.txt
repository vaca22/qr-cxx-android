cmake_minimum_required(VERSION 3.18.1)

project("qr_cxx_android")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3" )
add_subdirectory(mbedtls)
INCLUDE_DIRECTORIES(mbedtls/include)

add_subdirectory (core)
zxing_add_package_stb()
add_library(
        qr_cxx_android
        SHARED
        native-lib.cpp
        cJSON.c
        QrTask.cpp)

find_library(
        log-lib
        log)


target_link_libraries(
        qr_cxx_android
        ZXing::ZXing
        stb::stb
        mbedcrypto
        mbedx509
        mbedtls
        ${log-lib})