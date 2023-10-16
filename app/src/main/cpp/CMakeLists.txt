cmake_minimum_required(VERSION 3.18.1)

project("qr_cxx_android")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3" )
add_subdirectory(mbedtls)
INCLUDE_DIRECTORIES(mbedtls/include)



include_directories(${CMAKE_CURRENT_SOURCE_DIR}/wirehair)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/wirehair/include)
file(GLOB_RECURSE wirehair_src
        ${CMAKE_CURRENT_SOURCE_DIR}/wirehair/*.cpp)

add_subdirectory (core)
zxing_add_package_stb()
add_library(
        qr_cxx_android
        SHARED
        native-lib.cpp
        cJSON.c
        QrTask.cpp
        ${wirehair_src})

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